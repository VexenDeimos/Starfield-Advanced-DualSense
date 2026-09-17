#include <StarfieldDualSense/DualSenseAudioHapticsBackend.h>
#include <StarfieldDualSense/HapticCommandQueue.h>
#include <StarfieldDualSense/HapticEndpointSelection.h>
#include <StarfieldDualSense/HapticMixer.h>
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

struct sds::DualSenseAudioHapticsBackend::Impl
{
    LogCallback logCallback{};
    bool debugLogging{ false };
    std::chrono::milliseconds reconnectInterval{ 2000 };
    HapticCommandQueue commands{};
    HapticMixer mixer{};
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

    explicit Impl(LogCallback log, bool debug, std::chrono::milliseconds reconnect) :
        logCallback(std::move(log)),
        debugLogging(debug),
        reconnectInterval(reconnect)
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

    void teardownTransport() noexcept
    {
        transportActive.store(false, std::memory_order_release);
        if (audioClient) {
            (void)audioClient->Stop();
        }
        renderClient.Reset();
        audioClient.Reset();
        device.Reset();
        bufferFrameCount = 0;
        sampleFormat = HapticSampleFormat::Unsupported;
        closeRenderEvent();
        mixer = HapticMixer{};
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
        teardownTransport();
        if (!enumerator) {
            return false;
        }

        HRESULT hr = enumerator->GetDevice(candidate.id.c_str(), device.ReleaseAndGetAddressOf());
        if (FAILED(hr) || !device) {
            log("Haptics: selected endpoint disappeared before activation; retrying");
            teardownTransport();
            return false;
        }

        hr = device->Activate(
            __uuidof(IAudioClient), CLSCTX_ALL, nullptr,
            reinterpret_cast<void**>(audioClient.ReleaseAndGetAddressOf()));
        if (FAILED(hr) || !audioClient) {
            log("Haptics: selected endpoint audio client activation failed; retrying");
            teardownTransport();
            return false;
        }

        WAVEFORMATEX* mix = nullptr;
        hr = audioClient->GetMixFormat(&mix);
        if (FAILED(hr) || !mix) {
            if (mix) {
                CoTaskMemFree(mix);
            }
            log("Haptics: selected endpoint mix format unavailable; retrying");
            teardownTransport();
            return false;
        }

        const HapticSampleFormat selectedFormat = classifyFormat(mix);
        const bool valid = mix->nChannels == 4 && mix->nSamplesPerSec == 48000 &&
            selectedFormat != HapticSampleFormat::Unsupported;
        if (!valid) {
            CoTaskMemFree(mix);
            log("Haptics: selected endpoint format changed and is no longer supported 4ch/48k; retrying");
            teardownTransport();
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
            teardownTransport();
            return false;
        }

        renderEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!renderEvent) {
            log("Haptics: WASAPI render event creation failed; retrying");
            teardownTransport();
            return false;
        }

        hr = audioClient->SetEventHandle(renderEvent);
        if (FAILED(hr)) {
            log("Haptics: WASAPI render event binding failed; retrying");
            teardownTransport();
            return false;
        }

        hr = audioClient->GetBufferSize(&bufferFrameCount);
        if (FAILED(hr) || bufferFrameCount == 0) {
            log("Haptics: WASAPI buffer size unavailable; retrying");
            teardownTransport();
            return false;
        }

        hr = audioClient->GetService(__uuidof(IAudioRenderClient),
            reinterpret_cast<void**>(renderClient.ReleaseAndGetAddressOf()));
        if (FAILED(hr) || !renderClient) {
            log("Haptics: WASAPI render client unavailable; retrying");
            teardownTransport();
            return false;
        }

        BYTE* initial = nullptr;
        hr = renderClient->GetBuffer(bufferFrameCount, &initial);
        if (FAILED(hr)) {
            log("Haptics: WASAPI initial buffer acquisition failed; retrying");
            teardownTransport();
            return false;
        }
        hr = renderClient->ReleaseBuffer(bufferFrameCount, AUDCLNT_BUFFERFLAGS_SILENT);
        if (FAILED(hr)) {
            log("Haptics: WASAPI initial silent buffer failed; retrying");
            teardownTransport();
            return false;
        }

        hr = audioClient->Start();
        if (FAILED(hr)) {
            log("Haptics: WASAPI start failed; retrying, HID features unaffected");
            teardownTransport();
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
            mixer.add(synthesizeHapticEffect(command, 48000));
        }
        mixer.setContinuous(unpackHapticContinuousState(
            continuousPacked.load(std::memory_order_acquire)));
        logDropSummary(now);

        BYTE* raw = nullptr;
        hr = renderClient->GetBuffer(framesAvailable, &raw);
        if (FAILED(hr)) {
            log("Haptics: render buffer acquisition failed/device invalidated; reconnecting");
            return false;
        }

        if (mixer.empty()) {
            hr = renderClient->ReleaseBuffer(framesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
            if (FAILED(hr)) {
                log("Haptics: silent render buffer release failed/device invalidated; reconnecting");
                return false;
            }
            return true;
        }

        std::vector<HapticFrame> block(framesAvailable);
        mixer.render(block);

        switch (sampleFormat) {
        case HapticSampleFormat::Float32: {
            auto* output = reinterpret_cast<float*>(raw);
            for (UINT32 i = 0; i < framesAvailable; ++i) {
                output[i * 4 + 0] = 0.0F;
                output[i * 4 + 1] = 0.0F;
                output[i * 4 + 2] = std::clamp(block[i][2], -1.0F, 1.0F);
                output[i * 4 + 3] = std::clamp(block[i][3], -1.0F, 1.0F);
            }
            break;
        }
        case HapticSampleFormat::Pcm16: {
            auto* output = reinterpret_cast<std::int16_t*>(raw);
            for (UINT32 i = 0; i < framesAvailable; ++i) {
                output[i * 4 + 0] = 0;
                output[i * 4 + 1] = 0;
                output[i * 4 + 2] = static_cast<std::int16_t>(std::lround(
                    std::clamp(block[i][2], -1.0F, 1.0F) * 32767.0F));
                output[i * 4 + 3] = static_cast<std::int16_t>(std::lround(
                    std::clamp(block[i][3], -1.0F, 1.0F) * 32767.0F));
            }
            break;
        }
        case HapticSampleFormat::Pcm32: {
            auto* output = reinterpret_cast<std::int32_t*>(raw);
            constexpr double scale = 2147483647.0;
            for (UINT32 i = 0; i < framesAvailable; ++i) {
                output[i * 4 + 0] = 0;
                output[i * 4 + 1] = 0;
                output[i * 4 + 2] = static_cast<std::int32_t>(std::llround(
                    static_cast<double>(std::clamp(block[i][2], -1.0F, 1.0F)) * scale));
                output[i * 4 + 3] = static_cast<std::int32_t>(std::llround(
                    static_cast<double>(std::clamp(block[i][3], -1.0F, 1.0F)) * scale));
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
            const auto stats = measureHapticBlock(block);
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
                teardownTransport();
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

        teardownTransport();
        enumerator.Reset();
        transportActive.store(false, std::memory_order_release);
        if (comInitialized) {
            CoUninitialize();
        }
        running.store(false, std::memory_order_release);
    }
};

sds::DualSenseAudioHapticsBackend::DualSenseAudioHapticsBackend(
    LogCallback log,
    bool debugLogging,
    std::chrono::milliseconds reconnectInterval) :
    _impl(std::make_unique<Impl>(std::move(log), debugLogging, reconnectInterval))
{}

sds::DualSenseAudioHapticsBackend::~DualSenseAudioHapticsBackend()
{
    stop();
}

void sds::DualSenseAudioHapticsBackend::start()
{
    if (!_impl) {
        return;
    }

    bool expected = false;
    if (!_impl->running.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }

    _impl->continuousPacked.store(0U, std::memory_order_release);
    _impl->stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!_impl->stopEvent) {
        _impl->running.store(false, std::memory_order_release);
        throw std::runtime_error("DualSense haptics stop event creation failed");
    }

    try {
        _impl->worker = std::thread([impl = _impl.get()] { impl->threadMain(); });
    } catch (...) {
        CloseHandle(_impl->stopEvent);
        _impl->stopEvent = nullptr;
        _impl->running.store(false, std::memory_order_release);
        throw;
    }
}

void sds::DualSenseAudioHapticsBackend::stop() noexcept
{
    if (!_impl) {
        return;
    }

    _impl->continuousPacked.store(0U, std::memory_order_release);
    if (_impl->stopEvent) {
        SetEvent(_impl->stopEvent);
    }
    _impl->commands.stop();

    if (_impl->worker.joinable()) {
        try {
            _impl->worker.join();
        } catch (...) {
        }
    }

    if (_impl->stopEvent) {
        CloseHandle(_impl->stopEvent);
        _impl->stopEvent = nullptr;
    }
    _impl->transportActive.store(false, std::memory_order_release);
    _impl->running.store(false, std::memory_order_release);
}

bool sds::DualSenseAudioHapticsBackend::enqueue(HapticCommand command) noexcept
{
    if (!_impl) {
        return false;
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

bool sds::DualSenseAudioHapticsBackend::setContinuous(HapticContinuousState state) noexcept
{
    if (!_impl || !_impl->running.load(std::memory_order_acquire)) {
        return false;
    }
    _impl->continuousPacked.store(
        packHapticContinuousState(state),
        std::memory_order_release);
    return true;
}

bool sds::DualSenseAudioHapticsBackend::active() const noexcept
{
    return _impl && _impl->transportActive.load(std::memory_order_acquire);
}
