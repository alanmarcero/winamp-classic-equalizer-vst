#include "EqPresets.h"

#include <algorithm>
#include <cstring>

namespace wce {

const std::array<EqPreset, 18>& builtInPresets()
{
    // Exact values from CreateEQPresets in Src/Winamp/main.cpp.
    static const std::array<EqPreset, 18> kPresets { {
        { "Classical", { 31, 31, 31, 31, 31, 31, 44, 44, 44, 48 }, 31 },
        { "Club", { 31, 31, 26, 22, 22, 22, 26, 31, 31, 31 }, 31 },
        { "Dance", { 16, 20, 28, 32, 32, 42, 44, 44, 32, 32 }, 31 },
        { "Flat", { 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 31 },
        { "Full Bass", { 16, 16, 16, 22, 29, 39, 46, 49, 50, 50 }, 31 },
        { "Full Bass & Treble", { 20, 22, 31, 44, 40, 29, 18, 14, 12, 12 }, 31 },
        { "Full Treble", { 48, 48, 48, 39, 27, 14, 6, 6, 6, 4 }, 31 },
        { "Laptop speakers/headphones", { 24, 14, 23, 38, 36, 29, 24, 16, 11, 8 }, 31 },
        { "Large hall", { 15, 15, 22, 22, 31, 40, 40, 40, 31, 31 }, 31 },
        { "Live", { 40, 31, 25, 23, 22, 22, 25, 27, 27, 28 }, 31 },
        { "Party", { 20, 20, 31, 31, 31, 31, 31, 31, 20, 20 }, 31 },
        { "Pop", { 35, 24, 20, 19, 23, 34, 36, 36, 35, 35 }, 31 },
        { "Reggae", { 31, 31, 33, 42, 31, 21, 21, 31, 31, 31 }, 31 },
        { "Rock", { 19, 24, 41, 45, 38, 25, 17, 14, 14, 14 }, 31 },
        { "Ska", { 36, 40, 39, 33, 25, 22, 17, 16, 14, 16 }, 31 },
        { "Soft", { 24, 29, 34, 36, 34, 25, 18, 16, 14, 12 }, 31 },
        { "Soft Rock", { 25, 25, 28, 33, 39, 41, 38, 33, 27, 17 }, 31 },
        { "Techno", { 19, 22, 31, 41, 40, 31, 19, 16, 16, 17 }, 31 },
    } };
    return kPresets;
}

std::vector<EqPreset> parseEqfFile(std::span<const std::uint8_t> bytes)
{
    std::vector<EqPreset> presets;
    if (bytes.size() < kEqfSignature.size() + kEqfRecordSize)
        return presets;

    if (std::memcmp(bytes.data(), kEqfSignature.data(), kEqfSignature.size()) != 0)
        return presets;

    std::size_t offset = kEqfSignature.size();
    while (offset + kEqfRecordSize <= bytes.size()) {
        const auto* record = bytes.data() + offset;
        std::size_t nameLen = 0;
        while (nameLen < kEqfNameFieldSize - 1 && record[nameLen] != 0)
            ++nameLen;

        EqPreset preset;
        preset.name.assign(reinterpret_cast<const char*>(record), nameLen);
        for (int i = 0; i < kBandCount; ++i) {
            preset.bands[static_cast<std::size_t>(i)] =
                std::clamp<int>(record[kEqfNameFieldSize + static_cast<std::size_t>(i)], kSliderMin, kSliderMax);
        }
        preset.preamp = std::clamp<int>(record[kEqfNameFieldSize + kBandCount], kSliderMin, kSliderMax);
        presets.push_back( std::move(preset) );
        offset += kEqfRecordSize;
    }

    return presets;
}

std::vector<std::uint8_t> serializeEqfFile(std::span<const EqPreset> presets)
{
    std::vector<std::uint8_t> out(kEqfSignature.size() + presets.size() * kEqfRecordSize, 0);
    std::memcpy(out.data(), kEqfSignature.data(), kEqfSignature.size());

    std::size_t offset = kEqfSignature.size();
    for (const auto& preset : presets) {
        const std::size_t copyLen = std::min(preset.name.size(), kEqfNameFieldSize - 1);
        std::memcpy(out.data() + offset, preset.name.data(), copyLen);
        for (int i = 0; i < kBandCount; ++i) {
            out[offset + kEqfNameFieldSize + static_cast<std::size_t>(i)] =
                static_cast<std::uint8_t>(std::clamp(preset.bands[static_cast<std::size_t>(i)], kSliderMin, kSliderMax));
        }
        out[offset + kEqfNameFieldSize + kBandCount] =
            static_cast<std::uint8_t>(std::clamp(preset.preamp, kSliderMin, kSliderMax));
        offset += kEqfRecordSize;
    }
    return out;
}

std::vector<std::uint8_t> serializeSingleEqf(const BandArray& bands, int preamp, std::string_view entryName)
{
    const EqPreset single { std::string(entryName), bands, preamp };
    return serializeEqfFile(std::span<const EqPreset>(&single, 1));
}

} // namespace wce
