#pragma once

#include "EqConstants.h"
#include "EqSettings.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace wce {

using Rgb = std::array<std::uint8_t, 3>;

struct SkinBitmap {
    int width = 0;
    int height = 0;
    std::vector<Rgb> pixels;

    Rgb pixelAt(int x, int y) const
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return { 0, 0, 0 };
        return pixels[static_cast<std::size_t>(y * width + x)];
    }

    void setPixel(int x, int y, Rgb color)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return;
        pixels[static_cast<std::size_t>(y * width + x)] = color;
    }
};

// Built-in Classic Skin eqmain.bmp / eqmain_iso.bmp (275 x 315).
const SkinBitmap& defaultEqMainSkin(FrequencyBands frequencies = FrequencyBands::Winamp);

// Built-in Classic Skin eq_ex.bmp (275 x 82) for Windowshade mode.
const SkinBitmap& defaultEqExSkin();

// Reads a Windows .bmp file (1, 4, 8, 24, or 32 bpp uncompressed) for custom eqmain.bmp skins.
std::optional<SkinBitmap> parseBmp(std::span<const std::uint8_t> bytes);

} // namespace wce
