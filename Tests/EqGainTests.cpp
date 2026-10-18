#include "EqGain.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("preampGain matches Winamp's eq_lookup1 table")
{
    CHECK(wce::preampGain(0) == Approx(4.0f));
    CHECK(wce::preampGain(31) == Approx(1.0f));
    CHECK(wce::preampGain(63) == Approx(0.25f));
    CHECK(wce::preampGain(-5) == Approx(4.0f));
    CHECK(wce::preampGain(100) == Approx(0.25f));
}

TEST_CASE("valToDb matches Winamp's asymmetric VALTODB mapping around 31")
{
    CHECK(wce::valToDb(0) == Approx(12.0));
    CHECK(wce::valToDb(31) == Approx(0.0));
    CHECK(wce::valToDb(63) == Approx(-12.0));

    // One step above flat (pos 30) is +12 / 31 dB; one step below flat (pos 32) is -12 / 32 dB.
    CHECK(wce::valToDb(30) == Approx(12.0 / 31.0));
    CHECK(wce::valToDb(32) == Approx(-12.0 / 32.0));
}

TEST_CASE("songtickerDb snaps near-zero values at positions 31 and 32 to 0.0 dB")
{
    CHECK(wce::songtickerDb(0) == Approx(12.0f));
    CHECK(wce::songtickerDb(31) == Approx(0.0f));
    CHECK(wce::songtickerDb(32) == Approx(0.0f));
    CHECK(wce::songtickerDb(63) == Approx(-12.0f));
}

TEST_CASE("isBypassed matches eq_set in Src/Winamp/In.cpp")
{
    wce::EqSettings settings = wce::kWinampDefaults;
    CHECK(wce::isBypassed(settings));

    settings.preamp = 20;
    CHECK_FALSE(wce::isBypassed(settings));

    settings.preamp = 31;
    settings.bands[4] = 10;
    CHECK_FALSE(wce::isBypassed(settings));

    settings.enabled = false;
    CHECK(wce::isBypassed(settings));
}
