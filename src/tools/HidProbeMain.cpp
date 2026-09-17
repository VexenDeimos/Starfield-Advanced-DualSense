#include <StarfieldDualSense/HidProbePackets.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    constexpr USHORT kSonyVendorId = 0x054C;
    constexpr USHORT kDualSensePid = 0x0CE6;
    constexpr USHORT kDualSenseEdgePid = 0x0DF2;

    struct DeviceHandle
    {
        HANDLE handle{ INVALID_HANDLE_VALUE };
        HIDP_CAPS caps{};
        HIDD_ATTRIBUTES attributes{};
        std::wstring path{};

        DeviceHandle() = default;
        DeviceHandle(const DeviceHandle&) = delete;
        DeviceHandle& operator=(const DeviceHandle&) = delete;

        DeviceHandle(DeviceHandle&& other) noexcept :
            handle(other.handle),
            caps(other.caps),
            attributes(other.attributes),
            path(std::move(other.path))
        {
            other.handle = INVALID_HANDLE_VALUE;
        }

        DeviceHandle& operator=(DeviceHandle&& other) noexcept
        {
            if (this != &other) {
                close();
                handle = other.handle;
                caps = other.caps;
                attributes = other.attributes;
                path = std::move(other.path);
                other.handle = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        ~DeviceHandle()
        {
            close();
        }

        void close() noexcept
        {
            if (handle != INVALID_HANDLE_VALUE) {
                CloseHandle(handle);
                handle = INVALID_HANDLE_VALUE;
            }
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return handle != INVALID_HANDLE_VALUE;
        }
    };

    bool processRunning(std::wstring_view executable)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return false;
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        bool found = false;
        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (_wcsicmp(entry.szExeFile, executable.data()) == 0) {
                    found = true;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
        return found;
    }

    std::optional<DeviceHandle> openDualSense()
    {
        GUID hidGuid{};
        HidD_GetHidGuid(&hidGuid);

        HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
            &hidGuid,
            nullptr,
            nullptr,
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (deviceInfoSet == INVALID_HANDLE_VALUE) {
            return std::nullopt;
        }

        std::optional<DeviceHandle> result;
        for (DWORD index = 0; !result; ++index) {
            SP_DEVICE_INTERFACE_DATA interfaceData{};
            interfaceData.cbSize = sizeof(interfaceData);
            if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &hidGuid, index, &interfaceData)) {
                if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                    break;
                }
                continue;
            }

            DWORD requiredSize = 0;
            SetupDiGetDeviceInterfaceDetailW(
                deviceInfoSet,
                &interfaceData,
                nullptr,
                0,
                &requiredSize,
                nullptr);
            if (requiredSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) {
                continue;
            }

            std::vector<std::byte> storage(requiredSize);
            auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(
                    deviceInfoSet,
                    &interfaceData,
                    detail,
                    requiredSize,
                    nullptr,
                    nullptr)) {
                continue;
            }

            HANDLE candidate = CreateFileW(
                detail->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);
            if (candidate == INVALID_HANDLE_VALUE) {
                continue;
            }

            HIDD_ATTRIBUTES attributes{};
            attributes.Size = sizeof(attributes);
            if (!HidD_GetAttributes(candidate, &attributes) ||
                attributes.VendorID != kSonyVendorId ||
                (attributes.ProductID != kDualSensePid && attributes.ProductID != kDualSenseEdgePid)) {
                CloseHandle(candidate);
                continue;
            }

            PHIDP_PREPARSED_DATA preparsed = nullptr;
            HIDP_CAPS caps{};
            if (!HidD_GetPreparsedData(candidate, &preparsed)) {
                CloseHandle(candidate);
                continue;
            }
            const bool capsOk = HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS;
            HidD_FreePreparsedData(preparsed);
            if (!capsOk || caps.OutputReportByteLength != sds::probe::kUsbReportSize) {
                CloseHandle(candidate);
                continue;
            }

            DeviceHandle device;
            device.handle = candidate;
            device.caps = caps;
            device.attributes = attributes;
            device.path = detail->DevicePath;
            result.emplace(std::move(device));
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return result;
    }

    bool sendReport(HANDLE handle, const sds::probe::UsbReport& report, std::string_view label)
    {
        std::cout << "\nTX " << label << "\n"
                  << sds::probe::describeReport(report) << "\n";

        DWORD bytesWritten = 0;
        const BOOL ok = WriteFile(
            handle,
            report.data(),
            static_cast<DWORD>(report.size()),
            &bytesWritten,
            nullptr);
        if (!ok || bytesWritten != report.size()) {
            std::cerr << "WriteFile failed for " << label
                      << " (ok=" << (ok ? "true" : "false")
                      << ", bytesWritten=" << bytesWritten
                      << ", lastError=" << GetLastError() << ")\n";
            return false;
        }

        std::cout << "WriteFile OK: " << bytesWritten << "/" << report.size() << " bytes\n";
        return true;
    }

    char readObservation()
    {
        for (;;) {
            std::cout
                << "Observation [B=both color+R2, L=lightbar only, T=trigger only, N=neither, Q=quit]: ";
            std::string line;
            std::getline(std::cin, line);
            if (line.empty()) {
                continue;
            }
            const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(line.front())));
            if (c == 'B' || c == 'L' || c == 'T' || c == 'N' || c == 'Q') {
                return c;
            }
        }
    }

    std::string observationName(char value)
    {
        switch (value) {
        case 'B': return "both";
        case 'L': return "lightbar-only";
        case 'T': return "trigger-only";
        case 'N': return "neither";
        default: return "quit";
        }
    }

    bool pauseBeforeTest(std::string_view title, std::string_view expectation)
    {
        std::cout << "\n============================================================\n"
                  << title << "\n"
                  << expectation << "\n"
                  << "Press Enter to SEND this variant, or type Q then Enter to stop: ";
        std::string line;
        std::getline(std::cin, line);
        return line.empty() || std::toupper(static_cast<unsigned char>(line.front())) != 'Q';
    }
}

int main()
{
    std::cout
        << "StarfieldDualSense standalone HID probe v0.2.27f\n"
        << "This is a transport diagnostic. Starfield must be COMPLETELY CLOSED.\n"
        << "Keep DSX closed and do not launch Starfield until this probe exits.\n\n";

    if (processRunning(L"Starfield.exe")) {
        std::cerr << "REFUSING TO RUN: Starfield.exe is currently running. Close it and retry.\n";
        return 2;
    }

    auto device = openDualSense();
    if (!device || !device->valid()) {
        std::cerr
            << "No wired DualSense/DualSense Edge with a 48-byte USB output report was found.\n"
            << "Connect the controller by USB and retry.\n";
        return 3;
    }

    const char* model = device->attributes.ProductID == kDualSenseEdgePid ? "DualSense Edge" : "DualSense";
    std::cout
        << "Found " << model
        << " VID=054C PID=" << std::hex << std::uppercase << device->attributes.ProductID << std::dec
        << " input=" << device->caps.InputReportByteLength
        << " output=" << device->caps.OutputReportByteLength
        << " feature=" << device->caps.FeatureReportByteLength << "\n";

    struct Result
    {
        std::string name;
        char observation{ 'Q' };
    };
    std::vector<Result> results;

    auto cleanup = [&]() {
        const auto reset = sds::probe::buildResetReport();
        (void)sendReport(device->handle, reset, "cleanup/reset");
        Sleep(250);
    };

    if (pauseBeforeTest(
            "TEST 1/3 - LEGACY KNOWN-GOOD PACKET",
            "Expected if accepted: RED lightbar + a VERY obvious strong R2 resistance wall.")) {
        const auto legacy = sds::probe::buildLegacyKnownGoodReport();
        if (!sendReport(device->handle, legacy, "legacy-known-good")) {
            return 4;
        }
        const char observation = readObservation();
        results.push_back({ "legacy", observation });
        cleanup();
        if (observation == 'Q') {
            goto finish;
        }
    } else {
        goto finish;
    }

    if (pauseBeforeTest(
            "TEST 2/3 - CURRENT v0.2.27e SCOPED PACKET",
            "Expected if accepted: GREEN lightbar + the same strong R2 wall. This reproduces the current one-shot init + scoped steady packet.")) {
        const auto init = sds::probe::buildCurrentInitializationReport();
        const auto scoped = sds::probe::buildCurrentScopedReport();
        if (!sendReport(device->handle, init, "current-init") ||
            !sendReport(device->handle, scoped, "current-scoped")) {
            return 5;
        }
        const char observation = readObservation();
        results.push_back({ "current", observation });
        cleanup();
        if (observation == 'Q') {
            goto finish;
        }
    } else {
        goto finish;
    }

    if (pauseBeforeTest(
            "TEST 3/3 - HYBRID PACKET",
            "Expected if accepted: BLUE lightbar + the same strong R2 wall. This keeps the legacy validity/control shape but removes ONLY repeated lightbar setup.")) {
        const auto hybrid = sds::probe::buildHybridReport();
        if (!sendReport(device->handle, hybrid, "hybrid")) {
            return 6;
        }
        const char observation = readObservation();
        results.push_back({ "hybrid", observation });
        cleanup();
    }

finish:
    cleanup();

    std::cout << "\n==================== PROBE SUMMARY ====================\n";
    for (const auto& result : results) {
        std::cout << result.name << '=' << observationName(result.observation) << '\n';
    }

    std::ofstream file("sds-hid-probe-results.txt", std::ios::trunc);
    if (file) {
        file << "StarfieldDualSense HID probe v0.2.27f\n";
        file << "model=" << model
             << " pid=0x" << std::hex << std::uppercase << device->attributes.ProductID << std::dec
             << " input=" << device->caps.InputReportByteLength
             << " output=" << device->caps.OutputReportByteLength
             << " feature=" << device->caps.FeatureReportByteLength << '\n';
        for (const auto& result : results) {
            file << result.name << '=' << observationName(result.observation) << '\n';
        }
        std::cout << "Saved: sds-hid-probe-results.txt\n";
    }

    std::cout
        << "\nSend me the three result lines (or the saved results file).\n"
        << "If the controller remains in an odd LED state, unplug/replug it once after the probe exits.\n";
    return 0;
}
