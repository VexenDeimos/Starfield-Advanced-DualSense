#include <StarfieldDualSense/DualSenseAudioTransport.h>
#include <StarfieldDualSense/DualSenseAudioRenderBlock.h>
#include <StarfieldDualSense/SpeakerMixer.h>
#include <StarfieldDualSense/HapticCommandQueue.h>
#include <StarfieldDualSense/HapticEndpointSelection.h>
#include <StarfieldDualSense/HapticMixer.h>
#include <StarfieldDualSense/MusicHapticsMixer.h>
#include <StarfieldDualSense/HapticWaveforms.h>

#include <Windows.h>
#include <audioclient.h>
#include <propkeydef.h>
#include <functiondiscoverykeys_devpkey.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <propvarutil.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <cstdio>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr auto kMaxCommandAge = std::chrono::milliseconds(250);
    constexpr auto kDropLogInterval = std::chrono::seconds(1);
    constexpr std::size_t kMaxMusicControlCommands = 256u;

    std::string utf8(std::wstring_view text)
    {
        if (text.empty()) {
            return {};
        }
        const int required = WideCharToMultiByte(
            CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (required <= 0) {
            return {};
        }
        std::string result(static_cast<std::size_t>(required), '\0');
        WideCharToMultiByte(
            CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), required, nullptr, nullptr);
        return result;
    }

    std::string sampleFormatName(sds::HapticSampleFormat format)
    {
        switch (format) {
        case sds::HapticSampleFormat::Float32:
            return "float32";
        case sds::HapticSampleFormat::Pcm16:
            return "pcm16";
        case sds::HapticSampleFormat::Pcm32:
            return "pcm32";
        default:
            return "unsupported";
        }
    }

    sds::HapticSampleFormat classifyFormat(const WAVEFORMATEX* format) noexcept
    {
        if (!format) {
            return sds::HapticSampleFormat::Unsupported;
        }
        if (format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && format->wBitsPerSample == 32) {
            return sds::HapticSampleFormat::Float32;
        }
        if (format->wFormatTag == WAVE_FORMAT_PCM) {
            if (format->wBitsPerSample == 16) {
                return sds::HapticSampleFormat::Pcm16;
            }
            if (format->wBitsPerSample == 32) {
                return sds::HapticSampleFormat::Pcm32;
            }
            return sds::HapticSampleFormat::Unsupported;
        }
        if (format->wFormatTag != WAVE_FORMAT_EXTENSIBLE ||
            format->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
            return sds::HapticSampleFormat::Unsupported;
        }

        const auto* extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(format);
        if (IsEqualGUID(extensible->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) && format->wBitsPerSample == 32) {
            return sds::HapticSampleFormat::Float32;
        }
        if (IsEqualGUID(extensible->SubFormat, KSDATAFORMAT_SUBTYPE_PCM)) {
            if (format->wBitsPerSample == 16) {
                return sds::HapticSampleFormat::Pcm16;
            }
            if (format->wBitsPerSample == 32) {
                return sds::HapticSampleFormat::Pcm32;
            }
        }
        return sds::HapticSampleFormat::Unsupported;
    }

    bool compatible(const sds::HapticEndpointCandidate& candidate) noexcept
    {
        return sds::isDualSenseHapticEndpointName(candidate.friendlyName) &&
            candidate.channels == 4 && candidate.sampleRate == 48000 &&
            candidate.sampleFormat != sds::HapticSampleFormat::Unsupported;
    }
}

struct sds::DualSenseAudioTransport::Impl
{
    LogCallback logCallback{};
    bool debugLogging{ false };
    std::chrono::milliseconds reconnectInterval{ 2000 };
    HapticCommandQueue commands{};
    HapticMixer hapticMixer{};
    MusicHapticsMixer musicMixer{};
    enum class MusicControlKind : std::uint8_t { Add, StopPlayingId, Clear };
    struct MusicControlCommand
    {
        MusicControlKind kind{ MusicControlKind::Clear };
        MusicHapticVoice voice{};
        std::uint32_t playingId{ 0 };
    };
    std::mutex musicQueueMutex{};
    std::deque<MusicControlCommand> musicCommands{};
    SpeakerMixer speakerMixer;
    std::mutex speakerMixerMutex{};
    std::mutex speakerQueueMutex{};
    std::deque<SpeakerCommand> speakerCommands{};
    std::deque<PreparedSpeakerPcm> preparedSpeakerPcm{};
    enum class PersistentUpdateKind : std::uint8_t { Set, Clear };
    struct PersistentUpdate
    {
        PersistentUpdateKind kind{ PersistentUpdateKind::Clear };
        PersistentPreparedSpeakerPcm voice{};
        std::uint64_t owner{ 0 };
        bool force{ false };
    };
    std::optional<PersistentUpdate> pendingPersistent{};
    mutable std::mutex speakerPersistentCallbackMutex{};
    SpeakerPersistentInvalidationCallback speakerPersistentInvalidationCallback{};
    mutable std::mutex clientMutex{};
    DualSenseAudioClientLifetime clients{};
    std::atomic<std::uint32_t> continuousPacked{ 0U };
    std::atomic<bool> running{ false };
    std::atomic<bool> transportActive{ false };
    std::thread worker{};
    HANDLE stopEvent{ nullptr };
    HANDLE renderEvent{ nullptr };
    ComPtr<IMMDeviceEnumerator> enumerator{};
    ComPtr<IMMDevice> device{};
    ComPtr<IAudioClient> audioClient{};
    ComPtr<IAudioRenderClient> renderClient{};
    UINT32 bufferFrameCount{ 0 };
    HapticSampleFormat sampleFormat{ HapticSampleFormat::Unsupported };
    std::chrono::steady_clock::time_point nextDropLog{};

    explicit Impl(LogCallback log, bool debug, float speakerVolume, std::chrono::milliseconds reconnect) :
        logCallback(std::move(log)),
        debugLogging(debug),
        reconnectInterval(reconnect),
        speakerMixer(speakerVolume)
    {}

    void log(std::string_view message) const noexcept
    {
        if (!logCallback) {
            return;
        }
        try {
            logCallback(message);
        } catch (...) {
        }
    }

    bool shouldStop() const noexcept
    {
        return !running.load(std::memory_order_acquire) ||
            (stopEvent && WaitForSingleObject(stopEvent, 0) == WAIT_OBJECT_0);
    }

    bool waitForRetry() const noexcept
    {
        if (!stopEvent) {
            return true;
        }
        const auto waitMs = static_cast<DWORD>((std::max)(std::int64_t{ 0 }, reconnectInterval.count()));
        return WaitForSingleObject(stopEvent, waitMs) == WAIT_OBJECT_0;
    }

    void closeRenderEvent() noexcept
    {
        if (renderEvent) {
            CloseHandle(renderEvent);
            renderEvent = nullptr;
        }
    }

    void clearSpeakerState() noexcept
    {
        std::scoped_lock speakerStateLock(speakerQueueMutex, speakerMixerMutex);
        pendingPersistent.reset();
        speakerCommands.clear();
        preparedSpeakerPcm.clear();
        speakerMixer.clear();
    }

    void clearMusicState() noexcept
    {
        {
            std::scoped_lock musicLock(musicQueueMutex);
            musicCommands.clear();
        }
        musicMixer.clear();
    }

    void queueMusicClear() noexcept
    {
        try {
            std::scoped_lock musicLock(musicQueueMutex);
            musicCommands.clear();
            musicCommands.push_back(MusicControlCommand{ .kind = MusicControlKind::Clear });
        } catch (...) {
        }
    }

    void reportPersistentInvalidation(SpeakerPersistentInvalidationReason reason) noexcept
    {
        SpeakerPersistentInvalidationCallback callback;
        {
            std::scoped_lock callbackLock(speakerPersistentCallbackMutex);
            callback = speakerPersistentInvalidationCallback;
        }
        if (!callback) {
            return;
        }
        try {
            callback(reason);
        } catch (...) {
        }
    }

    void teardownTransport(bool notifyEndpointInvalidated) noexcept
    {
        const bool wasActive = transportActive.exchange(false, std::memory_order_acq_rel);
        clearSpeakerState();
        clearMusicState();
        if (audioClient) {
            (void)audioClient->Stop();
        }
        renderClient.Reset();
        audioClient.Reset();
        device.Reset();
        bufferFrameCount = 0;
        sampleFormat = HapticSampleFormat::Unsupported;
        closeRenderEvent();
        hapticMixer = HapticMixer{};
        if (notifyEndpointInvalidated && wasActive) {
            reportPersistentInvalidation(SpeakerPersistentInvalidationReason::EndpointInvalidated);
        }
    }


    bool ensureEnumerator()
    {
        if (enumerator) {
            return true;
        }
        const HRESULT hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            reinterpret_cast<void**>(enumerator.ReleaseAndGetAddressOf()));
        if (FAILED(hr)) {
            log("Haptics: MMDeviceEnumerator creation failed; retrying, HID features unaffected");
            enumerator.Reset();
            return false;
        }
        return true;
    }

    std::vector<HapticEndpointCandidate> enumerateCandidates()
    {
        std::vector<HapticEndpointCandidate> candidates;
        if (!ensureEnumerator()) {
            return candidates;
        }

        ComPtr<IMMDeviceCollection> collection;
        const HRESULT enumHr = enumerator->EnumAudioEndpoints(
            eRender, DEVICE_STATE_ACTIVE, collection.ReleaseAndGetAddressOf());
        if (FAILED(enumHr) || !collection) {
            log("Haptics: active render endpoint enumeration failed; retrying, HID features unaffected");
            return candidates;
        }

        UINT count = 0;
        if (FAILED(collection->GetCount(&count))) {
            log("Haptics: audio endpoint count failed; retrying, HID features unaffected");
            return candidates;
        }

        candidates.reserve(count);
        for (UINT index = 0; index < count; ++index) {
            ComPtr<IMMDevice> endpoint;
            if (FAILED(collection->Item(index, endpoint.ReleaseAndGetAddressOf())) || !endpoint) {
                continue;
            }

            LPWSTR rawId = nullptr;
            if (FAILED(endpoint->GetId(&rawId)) || !rawId) {
                if (rawId) {
                    CoTaskMemFree(rawId);
                }
                continue;
            }
            std::wstring id(rawId);
            CoTaskMemFree(rawId);

            std::wstring friendlyName;
            ComPtr<IPropertyStore> properties;
            if (SUCCEEDED(endpoint->OpenPropertyStore(STGM_READ, properties.ReleaseAndGetAddressOf())) && properties) {
                PROPVARIANT value;
                PropVariantInit(&value);
                if (SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName, &value)) && value.vt == VT_LPWSTR && value.pwszVal) {
                    friendlyName = value.pwszVal;
                }
                PropVariantClear(&value);
            }

            std::uint32_t sampleRate = 0;
            std::uint16_t channels = 0;
            HapticSampleFormat formatKind = HapticSampleFormat::Unsupported;
            ComPtr<IAudioClient> temporaryClient;
            if (SUCCEEDED(endpoint->Activate(
                    __uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                    reinterpret_cast<void**>(temporaryClient.ReleaseAndGetAddressOf()))) && temporaryClient) {
                WAVEFORMATEX* mix = nullptr;
                if (SUCCEEDED(temporaryClient->GetMixFormat(&mix)) && mix) {
                    sampleRate = mix->nSamplesPerSec;
                    channels = mix->nChannels;
                    formatKind = classifyFormat(mix);
                    CoTaskMemFree(mix);
                } else if (mix) {
                    CoTaskMemFree(mix);
                }
            }

            HapticEndpointCandidate candidate{
                std::move(id), std::move(friendlyName), sampleRate, channels, formatKind
            };

            if (isDualSenseHapticEndpointName(candidate.friendlyName) && !compatible(candidate)) {
                const auto name = utf8(candidate.friendlyName);
                const auto format = sampleFormatName(candidate.sampleFormat);
                char buffer[512]{};
                std::snprintf(
                    buffer, sizeof(buffer),
                    "Haptics: endpoint rejected name='%s' channels=%u rate=%u format=%s; requires supported 4ch/48k",
                    name.c_str(), static_cast<unsigned>(candidate.channels),
                    static_cast<unsigned>(candidate.sampleRate), format.c_str());
                log(buffer);
            }

            candidates.push_back(std::move(candidate));
        }
        return candidates;
    }

    bool initializeSelected(const HapticEndpointCandidate& candidate)
    {
        teardownTransport(false);
        if (!enumerator) {
            return false;
        }

        HRESULT hr = enumerator->GetDevice(candidate.id.c_str(), device.ReleaseAndGetAddressOf());
        if (FAILED(hr) || !device) {
            log("Haptics: selected endpoint disappeared before activation; retrying");
            teardownTransport(false);
            return false;
        }

        hr = device->Activate(
            __uuidof(IAudioClient), CLSCTX_ALL, nullptr,
            reinterpret_cast<void**>(audioClient.ReleaseAndGetAddressOf()));
        if (FAILED(hr) || !audioClient) {
            log("Haptics: selected endpoint audio client activation failed; retrying");
            teardownTransport(false);
            return false;
        }

        WAVEFORMATEX* mix = nullptr;
        hr = audioClient->GetMixFormat(&mix);
        if (FAILED(hr) || !mix) {
            if (mix) {
                CoTaskMemFree(mix);
            }
            log("Haptics: selected endpoint mix format unavailable; retrying");
            teardownTransport(false);
            return false;
        }

        const HapticSampleFormat selectedFormat = classifyFormat(mix);
        const bool valid = mix->nChannels == 4 && mix->nSamplesPerSec == 48000 &&
            selectedFormat != HapticSampleFormat::Unsupported;
        if (!valid) {
            CoTaskMemFree(mix);
            log("Haptics: selected endpoint format changed and is no longer supported 4ch/48k; retrying");
            teardownTransport(false);
            return false;
        }

        hr = audioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            0,
            0,
            mix,
            nullptr);
        CoTaskMemFree(mix);
        if (FAILED(hr)) {
            log("Haptics: WASAPI shared event initialization failed; retrying, HID features unaffected");
            teardownTransport(false);
            return false;
        }

        renderEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!renderEvent) {
            log("Haptics: WASAPI render event creation failed; retrying");
            teardownTransport(false);
            return false;
        }

        hr = audioClient->SetEventHandle(renderEvent);
        if (FAILED(hr)) {
            log("Haptics: WASAPI render event binding failed; retrying");
            teardownTransport(false);
            return false;
        }

        hr = audioClient->GetBufferSize(&bufferFrameCount);
        if (FAILED(hr) || bufferFrameCount == 0) {
            log("Haptics: WASAPI buffer size unavailable; retrying");
            teardownTransport(false);
            return false;
        }

        hr = audioClient->GetService(__uuidof(IAudioRenderClient),
            reinterpret_cast<void**>(renderClient.ReleaseAndGetAddressOf()));
        if (FAILED(hr) || !renderClient) {
            log("Haptics: WASAPI render client unavailable; retrying");
            teardownTransport(false);
            return false;
        }

        BYTE* initial = nullptr;
        hr = renderClient->GetBuffer(bufferFrameCount, &initial);
        if (FAILED(hr)) {
            log("Haptics: WASAPI initial buffer acquisition failed; retrying");
            teardownTransport(false);
            return false;
        }
        hr = renderClient->ReleaseBuffer(bufferFrameCount, AUDCLNT_BUFFERFLAGS_SILENT);
        if (FAILED(hr)) {
            log("Haptics: WASAPI initial silent buffer failed; retrying");
            teardownTransport(false);
            return false;
        }

        hr = audioClient->Start();
        if (FAILED(hr)) {
            log("Haptics: WASAPI start failed; retrying, HID features unaffected");
            teardownTransport(false);
            return false;
        }

        sampleFormat = selectedFormat;
        transportActive.store(true, std::memory_order_release);

        const auto name = utf8(candidate.friendlyName);
        const auto id = utf8(candidate.id);
        const auto format = sampleFormatName(sampleFormat);
        char details[768]{};
        std::snprintf(
            details, sizeof(details),
            "Haptics: endpoint active name='%s' id='%s' channels=4 rate=48000 format=%s",
            name.c_str(), id.c_str(), format.c_str());
        log(details);
        log("Haptics: ACTIVE transport=WASAPI-shared-event");
        return true;
    }

    void logDropSummary(std::chrono::steady_clock::time_point now) noexcept
    {
        if (!debugLogging || now < nextDropLog) {
            return;
        }
        const auto drops = commands.takeDropCounts();
        if (drops.overflow != 0 || drops.stale != 0) {
            char buffer[192]{};
            std::snprintf(
                buffer, sizeof(buffer), "Haptics: queue drops overflow=%llu stale=%llu",
                static_cast<unsigned long long>(drops.overflow),
                static_cast<unsigned long long>(drops.stale));
            log(buffer);
        }
        nextDropLog = now + kDropLogInterval;
    }

    bool renderOneBlock()
    {
        UINT32 padding = 0;
        HRESULT hr = audioClient->GetCurrentPadding(&padding);
        if (FAILED(hr)) {
            log("Haptics: render padding query failed/device invalidated; reconnecting");
            return false;
        }

        const UINT32 framesAvailable = bufferFrameCount > padding ? bufferFrameCount - padding : 0;
        if (framesAvailable == 0) {
            logDropSummary(std::chrono::steady_clock::now());
            return true;
        }

        const auto now = std::chrono::steady_clock::now();
        auto fresh = commands.drainFresh(now, kMaxCommandAge);
        for (auto& command : fresh) {
            if (isMeleeImpactHapticEffect(command.kind) || command.kind == HapticEffectKind::IncomingDamageImpact) {
                const auto kindName = hapticEffectKindName(command.kind);
                const auto ageUs = std::chrono::duration_cast<std::chrono::microseconds>(
                    now - command.when).count();
                char delivery[384]{};
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    command.kind == HapticEffectKind::IncomingDamageImpact ?
                        "Incoming damage delivery: stage=backend-drained eventWhenUs=%lld kind=%.*s gain=%.3f ageUs=%lld framesAvailable=%u" :
                        "Melee impact delivery: stage=backend-drained eventWhenUs=%lld kind=%.*s gain=%.3f ageUs=%lld framesAvailable=%u",
                    static_cast<long long>(hapticEventTimestampMicros(command.when)),
                    static_cast<int>(kindName.size()),
                    kindName.data(),
                    static_cast<double>(command.gain),
                    static_cast<long long>(ageUs),
                    static_cast<unsigned int>(framesAvailable));
                log(delivery);
            }
            hapticMixer.add(synthesizeHapticEffect(command, 48000));
        }
        hapticMixer.setContinuous(unpackHapticContinuousState(
            continuousPacked.load(std::memory_order_acquire)));
        logDropSummary(now);

        BYTE* raw = nullptr;
        hr = renderClient->GetBuffer(framesAvailable, &raw);
        if (FAILED(hr)) {
            log("Haptics: render buffer acquisition failed/device invalidated; reconnecting");
            return false;
        }

        std::vector<SpeakerCommand> freshSpeakerCommands;
        std::vector<PreparedSpeakerPcm> freshPreparedPcm;
        {
            std::scoped_lock speakerTransferLock(speakerQueueMutex, speakerMixerMutex);
            if (pendingPersistent) {
                if (pendingPersistent->kind == PersistentUpdateKind::Set) {
                    (void)speakerMixer.setPersistentPrepared(std::move(pendingPersistent->voice));
                } else {
                    (void)speakerMixer.clearPersistentPrepared(pendingPersistent->owner, pendingPersistent->force);
                }
                pendingPersistent.reset();
            }
            freshSpeakerCommands.assign(speakerCommands.begin(), speakerCommands.end());
            freshPreparedPcm.assign(preparedSpeakerPcm.begin(), preparedSpeakerPcm.end());
            speakerCommands.clear();
            preparedSpeakerPcm.clear();
            for (const auto& command : freshSpeakerCommands) {
                (void)speakerMixer.add(command);
            }
            for (const auto& pcm : freshPreparedPcm) {
                (void)speakerMixer.addPrepared(pcm);
            }
        }
        std::vector<StereoSpeakerFrame> speakerBlock(framesAvailable);
        bool speakerEmpty = true;
        {
            std::scoped_lock speakerMixerLock(speakerMixerMutex);
            speakerEmpty = speakerMixer.empty();
            if (!speakerEmpty) {
                speakerMixer.render(speakerBlock);
            }
        }
        {
            std::scoped_lock musicLock(musicQueueMutex);
            for (auto& command : musicCommands) {
                switch (command.kind) {
                case MusicControlKind::Add:
                    (void)musicMixer.add(std::move(command.voice));
                    break;
                case MusicControlKind::StopPlayingId:
                    (void)musicMixer.stopPlayingId(command.playingId);
                    break;
                case MusicControlKind::Clear:
                    musicMixer.clear();
                    break;
                }
            }
            musicCommands.clear();
        }

        if (hapticMixer.empty() && speakerEmpty && musicMixer.empty()) {
            hr = renderClient->ReleaseBuffer(framesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
            if (FAILED(hr)) {
                log("Haptics: silent render buffer release failed/device invalidated; reconnecting");
                return false;
            }
            return true;
        }

        std::vector<HapticFrame> hapticBlock(framesAvailable);
        hapticMixer.render(hapticBlock);
        if (!musicMixer.empty()) {
            std::vector<HapticFrame> musicBlock(framesAvailable);
            musicMixer.render(musicBlock);
            mixMusicUnderGameplay(hapticBlock, musicBlock);
        }
        std::vector<DualSenseAudioFrame> block(framesAvailable);
        composeDualSenseAudioFrames(speakerBlock, hapticBlock, block);

        switch (sampleFormat) {
        case HapticSampleFormat::Float32: {
            auto* output = reinterpret_cast<float*>(raw);
            for (UINT32 i = 0; i < framesAvailable; ++i) {
                output[i * 4 + 0] = block[i].ch1;
                output[i * 4 + 1] = block[i].ch2;
                output[i * 4 + 2] = block[i].ch3;
                output[i * 4 + 3] = block[i].ch4;
            }
            break;
        }
        case HapticSampleFormat::Pcm16: {
            auto* output = reinterpret_cast<std::int16_t*>(raw);
            for (UINT32 i = 0; i < framesAvailable; ++i) {
                output[i * 4 + 0] = static_cast<std::int16_t>(std::lround(block[i].ch1 * 32767.0F));
                output[i * 4 + 1] = static_cast<std::int16_t>(std::lround(block[i].ch2 * 32767.0F));
                output[i * 4 + 2] = static_cast<std::int16_t>(std::lround(block[i].ch3 * 32767.0F));
                output[i * 4 + 3] = static_cast<std::int16_t>(std::lround(block[i].ch4 * 32767.0F));
            }
            break;
        }
        case HapticSampleFormat::Pcm32: {
            auto* output = reinterpret_cast<std::int32_t*>(raw);
            constexpr double scale = 2147483647.0;
            for (UINT32 i = 0; i < framesAvailable; ++i) {
                output[i * 4 + 0] = static_cast<std::int32_t>(std::llround(static_cast<double>(block[i].ch1) * scale));
                output[i * 4 + 1] = static_cast<std::int32_t>(std::llround(static_cast<double>(block[i].ch2) * scale));
                output[i * 4 + 2] = static_cast<std::int32_t>(std::llround(static_cast<double>(block[i].ch3) * scale));
                output[i * 4 + 3] = static_cast<std::int32_t>(std::llround(static_cast<double>(block[i].ch4) * scale));
            }
            break;
        }
        default:
            (void)renderClient->ReleaseBuffer(framesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
            log("Haptics: unsupported render encoding encountered; reconnecting");
            return false;
        }

        hr = renderClient->ReleaseBuffer(framesAvailable, 0);
        if (FAILED(hr)) {
            log("Haptics: render buffer release failed/device invalidated; reconnecting");
            return false;
        }
        if (std::any_of(fresh.begin(), fresh.end(), [](const HapticCommand& command) {
                return isMeleeImpactHapticEffect(command.kind) || command.kind == HapticEffectKind::IncomingDamageImpact;
            })) {
            const auto stats = measureHapticBlock(hapticBlock);
            const auto format = sampleFormatName(sampleFormat);
            for (const auto& command : fresh) {
                if (!isMeleeImpactHapticEffect(command.kind) && command.kind != HapticEffectKind::IncomingDamageImpact) {
                    continue;
                }
                const auto kindName = hapticEffectKindName(command.kind);
                char delivery[640]{};
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    command.kind == HapticEffectKind::IncomingDamageImpact ?
                        "Incoming damage delivery: stage=backend-output-buffer eventWhenUs=%lld kind=%.*s gain=%.3f format=%s frames=%u peakCh3=%.6f rmsCh3=%.6f peakCh4=%.6f rmsCh4=%.6f nonZeroFrames=%zu firstNonZeroCh3=%.6f firstNonZeroCh4=%.6f" :
                        "Melee impact delivery: stage=backend-output-buffer eventWhenUs=%lld kind=%.*s gain=%.3f format=%s frames=%u peakCh3=%.6f rmsCh3=%.6f peakCh4=%.6f rmsCh4=%.6f nonZeroFrames=%zu firstNonZeroCh3=%.6f firstNonZeroCh4=%.6f",
                    static_cast<long long>(hapticEventTimestampMicros(command.when)),
                    static_cast<int>(kindName.size()),
                    kindName.data(),
                    static_cast<double>(command.gain),
                    format.c_str(),
                    static_cast<unsigned int>(framesAvailable),
                    static_cast<double>(stats.peakCh3),
                    static_cast<double>(stats.rmsCh3),
                    static_cast<double>(stats.peakCh4),
                    static_cast<double>(stats.rmsCh4),
                    stats.nonZeroFrames,
                    static_cast<double>(stats.firstNonZeroCh3),
                    static_cast<double>(stats.firstNonZeroCh4));
                log(delivery);
            }
        }

        for (const auto& command : fresh) {
            if (!isMeleeImpactHapticEffect(command.kind) && command.kind != HapticEffectKind::IncomingDamageImpact) {
                continue;
            }
            const auto kindName = hapticEffectKindName(command.kind);
            char delivery[320]{};
            std::snprintf(
                delivery,
                sizeof(delivery),
                command.kind == HapticEffectKind::IncomingDamageImpact ?
                    "Incoming damage delivery: stage=backend-rendered eventWhenUs=%lld kind=%.*s gain=%.3f frames=%u" :
                    "Melee impact delivery: stage=backend-rendered eventWhenUs=%lld kind=%.*s gain=%.3f frames=%u",
                static_cast<long long>(hapticEventTimestampMicros(command.when)),
                static_cast<int>(kindName.size()),
                kindName.data(),
                static_cast<double>(command.gain),
                static_cast<unsigned int>(framesAvailable));
            log(delivery);
        }
        return true;
    }

    bool runActiveTransport()
    {
        HANDLE waits[2]{ stopEvent, renderEvent };
        for (;;) {
            const DWORD result = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
            if (result == WAIT_OBJECT_0) {
                return false;
            }
            if (result == WAIT_OBJECT_0 + 1) {
                if (!renderOneBlock()) {
                    return true;
                }
                continue;
            }
            log("Haptics: render wait failed; reconnecting");
            return true;
        }
    }

    void startWorker()
    {
        bool expected = false;
        if (!running.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            return;
        }

        commands.reset();
        clearMusicState();
        continuousPacked.store(0U, std::memory_order_release);
        stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!stopEvent) {
            running.store(false, std::memory_order_release);
            throw std::runtime_error("DualSense audio stop event creation failed");
        }

        try {
            worker = std::thread([this] { threadMain(); });
        } catch (...) {
            CloseHandle(stopEvent);
            stopEvent = nullptr;
            running.store(false, std::memory_order_release);
            throw;
        }
    }

    void stopWorker() noexcept
    {
        continuousPacked.store(0U, std::memory_order_release);
        if (stopEvent) {
            SetEvent(stopEvent);
        }
        commands.stop();
        clearSpeakerState();
        queueMusicClear();

        if (worker.joinable()) {
            try {
                worker.join();
            } catch (...) {
            }
        }

        if (stopEvent) {
            CloseHandle(stopEvent);
            stopEvent = nullptr;
        }
        transportActive.store(false, std::memory_order_release);
        running.store(false, std::memory_order_release);
    }

    void threadMain() noexcept
    {
        bool comInitialized = false;
        try {
            const HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (coHr != S_OK && coHr != S_FALSE) {
                log("Haptics: COM initialization failed; haptics disabled, HID features unaffected");
                running.store(false, std::memory_order_release);
                return;
            }
            comInitialized = true;
            nextDropLog = std::chrono::steady_clock::now() + kDropLogInterval;

            while (!shouldStop()) {
                log("Haptics: searching for wired DualSense 4ch/48k audio endpoint");
                auto candidates = enumerateCandidates();
                const auto selected = selectDualSenseHapticEndpoint(candidates);
                if (!selected) {
                    log("Haptics: endpoint unavailable; haptics disabled, HID features unaffected");
                    if (waitForRetry()) {
                        break;
                    }
                    continue;
                }

                std::size_t compatibleCount = 0;
                for (const auto& candidate : candidates) {
                    if (compatible(candidate)) {
                        ++compatibleCount;
                    }
                }
                const auto& candidate = candidates[*selected];
                if (compatibleCount > 1) {
                    const auto id = utf8(candidate.id);
                    std::string message = "Haptics: multiple compatible endpoints found; choosing deterministic endpoint id='";
                    message += id;
                    message += "'";
                    log(message);
                }

                if (!initializeSelected(candidate)) {
                    if (waitForRetry()) {
                        break;
                    }
                    continue;
                }

                const bool reconnect = runActiveTransport();
                teardownTransport(reconnect);
                if (!reconnect || shouldStop()) {
                    break;
                }
                if (waitForRetry()) {
                    break;
                }
            }
        } catch (...) {
            log("Haptics: audio worker exception contained; HID features unaffected");
        }

        teardownTransport(false);
        enumerator.Reset();
        transportActive.store(false, std::memory_order_release);
        if (comInitialized) {
            CoUninitialize();
        }
        running.store(false, std::memory_order_release);
    }
};

sds::DualSenseAudioTransport::DualSenseAudioTransport(
    LogCallback log,
    bool debugLogging,
    float speakerVolume,
    std::chrono::milliseconds reconnectInterval) :
    _impl(std::make_unique<Impl>(std::move(log), debugLogging, speakerVolume, reconnectInterval))
{}

sds::DualSenseAudioTransport::~DualSenseAudioTransport()
{
    if (!_impl) {
        return;
    }
    std::scoped_lock lock(_impl->clientMutex);
    _impl->clients.stopHaptics();
    _impl->clients.stopSpeaker();
    _impl->stopWorker();
}

void sds::DualSenseAudioTransport::startHapticsClient()
{
    if (!_impl) {
        return;
    }
    std::scoped_lock lock(_impl->clientMutex);
    const bool wasRunning = _impl->clients.transportShouldRun();
    _impl->clients.startHaptics();
    if (!wasRunning) {
        try {
            _impl->startWorker();
        } catch (...) {
            _impl->clients.stopHaptics();
            throw;
        }
    }
}

void sds::DualSenseAudioTransport::stopHapticsClient() noexcept
{
    if (!_impl) {
        return;
    }
    std::scoped_lock lock(_impl->clientMutex);
    _impl->queueMusicClear();
    _impl->clients.stopHaptics();
    _impl->continuousPacked.store(0U, std::memory_order_release);
    if (!_impl->clients.transportShouldRun()) {
        _impl->stopWorker();
    }
}

void sds::DualSenseAudioTransport::startSpeakerClient()
{
    if (!_impl) {
        return;
    }
    std::scoped_lock lock(_impl->clientMutex);
    const bool wasRunning = _impl->clients.transportShouldRun();
    _impl->clients.startSpeaker();
    if (!wasRunning) {
        try {
            _impl->startWorker();
        } catch (...) {
            _impl->clients.stopSpeaker();
            throw;
        }
    }
}

void sds::DualSenseAudioTransport::stopSpeakerClient() noexcept
{
    if (!_impl) {
        return;
    }
    bool transitioned = false;
    {
        std::scoped_lock lock(_impl->clientMutex);
        transitioned = _impl->clients.speakerActive();
        _impl->clients.stopSpeaker();
        _impl->clearSpeakerState();
        if (!_impl->clients.transportShouldRun()) {
            _impl->stopWorker();
        }
    }
    if (transitioned) {
        _impl->reportPersistentInvalidation(SpeakerPersistentInvalidationReason::BackendStop);
    }
}

bool sds::DualSenseAudioTransport::enqueueHaptic(HapticCommand command) noexcept
{
    if (!_impl) {
        return false;
    }
    {
        std::scoped_lock lock(_impl->clientMutex);
        if (!_impl->clients.hapticsActive()) {
            return false;
        }
    }

    const auto kind = command.kind;
    const auto gain = command.gain;
    const auto when = command.when;
    const bool accepted = _impl->commands.push(std::move(command));
    if (isMeleeImpactHapticEffect(kind) || kind == HapticEffectKind::IncomingDamageImpact) {
        const auto kindName = hapticEffectKindName(kind);
        char delivery[320]{};
        std::snprintf(
            delivery,
            sizeof(delivery),
            kind == HapticEffectKind::IncomingDamageImpact ?
                (accepted ?
                    "Incoming damage delivery: stage=backend-enqueued eventWhenUs=%lld kind=%.*s gain=%.3f" :
                    "Incoming damage delivery: stage=backend-enqueue-rejected eventWhenUs=%lld kind=%.*s gain=%.3f") :
                (accepted ?
                    "Melee impact delivery: stage=backend-enqueued eventWhenUs=%lld kind=%.*s gain=%.3f" :
                    "Melee impact delivery: stage=backend-enqueue-rejected eventWhenUs=%lld kind=%.*s gain=%.3f"),
            static_cast<long long>(hapticEventTimestampMicros(when)),
            static_cast<int>(kindName.size()),
            kindName.data(),
            static_cast<double>(gain));
        _impl->log(delivery);
    }
    return accepted;
}

bool sds::DualSenseAudioTransport::setContinuous(HapticContinuousState state) noexcept
{
    if (!_impl || !_impl->running.load(std::memory_order_acquire)) {
        return false;
    }
    {
        std::scoped_lock lock(_impl->clientMutex);
        if (!_impl->clients.hapticsActive()) {
            return false;
        }
    }
    _impl->continuousPacked.store(packHapticContinuousState(state), std::memory_order_release);
    return true;
}

bool sds::DualSenseAudioTransport::enqueueMusicHaptic(MusicHapticVoice voice) noexcept
{
    if (!_impl || !voice.pcm || voice.pcm->frames.empty()) {
        return false;
    }
    try {
        {
            std::scoped_lock lock(_impl->clientMutex);
            if (!_impl->clients.hapticsActive()) {
                return false;
            }
        }
        std::scoped_lock musicLock(_impl->musicQueueMutex);
        if (_impl->musicCommands.size() >= kMaxMusicControlCommands) {
            return false;
        }
        Impl::MusicControlCommand command{};
        command.kind = Impl::MusicControlKind::Add;
        command.playingId = voice.playingId;
        command.voice = std::move(voice);
        _impl->musicCommands.push_back(std::move(command));
        return true;
    } catch (...) {
        return false;
    }
}

bool sds::DualSenseAudioTransport::stopMusicHapticsPlayingId(std::uint32_t playingId) noexcept
{
    if (!_impl || playingId == 0u) {
        return false;
    }
    try {
        std::scoped_lock musicLock(_impl->musicQueueMutex);
        std::erase_if(_impl->musicCommands, [playingId](const Impl::MusicControlCommand& command) {
            return command.kind == Impl::MusicControlKind::Add && command.playingId == playingId;
        });
        if (_impl->musicCommands.size() >= kMaxMusicControlCommands) {
            const auto removable = std::find_if(
                _impl->musicCommands.begin(),
                _impl->musicCommands.end(),
                [](const Impl::MusicControlCommand& command) {
                    return command.kind == Impl::MusicControlKind::Add;
                });
            if (removable == _impl->musicCommands.end()) {
                return false;
            }
            _impl->musicCommands.erase(removable);
        }
        Impl::MusicControlCommand command{};
        command.kind = Impl::MusicControlKind::StopPlayingId;
        command.playingId = playingId;
        _impl->musicCommands.push_back(std::move(command));
        return true;
    } catch (...) {
        return false;
    }
}

void sds::DualSenseAudioTransport::clearMusicHaptics() noexcept
{
    if (_impl) {
        _impl->queueMusicClear();
    }
}

bool sds::DualSenseAudioTransport::enqueueSpeaker(const SpeakerCommand& command) noexcept
{
    if (!_impl) {
        return false;
    }
    try {
        {
            std::scoped_lock lock(_impl->clientMutex);
            if (!_impl->clients.speakerActive()) {
                return false;
            }
        }
        std::scoped_lock lock(_impl->speakerQueueMutex);
        if (_impl->speakerCommands.size() >= kMaxPreparedSpeakerChunks) {
            return false;
        }
        _impl->speakerCommands.push_back(command);
        return true;
    } catch (...) {
        return false;
    }
}

bool sds::DualSenseAudioTransport::enqueuePreparedPcm(const PreparedSpeakerPcm& pcm) noexcept
{
    if (!_impl || pcm.frames.empty()) {
        return false;
    }
    try {
        {
            std::scoped_lock lock(_impl->clientMutex);
            if (!_impl->clients.speakerActive()) {
                return false;
            }
        }
        std::scoped_lock lock(_impl->speakerQueueMutex);
        if (_impl->preparedSpeakerPcm.size() >= kMaxPreparedSpeakerChunks) {
            return false;
        }
        _impl->preparedSpeakerPcm.push_back(pcm);
        return true;
    } catch (...) {
        return false;
    }
}

bool sds::DualSenseAudioTransport::replacePreparedPcm(PreparedSpeakerPcm pcm) noexcept
{
    if (!_impl || pcm.frames.empty()) {
        return false;
    }
    try {
        {
            std::scoped_lock lock(_impl->clientMutex);
            if (!_impl->clients.speakerActive()) {
                return false;
            }
        }
        std::scoped_lock lock(_impl->speakerQueueMutex, _impl->speakerMixerMutex);
        _impl->preparedSpeakerPcm.clear();
        _impl->speakerMixer.clearPrepared();
        _impl->preparedSpeakerPcm.push_back(std::move(pcm));
        return true;
    } catch (...) {
        return false;
    }
}

bool sds::DualSenseAudioTransport::setPersistentPreparedPcm(PersistentPreparedSpeakerPcm voice) noexcept
{
    if (!_impl || voice.owner == 0u) {
        return false;
    }
    if (voice.layers.empty()) {
        if (!voice.pcm || voice.pcm->frames.empty() || voice.loopResumeFrame >= voice.pcm->frames.size()) {
            return false;
        }
    } else {
        for (const auto& layer : voice.layers) {
            if (!layer.pcm || layer.pcm->frames.empty() || layer.loopResumeFrame >= layer.pcm->frames.size()) {
                return false;
            }
        }
    }
    try {
        {
            std::scoped_lock lock(_impl->clientMutex);
            if (!_impl->clients.speakerActive() || !_impl->transportActive.load(std::memory_order_acquire)) {
                return false;
            }
        }
        std::scoped_lock lock(_impl->speakerQueueMutex);
        Impl::PersistentUpdate update{};
        update.kind = Impl::PersistentUpdateKind::Set;
        update.owner = voice.owner;
        update.voice = std::move(voice);
        _impl->pendingPersistent = std::move(update);
        return true;
    } catch (...) {
        return false;
    }
}

void sds::DualSenseAudioTransport::clearSpeakerPlayback() noexcept
{
    if (!_impl) {
        return;
    }

    _impl->clearSpeakerState();
    _impl->reportPersistentInvalidation(SpeakerPersistentInvalidationReason::BackendStop);
}

bool sds::DualSenseAudioTransport::clearPersistentPreparedPcm(std::uint64_t owner, bool force) noexcept
{
    if (!_impl || (!force && owner == 0u)) {
        return false;
    }
    try {
        {
            std::scoped_lock lock(_impl->clientMutex);
            if (!_impl->clients.speakerActive()) {
                return false;
            }
        }
        std::scoped_lock lock(_impl->speakerQueueMutex);
        Impl::PersistentUpdate update{};
        update.kind = Impl::PersistentUpdateKind::Clear;
        update.owner = owner;
        update.force = force;
        _impl->pendingPersistent = std::move(update);
        return true;
    } catch (...) {
        return false;
    }
}

void sds::DualSenseAudioTransport::setSpeakerVolume(float volume) noexcept
{
    if (!_impl) {
        return;
    }
    std::scoped_lock lock(_impl->speakerMixerMutex);
    _impl->speakerMixer.setGlobalVolume(std::clamp(volume, 0.0F, 1.0F));
}

void sds::DualSenseAudioTransport::setSpeakerPersistentInvalidationCallback(
    SpeakerPersistentInvalidationCallback callback)
{
    if (!_impl) {
        return;
    }
    std::scoped_lock lock(_impl->speakerPersistentCallbackMutex);
    _impl->speakerPersistentInvalidationCallback = std::move(callback);
}

bool sds::DualSenseAudioTransport::active() const noexcept
{
    return _impl && _impl->transportActive.load(std::memory_order_acquire);
}

bool sds::DualSenseAudioTransport::hapticsClientStarted() const noexcept
{
    if (!_impl) {
        return false;
    }
    std::scoped_lock lock(_impl->clientMutex);
    return _impl->clients.hapticsActive();
}

bool sds::DualSenseAudioTransport::speakerClientStarted() const noexcept
{
    if (!_impl) {
        return false;
    }
    std::scoped_lock lock(_impl->clientMutex);
    return _impl->clients.speakerActive();
}

#if defined(SDS_TESTING)
void sds::DualSenseAudioTransport::testSetHapticsClientActive(bool active) noexcept
{
    if (!_impl) {
        return;
    }

    std::scoped_lock lock(_impl->clientMutex);
    if (active) {
        _impl->clients.startHaptics();
    } else {
        _impl->clients.stopHaptics();
    }
}

void sds::DualSenseAudioTransport::testSetSpeakerClientActive(bool active) noexcept
{
    if (!_impl) {
        return;
    }
    std::scoped_lock lock(_impl->clientMutex);
    if (active) {
        _impl->clients.startSpeaker();
    } else {
        _impl->clients.stopSpeaker();
    }
}

void sds::DualSenseAudioTransport::testSetTransportActive(bool active) noexcept
{
    if (_impl) {
        _impl->transportActive.store(active, std::memory_order_release);
    }
}

bool sds::DualSenseAudioTransport::testApplyPendingPersistentUpdate() noexcept
{
    if (!_impl) {
        return false;
    }
    std::scoped_lock lock(_impl->speakerQueueMutex, _impl->speakerMixerMutex);
    if (!_impl->pendingPersistent) {
        return false;
    }
    if (_impl->pendingPersistent->kind == Impl::PersistentUpdateKind::Set) {
        (void)_impl->speakerMixer.setPersistentPrepared(std::move(_impl->pendingPersistent->voice));
    } else {
        (void)_impl->speakerMixer.clearPersistentPrepared(
            _impl->pendingPersistent->owner, _impl->pendingPersistent->force);
    }
    _impl->pendingPersistent.reset();
    return true;
}

std::uint64_t sds::DualSenseAudioTransport::testPersistentOwner() const noexcept
{
    if (!_impl) {
        return 0u;
    }
    std::scoped_lock lock(_impl->speakerMixerMutex);
    return _impl->speakerMixer.persistentOwner();
}

bool sds::DualSenseAudioTransport::testSpeakerStateEmpty() const noexcept
{
    if (!_impl) {
        return true;
    }

    std::scoped_lock lock(_impl->speakerQueueMutex, _impl->speakerMixerMutex);
    return !_impl->pendingPersistent.has_value() &&
        _impl->speakerCommands.empty() &&
        _impl->preparedSpeakerPcm.empty() &&
        _impl->speakerMixer.empty();
}

void sds::DualSenseAudioTransport::testSimulateEndpointInvalidation() noexcept
{
    if (_impl) {
        _impl->teardownTransport(true);
    }
}
#endif
