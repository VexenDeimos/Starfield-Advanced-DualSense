#include <StarfieldDualSense/WwiseSecondaryOutputCanary.h>
#include <StarfieldDualSense/WwiseCanarySafety.h>
#include <StarfieldDualSense/WwiseWindowsDeviceId.h>

#include <RE/Starfield.h>
#include <REL/Relocation.h>

#include <Windows.h>
#include <wtypes.h>
#include <propkeydef.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <mmdeviceapi.h>
#include <propvarutil.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr std::uint64_t kAddOutputId = 150350;
    constexpr std::uint64_t kRemoveOutputId = 150406;
    constexpr std::uint64_t kRegisterGameObjId = 150401;
    constexpr std::uint64_t kSetListenersId = 150415;
    constexpr std::uint64_t kSetListenersCoreId = 150352;
    constexpr std::uint64_t kUnregisterGameObjId = 150436;

    constexpr std::uint32_t kAkSuccess = 1;
    constexpr std::uint64_t kCanaryListenerId = 0x5344534300000001ULL; // "SDSC" + 1
    constexpr std::uint64_t kCanaryEmitterId = 0x5344534300000002ULL;  // "SDSC" + 2

    struct WwiseOutputSettingsAbi
    {
        std::uint32_t audioDeviceShareset{ 0 }; // AK_INVALID_UNIQUE_ID -> default System device plug-in.
        std::uint32_t idDevice{ 0 };
        std::uint32_t ePanningRule{ 0 };       // AkPanningRule_Speakers.
        std::uint32_t channelConfig{ 0 };       // Clear/default endpoint configuration.
    };
    static_assert(sizeof(WwiseOutputSettingsAbi) == 16);

    using AddOutputFn = std::uint32_t (*)(
        const WwiseOutputSettingsAbi&,
        std::uint64_t*,
        const std::uint64_t*,
        std::uint32_t);
    using RemoveOutputFn = std::uint32_t (*)(std::uint64_t);
    using RegisterGameObjFn = std::uint32_t (*)(std::uint64_t);
    using SetListenersFn = std::uint32_t (*)(std::uint64_t, const std::uint64_t*, std::uint32_t);
    using UnregisterGameObjFn = std::uint32_t (*)(std::uint64_t);
    using GetIdFromStringFn = std::uint32_t (*)(const char*);

    template <class T>
    struct ComRelease
    {
        void operator()(T* value) const noexcept
        {
            if (value) {
                value->Release();
            }
        }
    };

    template <class T>
    using UniqueCom = std::unique_ptr<T, ComRelease<T>>;

    struct CoInitScope
    {
        HRESULT result{ E_FAIL };
        bool needsUninitialize{ false };

        CoInitScope() noexcept
        {
            result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            needsUninitialize = result == S_OK || result == S_FALSE;
        }

        ~CoInitScope()
        {
            if (needsUninitialize) {
                ::CoUninitialize();
            }
        }

        [[nodiscard]] bool usable() const noexcept
        {
            return SUCCEEDED(result) || result == RPC_E_CHANGED_MODE;
        }
    };

    struct RenderEndpoint
    {
        std::string friendlyName;
        std::string endpointId;
    };

    [[nodiscard]] std::uintptr_t resolveId(std::uint64_t id) noexcept
    {
        try {
            REL::Relocation<std::uintptr_t> relocation{ REL::ID(id) };
            return relocation.address();
        } catch (...) {
            return 0;
        }
    }

    [[nodiscard]] std::uintptr_t resolveGetIdFromString() noexcept
    {
        try {
            REL::Relocation<std::uintptr_t> relocation{ RE::ID::AkSoundEngine::GetIDFromString };
            return relocation.address();
        } catch (...) {
            return 0;
        }
    }

    [[nodiscard]] bool readCode(
        std::uintptr_t address,
        std::size_t count,
        std::vector<std::uint8_t>& out) noexcept
    {
        out.assign(count, 0);
        SIZE_T bytesRead = 0;
        if (!address || !count ||
            ::ReadProcessMemory(
                ::GetCurrentProcess(),
                reinterpret_cast<const void*>(address),
                out.data(),
                out.size(),
                &bytesRead) == FALSE ||
            bytesRead != out.size()) {
            out.clear();
            return false;
        }
        return true;
    }

    [[nodiscard]] std::string wideToUtf8(const wchar_t* value)
    {
        if (!value || !*value) {
            return {};
        }
        const int required = ::WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
        if (required <= 1) {
            return {};
        }
        std::string result(static_cast<std::size_t>(required), '\0');
        if (::WideCharToMultiByte(
                CP_UTF8,
                0,
                value,
                -1,
                result.data(),
                required,
                nullptr,
                nullptr) <= 0) {
            return {};
        }
        result.pop_back();
        return result;
    }

    [[nodiscard]] bool enumerateRenderEndpoints(std::vector<RenderEndpoint>& out) noexcept
    {
        try {
            out.clear();
            CoInitScope coInit;
            if (!coInit.usable()) {
                return false;
            }

            IMMDeviceEnumerator* rawEnumerator = nullptr;
            const auto createResult = ::CoCreateInstance(
                __uuidof(MMDeviceEnumerator),
                nullptr,
                CLSCTX_ALL,
                __uuidof(IMMDeviceEnumerator),
                reinterpret_cast<void**>(&rawEnumerator));
            UniqueCom<IMMDeviceEnumerator> enumerator(rawEnumerator);
            if (FAILED(createResult) || !enumerator) {
                return false;
            }

            IMMDeviceCollection* rawCollection = nullptr;
            const auto enumResult = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &rawCollection);
            UniqueCom<IMMDeviceCollection> collection(rawCollection);
            if (FAILED(enumResult) || !collection) {
                return false;
            }

            UINT count = 0;
            if (FAILED(collection->GetCount(&count))) {
                return false;
            }

            out.reserve(count);
            for (UINT index = 0; index < count; ++index) {
                IMMDevice* rawDevice = nullptr;
                if (FAILED(collection->Item(index, &rawDevice)) || !rawDevice) {
                    continue;
                }
                UniqueCom<IMMDevice> device(rawDevice);

                LPWSTR rawEndpointId = nullptr;
                if (FAILED(device->GetId(&rawEndpointId)) || !rawEndpointId) {
                    continue;
                }
                const auto endpointId = wideToUtf8(rawEndpointId);
                ::CoTaskMemFree(rawEndpointId);
                if (endpointId.empty()) {
                    continue;
                }

                std::string friendlyName;
                IPropertyStore* rawProperties = nullptr;
                if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &rawProperties)) && rawProperties) {
                    UniqueCom<IPropertyStore> properties(rawProperties);
                    PROPVARIANT value{};
                    ::PropVariantInit(&value);
                    if (SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName, &value)) &&
                        value.vt == VT_LPWSTR && value.pwszVal) {
                        friendlyName = wideToUtf8(value.pwszVal);
                    }
                    ::PropVariantClear(&value);
                }

                out.push_back({ std::move(friendlyName), endpointId });
            }
            return !out.empty();
        } catch (...) {
            out.clear();
            return false;
        }
    }

    [[nodiscard]] std::string formatHex64(std::uint64_t value)
    {
        std::ostringstream out;
        out << "0x" << std::hex << std::uppercase << value;
        return out.str();
    }

    [[nodiscard]] std::string formatHex32(std::uint32_t value)
    {
        std::ostringstream out;
        out << "0x" << std::hex << std::uppercase << value;
        return out.str();
    }
}

sds::WwiseSecondaryOutputCanary::WwiseSecondaryOutputCanary(WwiseSecondaryOutputCanaryLog log) :
    log_(std::move(log))
{}

sds::WwiseSecondaryOutputCanary::~WwiseSecondaryOutputCanary()
{
    stop();
}

std::uint64_t sds::WwiseSecondaryOutputCanary::emitterGameObjectId() const noexcept
{
    return active_ ? kCanaryEmitterId : 0;
}

bool sds::WwiseSecondaryOutputCanary::start() noexcept
{
    try {
        if (active_ || outputAdded_ || listenerRegistered_ || emitterRegistered_) {
            return active_;
        }
        if (!log_) {
            return false;
        }

        entryPoints_.addOutput = resolveId(kAddOutputId);
        entryPoints_.removeOutput = resolveId(kRemoveOutputId);
        entryPoints_.registerGameObj = resolveId(kRegisterGameObjId);
        entryPoints_.setListeners = resolveId(kSetListenersId);
        entryPoints_.setListenersCore = resolveId(kSetListenersCoreId);
        entryPoints_.unregisterGameObj = resolveId(kUnregisterGameObjId);
        entryPoints_.getIdFromString = resolveGetIdFromString();

        if (!entryPoints_.addOutput || !entryPoints_.removeOutput || !entryPoints_.registerGameObj ||
            !entryPoints_.setListeners || !entryPoints_.setListenersCore || !entryPoints_.unregisterGameObj ||
            !entryPoints_.getIdFromString) {
            log_("Wwise canary: FAIL one or more Address Library entry points did not resolve; no Wwise calls made");
            return false;
        }

        std::vector<std::uint8_t> addCode;
        std::vector<std::uint8_t> removeCode;
        std::vector<std::uint8_t> registerCode;
        std::vector<std::uint8_t> unregisterCode;
        std::vector<std::uint8_t> setListenersCode;
        if (!readCode(entryPoints_.addOutput, 384, addCode) ||
            !readCode(entryPoints_.removeOutput, 80, removeCode) ||
            !readCode(entryPoints_.registerGameObj, 96, registerCode) ||
            !readCode(entryPoints_.unregisterGameObj, 96, unregisterCode) ||
            !readCode(entryPoints_.setListeners, 16, setListenersCode) ||
            !matchesWwiseCanarySignature(WwiseCanaryApi::AddOutput, addCode) ||
            !matchesWwiseCanarySignature(WwiseCanaryApi::RemoveOutput, removeCode) ||
            !matchesWwiseCanarySignature(WwiseCanaryApi::RegisterGameObj, registerCode) ||
            !matchesWwiseCanarySignature(WwiseCanaryApi::UnregisterGameObj, unregisterCode) ||
            !matchesSetListenersWrapper(setListenersCode, entryPoints_.setListeners, entryPoints_.setListenersCore)) {
            log_("Wwise canary: FAIL runtime ABI signature guard rejected the identified functions; no Wwise calls made");
            return false;
        }

        log_("Wwise canary: ABI guard PASS AddOutput=150350 RemoveOutput=150406 RegisterGameObj=150401 SetListeners=150415->150352 UnregisterGameObj=150436; canary setup posts no event");

        std::vector<RenderEndpoint> endpoints;
        if (!enumerateRenderEndpoints(endpoints)) {
            log_("Wwise canary: FAIL active Windows render endpoints unavailable; no Wwise calls made");
            return false;
        }

        std::vector<std::string_view> friendlyNames;
        friendlyNames.reserve(endpoints.size());
        for (const auto& endpoint : endpoints) {
            friendlyNames.emplace_back(endpoint.friendlyName);
        }
        const auto selected = selectDualSenseRenderEndpoint(friendlyNames);
        if (selected == kNoEndpointIndex || selected >= endpoints.size()) {
            log_("Wwise canary: FAIL no active DualSense render endpoint found; no Wwise calls made");
            return false;
        }

        const auto& endpoint = endpoints[selected];
        const auto getIdFromString = reinterpret_cast<GetIdFromStringFn>(entryPoints_.getIdFromString);
        const auto currentHashId = getIdFromString(endpoint.endpointId.c_str());
        const auto documentedFnvId = computeWwiseWindowsDeviceId(endpoint.endpointId);
        const bool deviceIdMatch = currentHashId != 0 && currentHashId == documentedFnvId;

        {
            std::ostringstream message;
            message << "Wwise canary: device-ID VERIFY endpoint='" << endpoint.endpointId
                    << "' currentHashId=" << formatHex32(currentHashId)
                    << " documentedFnvId=" << formatHex32(documentedFnvId)
                    << " match=" << (deviceIdMatch ? "yes" : "no");
            log_(message.str());
        }
        if (!deviceIdMatch) {
            log_("Wwise canary: FAIL Windows device-ID verification mismatch; no secondary-output Wwise calls made");
            return false;
        }
        endpointDeviceId_ = documentedFnvId;

        {
            std::ostringstream message;
            message << "Wwise canary: endpoint selected name='" << endpoint.friendlyName
                    << "' wwiseDeviceId=" << formatHex32(endpointDeviceId_)
                    << " listener=" << formatHex64(kCanaryListenerId)
                    << " emitter=" << formatHex64(kCanaryEmitterId);
            log_(message.str());
        }

        const auto registerGameObj = reinterpret_cast<RegisterGameObjFn>(entryPoints_.registerGameObj);
        const auto addOutput = reinterpret_cast<AddOutputFn>(entryPoints_.addOutput);
        const auto setListeners = reinterpret_cast<SetListenersFn>(entryPoints_.setListeners);
        const auto setPosition = [](std::uint64_t gameObjectId, const RE::BGSAudio::AkSoundPosition& position) {
            return RE::BGSAudio::AkSoundEngine::SetPosition(gameObjectId, position);
        };

        auto result = registerGameObj(kCanaryListenerId);
        {
            std::ostringstream message;
            message << "Wwise canary: RegisterGameObj(listener) result=" << result;
            log_(message.str());
        }
        if (result != kAkSuccess) {
            return false;
        }
        listenerRegistered_ = true;

        result = registerGameObj(kCanaryEmitterId);
        {
            std::ostringstream message;
            message << "Wwise canary: RegisterGameObj(emitter) result=" << result;
            log_(message.str());
        }
        if (result != kAkSuccess) {
            stop();
            return false;
        }
        emitterRegistered_ = true;

        const RE::BGSAudio::AkSoundPosition canaryPosition{
            .orientationFront = { 0.0F, 0.0F, 1.0F },
            .orientationTop = { 0.0F, 1.0F, 0.0F },
            .position = { 0.0F, 0.0F, 0.0F },
        };

        result = setPosition(kCanaryListenerId, canaryPosition);
        {
            std::ostringstream message;
            message << "Wwise canary: SetPosition(listener) result=" << result
                    << " pos=0,0,0 front=0,0,1 top=0,1,0";
            log_(message.str());
        }
        if (result != kAkSuccess) {
            stop();
            return false;
        }

        result = setPosition(kCanaryEmitterId, canaryPosition);
        {
            std::ostringstream message;
            message << "Wwise canary: SetPosition(emitter) result=" << result
                    << " pos=0,0,0 front=0,0,1 top=0,1,0";
            log_(message.str());
        }
        if (result != kAkSuccess) {
            stop();
            return false;
        }

        WwiseOutputSettingsAbi settings{};
        settings.idDevice = endpointDeviceId_;
        const auto expectedOutputId = packWwiseOutputDeviceId(settings.audioDeviceShareset, settings.idDevice);
        result = addOutput(settings, &outputDeviceId_, &kCanaryListenerId, 1);
        outputAdded_ = result == kAkSuccess;
        {
            std::ostringstream message;
            message << "Wwise canary: AddOutput result=" << result
                    << " output=" << formatHex64(outputDeviceId_)
                    << " expected=" << formatHex64(expectedOutputId);
            log_(message.str());
        }
        if (result != kAkSuccess || outputDeviceId_ != expectedOutputId) {
            stop();
            return false;
        }

        result = setListeners(kCanaryEmitterId, &kCanaryListenerId, 1);
        {
            std::ostringstream message;
            message << "Wwise canary: SetListeners result=" << result;
            log_(message.str());
        }
        if (result != kAkSuccess) {
            stop();
            return false;
        }

        active_ = true;
        log_("Wwise canary: PASS isolated secondary output/listener/emitter active; positioned=yes zeroDistance=yes; postedEvents=0 during canary setup; normal Starfield audio routing unchanged");
        return true;
    } catch (...) {
        if (log_) {
            log_("Wwise canary: FAIL exception while preparing silent secondary output; tearing down partial state");
        }
        stop();
        return false;
    }
}

void sds::WwiseSecondaryOutputCanary::stop() noexcept
{
    try {
        if (!outputAdded_ && !listenerRegistered_ && !emitterRegistered_) {
            active_ = false;
            return;
        }

        active_ = false;
        const auto removeOutput = entryPoints_.removeOutput ?
            reinterpret_cast<RemoveOutputFn>(entryPoints_.removeOutput) : nullptr;
        const auto unregisterGameObj = entryPoints_.unregisterGameObj ?
            reinterpret_cast<UnregisterGameObjFn>(entryPoints_.unregisterGameObj) : nullptr;

        if (outputAdded_ && removeOutput) {
            const auto result = removeOutput(outputDeviceId_);
            if (log_) {
                std::ostringstream message;
                message << "Wwise canary: RemoveOutput result=" << result
                        << " output=" << formatHex64(outputDeviceId_);
                log_(message.str());
            }
        }
        outputAdded_ = false;
        outputDeviceId_ = 0;

        if (emitterRegistered_ && unregisterGameObj) {
            const auto result = unregisterGameObj(kCanaryEmitterId);
            if (log_) {
                std::ostringstream message;
                message << "Wwise canary: UnregisterGameObj(emitter) result=" << result;
                log_(message.str());
            }
        }
        emitterRegistered_ = false;

        if (listenerRegistered_ && unregisterGameObj) {
            const auto result = unregisterGameObj(kCanaryListenerId);
            if (log_) {
                std::ostringstream message;
                message << "Wwise canary: UnregisterGameObj(listener) result=" << result;
                log_(message.str());
            }
        }
        listenerRegistered_ = false;
        endpointDeviceId_ = 0;

        if (log_) {
            log_("Wwise canary: teardown complete");
        }
    } catch (...) {
        outputAdded_ = false;
        emitterRegistered_ = false;
        listenerRegistered_ = false;
        outputDeviceId_ = 0;
        endpointDeviceId_ = 0;
        active_ = false;
    }
}
