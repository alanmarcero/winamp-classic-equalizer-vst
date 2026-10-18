#include "EqSkin.h"

#include "DefaultSkinData.h"

#include <algorithm>
#include <cmath>

namespace wce {
namespace {

std::vector<std::uint8_t> decompressLzss(std::span<const std::uint8_t> compressed, std::size_t outputSize)
{
    std::vector<std::uint8_t> out(outputSize, 0);
    std::size_t si = 0;
    std::size_t di = 0;
    while (si < compressed.size() && di < outputSize) {
        const int token = compressed[si++];
        if (token < 128) {
            const std::size_t length = static_cast<std::size_t>(token + 1);
            for (std::size_t k = 0; k < length && si < compressed.size() && di < outputSize; ++k)
                out[di++] = compressed[si++];
        } else {
            if (si + 1 >= compressed.size())
                break;
            const std::size_t length = static_cast<std::size_t>((token - 128) + 3);
            const std::size_t distance = (static_cast<std::size_t>(compressed[si]) << 8)
                                         | static_cast<std::size_t>(compressed[si + 1]);
            si += 2;
            for (std::size_t k = 0; k < length && di < outputSize; ++k) {
                out[di] = distance <= di ? out[di - distance] : 0;
                ++di;
            }
        }
    }
    return out;
}

SkinBitmap buildDefaultMain(bool isoLabels)
{
    SkinBitmap bmp;
    bmp.width = kSkinSheetWidth;
    bmp.height = kSkinSheetHeight;
    bmp.pixels.resize(static_cast<std::size_t>(bmp.width * bmp.height));

    auto indices = decompressLzss(detail::kEqMainCompressed, bmp.pixels.size());
    if (isoLabels) {
        constexpr int kPatchX = 80;
        constexpr int kPatchY = 101;
        constexpr int kPatchW = 174;
        constexpr int kPatchH = 8;
        const auto patch = decompressLzss(detail::kEqIsoPatchCompressed, static_cast<std::size_t>(kPatchW * kPatchH));
        for (int py = 0; py < kPatchH; ++py) {
            for (int px = 0; px < kPatchW; ++px) {
                indices[static_cast<std::size_t>((kPatchY + py) * bmp.width + (kPatchX + px))] =
                    patch[static_cast<std::size_t>(py * kPatchW + px)];
            }
        }
    }

    for (std::size_t i = 0; i < bmp.pixels.size(); ++i)
        bmp.pixels[i] = detail::kEqMainPalette[indices[i]];
    return bmp;
}

SkinBitmap buildDefaultEx()
{
    SkinBitmap bmp;
    bmp.width = kExSheetWidth;
    bmp.height = kExSheetHeight;
    bmp.pixels.resize(static_cast<std::size_t>(bmp.width * bmp.height));

    const auto indices = decompressLzss(detail::kEqExCompressed, bmp.pixels.size());
    for (std::size_t i = 0; i < bmp.pixels.size(); ++i) {
        const auto idx = std::min<std::size_t>(indices[i], detail::kEqExPalette.size() - 1);
        bmp.pixels[i] = detail::kEqExPalette[idx];
    }
    return bmp;
}

std::uint16_t readU16(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    return static_cast<std::uint16_t>(bytes[offset] | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8));
}

std::uint32_t readU32(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset])
           | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
           | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
           | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::int32_t readS32(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    return static_cast<std::int32_t>(readU32(bytes, offset));
}

} // namespace

const SkinBitmap& defaultEqMainSkin(FrequencyBands frequencies)
{
    static const SkinBitmap winampSkin = buildDefaultMain(false);
    static const SkinBitmap isoSkin = buildDefaultMain(true);
    return (frequencies == FrequencyBands::Iso) ? isoSkin : winampSkin;
}

const SkinBitmap& defaultEqExSkin()
{
    static const SkinBitmap exSkin = buildDefaultEx();
    return exSkin;
}

std::optional<SkinBitmap> parseBmp(std::span<const std::uint8_t> bytes)
{
    if (bytes.size() < 54 || bytes[0] != 'B' || bytes[1] != 'M')
        return std::nullopt;

    const std::uint32_t dataOffset = readU32(bytes, 10);
    const std::uint32_t headerSize = readU32(bytes, 14);
    if (headerSize < 40 || bytes.size() < 14 + headerSize)
        return std::nullopt;

    const std::int32_t width = readS32(bytes, 18);
    const std::int32_t rawHeight = readS32(bytes, 22);
    const bool topDown = rawHeight < 0;
    const std::int32_t height = std::abs(rawHeight);
    const std::uint16_t planes = readU16(bytes, 26);
    const std::uint16_t bpp = readU16(bytes, 28);
    const std::uint32_t compression = readU32(bytes, 30);
    const std::uint32_t colorsUsed = readU32(bytes, 46);

    if (width <= 0 || height <= 0 || width > 4096 || height > 4096 || planes != 1)
        return std::nullopt;
    if (compression != 0 && compression != 3)
        return std::nullopt;
    if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 24 && bpp != 32)
        return std::nullopt;

    std::array<Rgb, 256> palette {};
    if (bpp <= 8) {
        const std::size_t maxEntries = std::size_t(1) << bpp;
        const std::size_t count = (colorsUsed > 0 && colorsUsed <= maxEntries) ? colorsUsed : maxEntries;
        const std::size_t paletteOffset = 14 + headerSize;
        if (bytes.size() < paletteOffset + count * 4)
            return std::nullopt;
        for (std::size_t i = 0; i < count; ++i) {
            const std::size_t p = paletteOffset + i * 4;
            palette[i] = { bytes[p + 2], bytes[p + 1], bytes[p] };
        }
    }

    const std::size_t rowStride = ((static_cast<std::size_t>(width) * bpp + 31) / 32) * 4;
    if (dataOffset > bytes.size() || bytes.size() - dataOffset < rowStride * static_cast<std::size_t>(height))
        return std::nullopt;

    SkinBitmap result;
    result.width = width;
    result.height = height;
    result.pixels.resize(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        const int srcRow = topDown ? y : (height - 1 - y);
        const std::size_t rowStart = dataOffset + static_cast<std::size_t>(srcRow) * rowStride;
        for (int x = 0; x < width; ++x) {
            Rgb color {};
            if (bpp == 32) {
                const std::size_t p = rowStart + static_cast<std::size_t>(x) * 4;
                color = { bytes[p + 2], bytes[p + 1], bytes[p] };
            } else if (bpp == 24) {
                const std::size_t p = rowStart + static_cast<std::size_t>(x) * 3;
                color = { bytes[p + 2], bytes[p + 1], bytes[p] };
            } else if (bpp == 8) {
                color = palette[bytes[rowStart + static_cast<std::size_t>(x)]];
            } else if (bpp == 4) {
                const std::uint8_t byteVal = bytes[rowStart + static_cast<std::size_t>(x / 2)];
                const std::uint8_t idx = (x & 1) == 0 ? (byteVal >> 4) : (byteVal & 0x0F);
                color = palette[idx];
            } else if (bpp == 1) {
                const std::uint8_t byteVal = bytes[rowStart + static_cast<std::size_t>(x / 8)];
                const std::uint8_t idx = (byteVal >> (7 - (x & 7))) & 1;
                color = palette[idx];
            }
            result.setPixel(x, y, color);
        }
    }

    return result;
}

} // namespace wce
