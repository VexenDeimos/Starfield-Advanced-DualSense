#include <StarfieldDualSense/WwiseRemoteVoFilesystemProbe.h>

#include <sstream>

namespace
{
    [[nodiscard]] std::string narrow(std::wstring_view value)
    {
        std::string out;
        out.reserve(value.size());
        for (const wchar_t ch : value) {
            out.push_back(ch >= 0 && ch <= 0x7F ? static_cast<char>(ch) : '?');
        }
        return out;
    }

    [[nodiscard]] std::string narrowPath(const std::filesystem::path& path)
    {
        return narrow(path.wstring());
    }

    void appendCapturedComponents(std::filesystem::path& base, std::wstring_view capturedPath)
    {
        std::size_t start = 0;
        while (start < capturedPath.size()) {
            while (start < capturedPath.size() &&
                   (capturedPath[start] == L'\\' || capturedPath[start] == L'/')) {
                ++start;
            }
            if (start >= capturedPath.size()) {
                break;
            }

            auto end = start;
            while (end < capturedPath.size() &&
                   capturedPath[end] != L'\\' && capturedPath[end] != L'/') {
                ++end;
            }
            base /= std::filesystem::path(std::wstring(capturedPath.substr(start, end - start)));
            start = end;
        }
    }
}

sds::RemoteVoFilesystemCandidates sds::buildRemoteVoFilesystemCandidates(
    std::wstring_view capturedPath,
    const std::filesystem::path& executablePath)
{
    RemoteVoFilesystemCandidates candidates{};
    candidates.raw.label = "raw";
    candidates.raw.path = std::filesystem::path(std::wstring(capturedPath));

    candidates.dataRoot.label = "data-root";
    if (!executablePath.empty()) {
        candidates.dataRoot.path = executablePath.parent_path() / "Data";
        appendCapturedComponents(candidates.dataRoot.path, capturedPath);
    }
    return candidates;
}

std::string sds::formatRemoteVoFilesystemProbeContext(
    std::wstring_view capturedPath,
    const std::filesystem::path& executablePath)
{
    std::ostringstream out;
    out << "Remote VO filesystem probe: captured=\"" << narrow(capturedPath) << "\""
        << " executable=\"" << narrowPath(executablePath) << "\"";
    return out.str();
}

std::string sds::formatRemoteVoFilesystemCandidateProbe(
    const RemoteVoFilesystemCandidate& candidate,
    bool exists,
    const WwiseWemSourceMetadata& metadata)
{
    std::ostringstream out;
    out << "Remote VO filesystem probe: candidate=" << candidate.label
        << " path=\"" << narrowPath(candidate.path) << "\""
        << " exists=" << (exists ? "yes" : "no");

    if (!metadata.openSucceeded) {
        out << " open=failed";
        if (!metadata.error.empty()) {
            out << " error=\"" << metadata.error << "\"";
        }
        return out.str();
    }

    out << " open=success"
        << " fileSize=" << metadata.fileSize
        << " container=" << (metadata.riffWave ? "RIFF/WAVE" : "unknown")
        << " fmt=" << (metadata.fmtFound ? "yes" : "no")
        << " formatTag=0x" << std::hex << std::uppercase << metadata.formatTag << std::dec
        << " channels=" << metadata.channels
        << " sampleRate=" << metadata.sampleRate
        << " vorb=" << (metadata.vorbFound ? "yes" : "no")
        << " vorbSize=" << metadata.vorbSize
        << " data=" << (metadata.dataFound ? "yes" : "no")
        << " dataOffset=" << metadata.dataOffset
        << " dataSize=" << metadata.dataSize
        << " chunksScanned=" << metadata.chunksScanned;
    if (!metadata.error.empty()) {
        out << " warning=\"" << metadata.error << "\"";
    }
    return out.str();
}
