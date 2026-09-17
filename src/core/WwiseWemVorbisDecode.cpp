#include <StarfieldDualSense/WwiseWemVorbisDecode.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace sds
{
    VoiceWwiseVorbisDecodeResult decodeWwiseVorbisToPcmWithBackend(
        const VoiceWemPayloadProbeResult& payloadResult,
        const VoiceWemStructureProbeResult& structureResult,
        const VoiceWwiseVorbisPacketProbeResult& packetProbe,
        std::span<const unsigned char> packedCodebookLibrary,
        OggVorbisDecodeBackend backend,
        std::string_view backendName)
    {
        VoiceWwiseVorbisDecodeResult result{};
        result.attempted = true;
        result.backend = std::string(backendName);
        result.expectedSamples = packetProbe.numSamples;

        if (!backend) {
            result.error = "Vorbis decoder backend is null";
            return result;
        }

        const auto rebuilt = rebuildWwiseVorbisOgg(
            payloadResult,
            structureResult,
            packetProbe,
            packedCodebookLibrary);
        result.reconstructed = rebuilt.success;
        result.oggBytes = rebuilt.ogg.size();
        result.audioPacketCount = rebuilt.audioPacketCount;
        if (!rebuilt.success) {
            result.error = rebuilt.error;
            return result;
        }

        std::uint16_t decodedChannels = 0u;
        std::uint32_t decodedRate = 0u;
        std::vector<std::int16_t> pcm;
        std::string backendError;
        if (!backend(rebuilt.ogg, decodedChannels, decodedRate, pcm, backendError)) {
            result.error = backendError.empty() ? "Vorbis decoder backend failed" : backendError;
            return result;
        }
        if (decodedChannels == 0u || decodedRate == 0u) {
            result.error = "Vorbis decoder returned invalid format";
            return result;
        }
        if ((pcm.size() % decodedChannels) != 0u) {
            result.error = "Vorbis decoder returned non-interleaved sample count";
            return result;
        }

        result.channels = decodedChannels;
        result.sampleRate = decodedRate;
        result.decodedSamples = static_cast<std::uint32_t>(pcm.size() / decodedChannels);
        result.sampleCountMatches = result.decodedSamples == result.expectedSamples;
        result.durationMs = (static_cast<std::uint64_t>(result.decodedSamples) * 1000u) / decodedRate;
        result.pcmBytes = pcm.size() * sizeof(std::int16_t);
        result.pcm = std::move(pcm);

        if (!result.pcm.empty()) {
            result.firstSample = result.pcm.front();
            result.lastSample = result.pcm.back();
            double sumSquares = 0.0;
            float peak = 0.0f;
            for (const auto sample : result.pcm) {
                const auto normalized = static_cast<float>(sample) / 32768.0f;
                peak = std::max(peak, std::abs(normalized));
                sumSquares += static_cast<double>(normalized) * static_cast<double>(normalized);
            }
            result.peak = peak;
            result.rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(result.pcm.size())));
        }

        result.decodeSucceeded = true;
        return result;
    }

    std::string formatWwiseVorbisDecodeSummary(const VoiceWwiseVorbisDecodeResult& result)
    {
        std::ostringstream out;
        out << "Voice Wwise Vorbis decode:"
            << " status=" << (result.decodeSucceeded ? "success" : "failed")
            << " backend=" << result.backend
            << " reconstructed=" << (result.reconstructed ? "yes" : "no")
            << " channels=" << result.channels
            << " sampleRate=" << result.sampleRate
            << " expectedSamples=" << result.expectedSamples
            << " decodedSamples=" << result.decodedSamples
            << " durationMs=" << result.durationMs
            << " peak=" << std::fixed << std::setprecision(6) << result.peak
            << " rms=" << std::fixed << std::setprecision(6) << result.rms
            << " firstSample=" << result.firstSample
            << " lastSample=" << result.lastSample
            << " pcmBytes=" << result.pcmBytes
            << " oggBytes=" << result.oggBytes
            << " audioPackets=" << result.audioPacketCount
            << " sampleCountMatches=" << (result.sampleCountMatches ? "yes" : "no");
        if (!result.error.empty()) {
            out << " error=\"" << result.error << '"';
        }
        return out.str();
    }
}
