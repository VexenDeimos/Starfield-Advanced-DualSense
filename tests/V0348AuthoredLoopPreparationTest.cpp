#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/WwiseWemSmplLoop.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            std::exit(1);
        }
    }

    void put32(std::vector<unsigned char>& bytes, std::uint32_t value)
    {
        bytes.push_back(static_cast<unsigned char>(value & 0xFFu));
        bytes.push_back(static_cast<unsigned char>((value >> 8u) & 0xFFu));
        bytes.push_back(static_cast<unsigned char>((value >> 16u) & 0xFFu));
        bytes.push_back(static_cast<unsigned char>((value >> 24u) & 0xFFu));
    }

    void patch32(std::vector<unsigned char>& bytes, std::size_t offset, std::uint32_t value)
    {
        for (unsigned i = 0; i < 4u; ++i) {
            bytes[offset + i] = static_cast<unsigned char>((value >> (8u * i)) & 0xFFu);
        }
    }

    void addChunk(std::vector<unsigned char>& bytes, const char* id, const std::vector<unsigned char>& payload)
    {
        bytes.insert(bytes.end(), id, id + 4);
        put32(bytes, static_cast<std::uint32_t>(payload.size()));
        bytes.insert(bytes.end(), payload.begin(), payload.end());
        if ((payload.size() & 1u) != 0u) {
            bytes.push_back(0u);
        }
    }

    std::vector<unsigned char> makeLoopWem()
    {
        std::vector<unsigned char> bytes{ 'R','I','F','F',0,0,0,0,'W','A','V','E' };
        std::vector<unsigned char> smpl;
        put32(smpl, 0u); put32(smpl, 0u); put32(smpl, 44100u); put32(smpl, 0u);
        put32(smpl, 0u); put32(smpl, 0u); put32(smpl, 0u); put32(smpl, 1u); put32(smpl, 0u);
        put32(smpl, 0u); put32(smpl, 0u); put32(smpl, 110145u); put32(smpl, 1589121u); put32(smpl, 0u); put32(smpl, 0u);
        addChunk(bytes, "smpl", smpl);
        std::vector<unsigned char> data(8u, 0u);
        addChunk(bytes, "data", data);
        patch32(bytes, 4u, static_cast<std::uint32_t>(bytes.size() - 8u));
        return bytes;
    }
}

int main()
{
    const auto wem = makeLoopWem();
    const auto loop = sds::probeWwiseSmplLoop(wem);
    require(loop.found, "smpl loop found");
    require(loop.startFrame == 110145u, "smpl start frame");
    require(loop.endFrameInclusive == 1589121u, "smpl inclusive end frame");

    sds::PreparedSpeakerPcm pcm{};
    pcm.frames.resize(1000u);
    for (std::size_t i = 0; i < pcm.frames.size(); ++i) {
        pcm.frames[i] = { static_cast<float>(i), static_cast<float>(i) };
    }
    std::size_t resume = 0u;
    const bool ok = sds::prepareWeaponSpeakerAuthoredSustainedLoopPcm(
        pcm, 0.35F, 10u, 24000u, 100u, 399u, resume);
    require(ok, "authored loop preparation");
    require(pcm.frames.size() == 800u, "trim after authored inclusive loop end");
    require(resume == 210u, "resume skips crossfaded loop head");
    require(std::abs(pcm.gain - 0.35F) < 0.0001F, "gain preserved");
    require(pcm.frames[799].left > 200.0F && pcm.frames[799].left < 799.0F,
        "loop tail crossfades toward authored loop head");

    std::cout << "PASS v0.3.48 authored WEM loop preparation\n";
    return 0;
}
