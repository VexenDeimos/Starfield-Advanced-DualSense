#include <StarfieldDualSense/WwiseVorbisRebuild.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    class BitReader
    {
    public:
        explicit BitReader(std::span<const unsigned char> bytes) : _bytes(bytes) {}

        std::uint64_t read(unsigned bits)
        {
            if (bits > 64u || _bitPos + bits > _bytes.size() * 8u) {
                throw std::runtime_error("bitstream truncated");
            }
            std::uint64_t value = 0;
            for (unsigned i = 0; i < bits; ++i) {
                const auto byteIndex = (_bitPos + i) / 8u;
                const auto bitIndex = (_bitPos + i) % 8u;
                value |= static_cast<std::uint64_t>((_bytes[byteIndex] >> bitIndex) & 1u) << i;
            }
            _bitPos += bits;
            return value;
        }

        [[nodiscard]] std::size_t bitsRead() const noexcept { return _bitPos; }
        [[nodiscard]] std::size_t roundedBytesRead() const noexcept { return (_bitPos + 7u) / 8u; }

    private:
        std::span<const unsigned char> _bytes;
        std::size_t _bitPos{ 0 };
    };

    class BitWriter
    {
    public:
        void write(std::uint64_t value, unsigned bits)
        {
            if (bits > 64u) {
                throw std::runtime_error("invalid bit width");
            }
            for (unsigned i = 0; i < bits; ++i) {
                const auto byteIndex = _bitPos / 8u;
                const auto bitIndex = _bitPos % 8u;
                if (byteIndex == _bytes.size()) {
                    _bytes.push_back(0u);
                }
                const auto bit = static_cast<unsigned char>((value >> i) & 1u);
                _bytes[byteIndex] = static_cast<unsigned char>(_bytes[byteIndex] | (bit << bitIndex));
                ++_bitPos;
            }
        }

        void writeBytes(std::span<const unsigned char> bytes)
        {
            for (const auto byte : bytes) {
                write(byte, 8u);
            }
        }

        [[nodiscard]] std::vector<unsigned char> take() { return std::move(_bytes); }

    private:
        std::vector<unsigned char> _bytes{};
        std::size_t _bitPos{ 0 };
    };

    unsigned ilog(std::uint32_t value)
    {
        unsigned bits = 0;
        while (value != 0u) {
            ++bits;
            value >>= 1u;
        }
        return bits;
    }

    std::uint32_t readLe32(std::span<const unsigned char> bytes, std::size_t offset)
    {
        if (offset + 4u > bytes.size()) {
            throw std::runtime_error("codebook library truncated");
        }
        return static_cast<std::uint32_t>(bytes[offset]) |
               (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
               (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
               (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
    }

    void appendLe32(std::vector<unsigned char>& out, std::uint32_t value)
    {
        out.push_back(static_cast<unsigned char>(value & 0xFFu));
        out.push_back(static_cast<unsigned char>((value >> 8u) & 0xFFu));
        out.push_back(static_cast<unsigned char>((value >> 16u) & 0xFFu));
        out.push_back(static_cast<unsigned char>((value >> 24u) & 0xFFu));
    }

    void appendLe64(std::vector<unsigned char>& out, std::uint64_t value)
    {
        for (unsigned i = 0; i < 8u; ++i) {
            out.push_back(static_cast<unsigned char>((value >> (8u * i)) & 0xFFu));
        }
    }

    class PackedCodebookLibrary
    {
    public:
        explicit PackedCodebookLibrary(std::span<const unsigned char> bytes) : _bytes(bytes)
        {
            if (bytes.size() < 12u) {
                throw std::runtime_error("invalid packed codebook library");
            }
            const auto tableOffset = readLe32(bytes, bytes.size() - 4u);
            if (tableOffset >= bytes.size() || tableOffset + 8u > bytes.size() ||
                ((bytes.size() - tableOffset) % 4u) != 0u) {
                throw std::runtime_error("invalid packed codebook offset table");
            }
            const auto count = (bytes.size() - tableOffset) / 4u;
            if (count < 2u) {
                throw std::runtime_error("packed codebook library has no entries");
            }
            _offsets.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                const auto offset = readLe32(bytes, tableOffset + i * 4u);
                if (offset > tableOffset || (!_offsets.empty() && offset < _offsets.back())) {
                    throw std::runtime_error("invalid packed codebook offsets");
                }
                _offsets.push_back(offset);
            }
            if (_offsets.front() != 0u || _offsets.back() != tableOffset) {
                throw std::runtime_error("packed codebook offsets do not cover data");
            }
        }

        [[nodiscard]] std::span<const unsigned char> codebook(std::uint32_t id) const
        {
            if (id + 1u >= _offsets.size()) {
                throw std::runtime_error("invalid external codebook id");
            }
            const auto begin = _offsets[id];
            const auto end = _offsets[id + 1u];
            return _bytes.subspan(begin, end - begin);
        }

    private:
        std::span<const unsigned char> _bytes;
        std::vector<std::uint32_t> _offsets{};
    };

    std::uint32_t mapType1Quantvals(std::uint32_t entries, std::uint32_t dimensions)
    {
        if (entries == 0u || dimensions == 0u) {
            throw std::runtime_error("invalid codebook dimensions or entries");
        }
        const auto bits = ilog(entries);
        auto values = entries >> (((bits - 1u) * (dimensions - 1u)) / dimensions);
        values = std::max<std::uint32_t>(values, 1u);
        for (;;) {
            std::uint64_t lower = 1u;
            std::uint64_t upper = 1u;
            for (std::uint32_t i = 0; i < dimensions; ++i) {
                lower *= values;
                upper *= values + 1u;
            }
            if (lower <= entries && upper > entries) {
                return values;
            }
            if (lower > entries) {
                --values;
            } else {
                ++values;
            }
        }
    }

    void rebuildPackedCodebook(std::span<const unsigned char> packed, BitWriter& out)
    {
        BitReader in(packed);
        const auto dimensions = static_cast<std::uint32_t>(in.read(4u));
        const auto entries = static_cast<std::uint32_t>(in.read(14u));
        if (dimensions == 0u || entries == 0u) {
            throw std::runtime_error("invalid packed codebook dimensions or entries");
        }

        out.write(0x564342u, 24u);
        out.write(dimensions, 16u);
        out.write(entries, 24u);

        const auto ordered = in.read(1u);
        out.write(ordered, 1u);
        if (ordered != 0u) {
            out.write(in.read(5u), 5u);
            std::uint32_t currentEntry = 0u;
            while (currentEntry < entries) {
                const auto width = ilog(entries - currentEntry);
                const auto count = static_cast<std::uint32_t>(in.read(width));
                out.write(count, width);
                currentEntry += count;
                if (count == 0u || currentEntry > entries) {
                    throw std::runtime_error("invalid ordered packed codebook lengths");
                }
            }
        } else {
            const auto lengthBits = static_cast<unsigned>(in.read(3u));
            const auto sparse = in.read(1u);
            if (lengthBits == 0u || lengthBits > 5u) {
                throw std::runtime_error("invalid packed codebook length width");
            }
            out.write(sparse, 1u);
            for (std::uint32_t i = 0; i < entries; ++i) {
                bool present = true;
                if (sparse != 0u) {
                    present = in.read(1u) != 0u;
                    out.write(present ? 1u : 0u, 1u);
                }
                if (present) {
                    out.write(in.read(lengthBits), 5u);
                }
            }
        }

        const auto lookupType = static_cast<std::uint32_t>(in.read(1u));
        out.write(lookupType, 4u);
        if (lookupType == 1u) {
            out.write(in.read(32u), 32u);
            out.write(in.read(32u), 32u);
            const auto valueLengthMinus1 = static_cast<unsigned>(in.read(4u));
            out.write(valueLengthMinus1, 4u);
            out.write(in.read(1u), 1u);
            const auto quantvals = mapType1Quantvals(entries, dimensions);
            for (std::uint32_t i = 0; i < quantvals; ++i) {
                out.write(in.read(valueLengthMinus1 + 1u), valueLengthMinus1 + 1u);
            }
        }

        const auto expectedBytes = in.bitsRead() / 8u + 1u;
        if (expectedBytes != packed.size()) {
            throw std::runtime_error("packed codebook size mismatch");
        }
    }

    std::vector<unsigned char> makeIdentificationPacket(
        std::uint16_t channels,
        std::uint32_t sampleRate,
        std::uint32_t averageBytesPerSecond,
        std::uint8_t blockExpSmall,
        std::uint8_t blockExpLarge)
    {
        std::vector<unsigned char> packet;
        packet.reserve(30u);
        packet.push_back(1u);
        packet.insert(packet.end(), { 'v', 'o', 'r', 'b', 'i', 's' });
        appendLe32(packet, 0u);
        packet.push_back(static_cast<unsigned char>(channels));
        appendLe32(packet, sampleRate);
        appendLe32(packet, 0u);
        appendLe32(packet, averageBytesPerSecond * 8u);
        appendLe32(packet, 0u);
        packet.push_back(static_cast<unsigned char>((blockExpLarge << 4u) | blockExpSmall));
        packet.push_back(1u);
        return packet;
    }

    std::vector<unsigned char> makeCommentPacket()
    {
        static constexpr char kVendor[] = "StarfieldDualSense Wwise Vorbis bridge";
        std::vector<unsigned char> packet;
        packet.push_back(3u);
        packet.insert(packet.end(), { 'v', 'o', 'r', 'b', 'i', 's' });
        appendLe32(packet, static_cast<std::uint32_t>(sizeof(kVendor) - 1u));
        packet.insert(packet.end(), kVendor, kVendor + sizeof(kVendor) - 1u);
        appendLe32(packet, 0u);
        packet.push_back(1u);
        return packet;
    }

    std::uint32_t oggCrc(std::span<const unsigned char> bytes)
    {
        std::uint32_t crc = 0u;
        for (const auto byte : bytes) {
            crc ^= static_cast<std::uint32_t>(byte) << 24u;
            for (unsigned bit = 0; bit < 8u; ++bit) {
                crc = (crc & 0x80000000u) != 0u ? (crc << 1u) ^ 0x04C11DB7u : crc << 1u;
            }
        }
        return crc;
    }

    void appendOggPage(
        std::vector<unsigned char>& ogg,
        std::uint8_t headerType,
        std::uint64_t granule,
        std::uint32_t serial,
        std::uint32_t sequence,
        std::span<const unsigned char> lacing,
        std::span<const unsigned char> payload)
    {
        const auto pageStart = ogg.size();
        ogg.insert(ogg.end(), { 'O', 'g', 'g', 'S' });
        ogg.push_back(0u);
        ogg.push_back(headerType);
        appendLe64(ogg, granule);
        appendLe32(ogg, serial);
        appendLe32(ogg, sequence);
        appendLe32(ogg, 0u);
        ogg.push_back(static_cast<unsigned char>(lacing.size()));
        ogg.insert(ogg.end(), lacing.begin(), lacing.end());
        ogg.insert(ogg.end(), payload.begin(), payload.end());

        const auto crc = oggCrc(std::span<const unsigned char>(ogg).subspan(pageStart));
        const auto crcOffset = pageStart + 22u;
        ogg[crcOffset + 0u] = static_cast<unsigned char>(crc & 0xFFu);
        ogg[crcOffset + 1u] = static_cast<unsigned char>((crc >> 8u) & 0xFFu);
        ogg[crcOffset + 2u] = static_cast<unsigned char>((crc >> 16u) & 0xFFu);
        ogg[crcOffset + 3u] = static_cast<unsigned char>((crc >> 24u) & 0xFFu);
    }

    void appendOggPacket(
        std::vector<unsigned char>& ogg,
        std::span<const unsigned char> packet,
        std::uint8_t firstHeaderType,
        bool eos,
        std::uint64_t finalGranule,
        std::uint32_t serial,
        std::uint32_t& sequence)
    {
        std::size_t offset = 0u;
        bool firstPage = true;
        bool terminated = false;
        do {
            std::vector<unsigned char> lacing;
            lacing.reserve(255u);
            const auto payloadStart = offset;
            while (lacing.size() < 255u && offset + 255u <= packet.size()) {
                lacing.push_back(255u);
                offset += 255u;
            }
            if (lacing.size() < 255u) {
                const auto remainder = packet.size() - offset;
                lacing.push_back(static_cast<unsigned char>(remainder));
                offset += remainder;
                terminated = true;
            }

            auto headerType = static_cast<std::uint8_t>(firstPage ? firstHeaderType : 0x01u);
            if (eos && terminated) {
                headerType = static_cast<std::uint8_t>(headerType | 0x04u);
            }
            const auto granule = terminated ? finalGranule : std::numeric_limits<std::uint64_t>::max();
            appendOggPage(
                ogg,
                headerType,
                granule,
                serial,
                sequence++,
                lacing,
                packet.subspan(payloadStart, offset - payloadStart));
            firstPage = false;
        } while (!terminated);
    }

    struct RawPacket
    {
        std::span<const unsigned char> payload{};
        std::size_t nextOffset{ 0 };
    };

    RawPacket readWwisePacket(std::span<const unsigned char> data, std::size_t offset)
    {
        if (offset + 2u > data.size()) {
            throw std::runtime_error("packet header truncated");
        }
        const auto size = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(data[offset]) |
            (static_cast<std::uint16_t>(data[offset + 1u]) << 8u));
        const auto payloadOffset = offset + 2u;
        const auto next = payloadOffset + size;
        if (next > data.size()) {
            throw std::runtime_error("packet exceeds data bounds");
        }
        return { data.subspan(payloadOffset, size), next };
    }

    unsigned readModeNumber(std::span<const unsigned char> packet, unsigned modeBits)
    {
        if (packet.empty()) {
            throw std::runtime_error("empty audio packet");
        }
        if (modeBits == 0u) {
            return 0u;
        }
        BitReader reader(packet.first(1u));
        return static_cast<unsigned>(reader.read(modeBits));
    }
}

namespace sds
{
    WwiseVorbisSetupRebuildResult rebuildWwiseVorbisSetupPacket(
        std::span<const unsigned char> strippedSetup,
        std::span<const unsigned char> packedCodebookLibrary,
        std::uint16_t channels)
    {
        WwiseVorbisSetupRebuildResult result{};
        try {
            if (strippedSetup.empty()) {
                throw std::runtime_error("setup packet is empty");
            }
            if (channels == 0u || channels > 255u) {
                throw std::runtime_error("invalid channel count");
            }

            PackedCodebookLibrary codebooks(packedCodebookLibrary);
            BitReader in(strippedSetup);
            BitWriter out;
            out.write(5u, 8u);
            for (const auto c : std::array<unsigned char, 6>{ 'v', 'o', 'r', 'b', 'i', 's' }) {
                out.write(c, 8u);
            }

            const auto codebookCountMinus1 = static_cast<std::uint32_t>(in.read(8u));
            const auto codebookCount = codebookCountMinus1 + 1u;
            out.write(codebookCountMinus1, 8u);
            for (std::uint32_t i = 0; i < codebookCount; ++i) {
                const auto codebookId = static_cast<std::uint32_t>(in.read(10u));
                rebuildPackedCodebook(codebooks.codebook(codebookId), out);
            }

            out.write(0u, 6u);  // one time-domain-transform placeholder
            out.write(0u, 16u); // transform type 0

            const auto floorCountMinus1 = static_cast<std::uint32_t>(in.read(6u));
            const auto floorCount = floorCountMinus1 + 1u;
            out.write(floorCountMinus1, 6u);
            for (std::uint32_t floor = 0; floor < floorCount; ++floor) {
                out.write(1u, 16u); // Wwise stripped setup uses floor type 1
                const auto partitions = static_cast<std::uint32_t>(in.read(5u));
                out.write(partitions, 5u);

                std::vector<std::uint32_t> partitionClasses(partitions);
                std::uint32_t maxClass = 0u;
                for (std::uint32_t i = 0; i < partitions; ++i) {
                    partitionClasses[i] = static_cast<std::uint32_t>(in.read(4u));
                    out.write(partitionClasses[i], 4u);
                    maxClass = std::max(maxClass, partitionClasses[i]);
                }

                std::vector<std::uint32_t> classDimensions(maxClass + 1u);
                for (std::uint32_t cls = 0; cls <= maxClass; ++cls) {
                    const auto dimensionsMinus1 = static_cast<std::uint32_t>(in.read(3u));
                    classDimensions[cls] = dimensionsMinus1 + 1u;
                    out.write(dimensionsMinus1, 3u);
                    const auto subclasses = static_cast<std::uint32_t>(in.read(2u));
                    out.write(subclasses, 2u);
                    if (subclasses != 0u) {
                        const auto masterbook = static_cast<std::uint32_t>(in.read(8u));
                        if (masterbook >= codebookCount) {
                            throw std::runtime_error("invalid floor masterbook");
                        }
                        out.write(masterbook, 8u);
                    }
                    for (std::uint32_t i = 0; i < (1u << subclasses); ++i) {
                        const auto bookPlus1 = static_cast<std::uint32_t>(in.read(8u));
                        if (bookPlus1 != 0u && bookPlus1 - 1u >= codebookCount) {
                            throw std::runtime_error("invalid floor subclass book");
                        }
                        out.write(bookPlus1, 8u);
                    }
                }

                out.write(in.read(2u), 2u); // multiplier - 1
                const auto rangeBits = static_cast<unsigned>(in.read(4u));
                out.write(rangeBits, 4u);
                for (const auto cls : partitionClasses) {
                    for (std::uint32_t i = 0; i < classDimensions[cls]; ++i) {
                        out.write(in.read(rangeBits), rangeBits);
                    }
                }
            }

            const auto residueCountMinus1 = static_cast<std::uint32_t>(in.read(6u));
            const auto residueCount = residueCountMinus1 + 1u;
            out.write(residueCountMinus1, 6u);
            for (std::uint32_t residue = 0; residue < residueCount; ++residue) {
                const auto residueType = static_cast<std::uint32_t>(in.read(2u));
                if (residueType > 2u) {
                    throw std::runtime_error("invalid residue type");
                }
                out.write(residueType, 16u);
                out.write(in.read(24u), 24u);
                out.write(in.read(24u), 24u);
                out.write(in.read(24u), 24u);
                const auto classificationsMinus1 = static_cast<std::uint32_t>(in.read(6u));
                const auto classifications = classificationsMinus1 + 1u;
                out.write(classificationsMinus1, 6u);
                const auto classbook = static_cast<std::uint32_t>(in.read(8u));
                if (classbook >= codebookCount) {
                    throw std::runtime_error("invalid residue classbook");
                }
                out.write(classbook, 8u);

                std::vector<std::uint32_t> cascades(classifications);
                for (std::uint32_t i = 0; i < classifications; ++i) {
                    const auto low = static_cast<std::uint32_t>(in.read(3u));
                    out.write(low, 3u);
                    const auto hasHigh = in.read(1u) != 0u;
                    out.write(hasHigh ? 1u : 0u, 1u);
                    std::uint32_t high = 0u;
                    if (hasHigh) {
                        high = static_cast<std::uint32_t>(in.read(5u));
                        out.write(high, 5u);
                    }
                    cascades[i] = high * 8u + low;
                }
                for (const auto cascade : cascades) {
                    for (unsigned bit = 0; bit < 8u; ++bit) {
                        if ((cascade & (1u << bit)) != 0u) {
                            const auto book = static_cast<std::uint32_t>(in.read(8u));
                            if (book >= codebookCount) {
                                throw std::runtime_error("invalid residue book");
                            }
                            out.write(book, 8u);
                        }
                    }
                }
            }

            const auto mappingCountMinus1 = static_cast<std::uint32_t>(in.read(6u));
            const auto mappingCount = mappingCountMinus1 + 1u;
            out.write(mappingCountMinus1, 6u);
            for (std::uint32_t mapping = 0; mapping < mappingCount; ++mapping) {
                out.write(0u, 16u); // mapping type 0
                const auto hasSubmaps = in.read(1u) != 0u;
                out.write(hasSubmaps ? 1u : 0u, 1u);
                std::uint32_t submaps = 1u;
                if (hasSubmaps) {
                    const auto submapsMinus1 = static_cast<std::uint32_t>(in.read(4u));
                    out.write(submapsMinus1, 4u);
                    submaps = submapsMinus1 + 1u;
                }

                const auto hasCoupling = in.read(1u) != 0u;
                out.write(hasCoupling ? 1u : 0u, 1u);
                if (hasCoupling) {
                    if (channels < 2u) {
                        throw std::runtime_error("coupling present for mono stream");
                    }
                    const auto stepsMinus1 = static_cast<std::uint32_t>(in.read(8u));
                    out.write(stepsMinus1, 8u);
                    const auto channelBits = ilog(static_cast<std::uint32_t>(channels) - 1u);
                    for (std::uint32_t step = 0; step <= stepsMinus1; ++step) {
                        const auto magnitude = static_cast<std::uint32_t>(in.read(channelBits));
                        const auto angle = static_cast<std::uint32_t>(in.read(channelBits));
                        if (magnitude >= channels || angle >= channels || magnitude == angle) {
                            throw std::runtime_error("invalid channel coupling");
                        }
                        out.write(magnitude, channelBits);
                        out.write(angle, channelBits);
                    }
                }

                const auto reserved = static_cast<std::uint32_t>(in.read(2u));
                if (reserved != 0u) {
                    throw std::runtime_error("mapping reserved field nonzero");
                }
                out.write(reserved, 2u);

                if (submaps > 1u) {
                    for (std::uint16_t channel = 0; channel < channels; ++channel) {
                        const auto mux = static_cast<std::uint32_t>(in.read(4u));
                        if (mux >= submaps) {
                            throw std::runtime_error("mapping mux outside submaps");
                        }
                        out.write(mux, 4u);
                    }
                }
                for (std::uint32_t submap = 0; submap < submaps; ++submap) {
                    out.write(in.read(8u), 8u); // time config
                    const auto floorNumber = static_cast<std::uint32_t>(in.read(8u));
                    const auto residueNumber = static_cast<std::uint32_t>(in.read(8u));
                    if (floorNumber >= floorCount || residueNumber >= residueCount) {
                        throw std::runtime_error("invalid mapping floor or residue");
                    }
                    out.write(floorNumber, 8u);
                    out.write(residueNumber, 8u);
                }
            }

            const auto modeCountMinus1 = static_cast<std::uint32_t>(in.read(6u));
            const auto modeCount = modeCountMinus1 + 1u;
            out.write(modeCountMinus1, 6u);
            result.modeBlockFlags.reserve(modeCount);
            result.modeBits = ilog(modeCount - 1u);
            for (std::uint32_t mode = 0; mode < modeCount; ++mode) {
                const auto blockFlag = in.read(1u) != 0u;
                result.modeBlockFlags.push_back(blockFlag);
                out.write(blockFlag ? 1u : 0u, 1u);
                out.write(0u, 16u); // window type
                out.write(0u, 16u); // transform type
                const auto mapping = static_cast<std::uint32_t>(in.read(8u));
                if (mapping >= mappingCount) {
                    throw std::runtime_error("invalid mode mapping");
                }
                out.write(mapping, 8u);
            }
            out.write(1u, 1u); // Vorbis setup framing bit

            if (in.roundedBytesRead() != strippedSetup.size()) {
                throw std::runtime_error("setup packet was not consumed exactly");
            }

            result.packet = out.take();
            result.success = true;
        } catch (const std::exception& e) {
            result.error = e.what();
        }
        return result;
    }

    WwiseVorbisAudioRebuildResult rebuildWwiseVorbisAudioPacket(
        std::span<const unsigned char> modifiedPacket,
        const std::vector<bool>& modeBlockFlags,
        bool previousBlockFlag,
        bool nextBlockFlag)
    {
        WwiseVorbisAudioRebuildResult result{};
        try {
            if (modifiedPacket.empty()) {
                throw std::runtime_error("empty audio packet");
            }
            if (modeBlockFlags.empty()) {
                throw std::runtime_error("mode table is empty");
            }
            const auto modeBits = ilog(static_cast<std::uint32_t>(modeBlockFlags.size() - 1u));
            const auto mode = readModeNumber(modifiedPacket, modeBits);
            if (mode >= modeBlockFlags.size()) {
                throw std::runtime_error("audio packet mode outside mode table");
            }

            BitWriter out;
            out.write(0u, 1u); // audio packet type
            out.write(mode, modeBits);
            result.modeNumber = mode;
            result.blockFlag = modeBlockFlags[mode];
            if (result.blockFlag) {
                out.write(previousBlockFlag ? 1u : 0u, 1u);
                out.write(nextBlockFlag ? 1u : 0u, 1u);
            }

            BitReader in(modifiedPacket);
            if (modeBits != 0u) {
                (void)in.read(modeBits);
            }
            const auto totalBits = modifiedPacket.size() * 8u;
            while (in.bitsRead() < totalBits) {
                const auto remaining = totalBits - in.bitsRead();
                const auto width = static_cast<unsigned>(std::min<std::size_t>(remaining, 8u));
                out.write(in.read(width), width);
            }
            result.packet = out.take();
            result.success = true;
        } catch (const std::exception& e) {
            result.error = e.what();
        }
        return result;
    }

    WwiseVorbisOggRebuildResult rebuildWwiseVorbisOgg(
        const VoiceWemPayloadProbeResult& payloadResult,
        const VoiceWemStructureProbeResult& structureResult,
        const VoiceWwiseVorbisPacketProbeResult& packetProbe,
        std::span<const unsigned char> packedCodebookLibrary)
    {
        WwiseVorbisOggRebuildResult result{};
        result.attempted = true;
        try {
            if (!payloadResult.readSucceeded || !structureResult.scanComplete || !packetProbe.probeComplete) {
                throw std::runtime_error("source probes are incomplete");
            }
            if (!packetProbe.recognizedNewFmt30 || packetProbe.packetHeaderBytes != 2u || !packetProbe.modifiedPackets) {
                throw std::runtime_error("unsupported Wwise Vorbis variant");
            }
            if (!structureResult.dataFound || structureResult.dataOffset + structureResult.dataSize > payloadResult.payload.size()) {
                throw std::runtime_error("WEM data chunk outside payload");
            }
            if (packetProbe.setupOffset >= structureResult.dataSize || packetProbe.audioOffset > structureResult.dataSize) {
                throw std::runtime_error("Wwise Vorbis offsets outside data");
            }

            const auto data = std::span<const unsigned char>(payloadResult.payload)
                                  .subspan(static_cast<std::size_t>(structureResult.dataOffset), structureResult.dataSize);
            const auto setupPacket = readWwisePacket(data, packetProbe.setupOffset);
            if (setupPacket.nextOffset != packetProbe.audioOffset) {
                throw std::runtime_error("setup packet does not end at audio offset");
            }

            const auto setup = rebuildWwiseVorbisSetupPacket(
                setupPacket.payload,
                packedCodebookLibrary,
                packetProbe.channels);
            if (!setup.success) {
                throw std::runtime_error("setup rebuild failed: " + setup.error);
            }
            result.modeBlockFlags = setup.modeBlockFlags;
            result.modeBits = setup.modeBits;

            static constexpr std::uint32_t kSerial = 0x53445336u; // "SDS6"
            std::uint32_t sequence = 0u;
            const auto identification = makeIdentificationPacket(
                packetProbe.channels,
                packetProbe.sampleRate,
                structureResult.averageBytesPerSecond,
                packetProbe.blockExpSmall,
                packetProbe.blockExpLarge);
            const auto comment = makeCommentPacket();
            appendOggPacket(result.ogg, identification, 0x02u, false, 0u, kSerial, sequence);
            appendOggPacket(result.ogg, comment, 0u, false, 0u, kSerial, sequence);
            appendOggPacket(result.ogg, setup.packet, 0u, false, 0u, kSerial, sequence);

            std::size_t offset = packetProbe.audioOffset;
            bool previousBlockFlag = false;
            while (offset < data.size()) {
                const auto current = readWwisePacket(data, offset);
                const bool lastPacket = current.nextOffset == data.size();
                bool nextBlockFlag = false;
                if (!lastPacket) {
                    const auto next = readWwisePacket(data, current.nextOffset);
                    if (!next.payload.empty()) {
                        const auto nextMode = readModeNumber(next.payload, setup.modeBits);
                        if (nextMode >= setup.modeBlockFlags.size()) {
                            throw std::runtime_error("next audio packet mode outside mode table");
                        }
                        nextBlockFlag = setup.modeBlockFlags[nextMode];
                    }
                }

                const auto rebuilt = rebuildWwiseVorbisAudioPacket(
                    current.payload,
                    setup.modeBlockFlags,
                    previousBlockFlag,
                    nextBlockFlag);
                if (!rebuilt.success) {
                    throw std::runtime_error("audio packet rebuild failed: " + rebuilt.error);
                }
                appendOggPacket(
                    result.ogg,
                    rebuilt.packet,
                    0u,
                    lastPacket,
                    lastPacket ? packetProbe.numSamples : std::numeric_limits<std::uint64_t>::max(),
                    kSerial,
                    sequence);
                previousBlockFlag = rebuilt.blockFlag;
                ++result.audioPacketCount;
                offset = current.nextOffset;
            }
            if (offset != data.size() || result.audioPacketCount == 0u) {
                throw std::runtime_error("audio packet stream did not terminate cleanly");
            }
            result.success = true;
        } catch (const std::exception& e) {
            result.error = e.what();
            result.ogg.clear();
        }
        return result;
    }
}
