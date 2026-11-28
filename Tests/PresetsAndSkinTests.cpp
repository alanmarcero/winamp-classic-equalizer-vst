#include "EqPresets.h"
#include "EqSkin.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("builtInPresets contains all 18 Winamp presets and .eqf round-trips")
{
    const auto& presets = wce::builtInPresets();
    CHECK(presets.size() == 18);
    CHECK(presets[0].name == "Classical");
    CHECK(presets[13].name == "Rock");
    CHECK(presets[13].bands == wce::BandArray { 19, 24, 41, 45, 38, 25, 17, 14, 14, 14 });

    const auto bytes = wce::serializeEqfFile(presets);
    CHECK(bytes.size() == wce::kEqfSignature.size() + presets.size() * wce::kEqfRecordSize);

    const auto parsed = wce::parseEqfFile(bytes);
    REQUIRE(parsed.size() == presets.size());
    for (std::size_t i = 0; i < presets.size(); ++i) {
        CHECK(parsed[i].name == presets[i].name);
        CHECK(parsed[i].bands == presets[i].bands);
        CHECK(parsed[i].preamp == presets[i].preamp);
    }
}

TEST_CASE("defaultEqMainSkin matches Eqmain.bmp dimensions and spline gradient colors")
{
    const auto& winampSkin = wce::defaultEqMainSkin(wce::FrequencyBands::Winamp);
    const auto& isoSkin = wce::defaultEqMainSkin(wce::FrequencyBands::Iso);
    const auto& exSkin = wce::defaultEqExSkin();

    CHECK(winampSkin.width == 275);
    CHECK(winampSkin.height == 315);
    CHECK(isoSkin.width == 275);
    CHECK(isoSkin.height == 315);
    CHECK(exSkin.width == 275);
    CHECK(exSkin.height == 82);

    // Top and bottom of the 1x19 spline color strip at x = 115, y = 294..312, plus preamp line at (0, 314).
    CHECK(winampSkin.pixelAt(115, 294) == wce::Rgb { 211, 34, 27 });
    CHECK(winampSkin.pixelAt(115, 312) == wce::Rgb { 42, 154, 22 });
    CHECK(winampSkin.pixelAt(0, 314) == wce::Rgb { 186, 203, 221 });
}

TEST_CASE("parseBmp rejects INT32_MIN height without signed overflow")
{
    std::array<std::uint8_t, 54> bmp {};
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[10] = 54;
    bmp[14] = 40;
    bmp[18] = 1; // width = 1
    bmp[22] = 0x00;
    bmp[23] = 0x00;
    bmp[24] = 0x00;
    bmp[25] = 0x80; // rawHeight = INT32_MIN
    bmp[26] = 1;    // planes = 1
    bmp[28] = 24;   // bpp = 24
    CHECK(!wce::parseBmp(bmp).has_value());
}

TEST_CASE("parseBmp rejects 32-bit headerSize wraparound")
{
    std::array<std::uint8_t, 54> bmp {};
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[10] = 54;
    bmp[14] = 0xF2;
    bmp[15] = 0xFF;
    bmp[16] = 0xFF;
    bmp[17] = 0xFF; // 14 + 0xFFFFFFF2 wraps uint32_t to 0
    bmp[18] = 1;
    bmp[22] = 1;
    bmp[26] = 1;
    bmp[28] = 8;
    CHECK(!wce::parseBmp(bmp).has_value());
}
