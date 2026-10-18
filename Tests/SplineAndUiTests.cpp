#include "ClassicEqWindow.h"
#include "Spline.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("draw_eq_graphthingy spline curve matches Plush 1.2 points")
{
    const auto flat = wce::computeEqGraphPoints(wce::kFlatBands);
    for (int y : flat)
        CHECK(y == 9);

    wce::BandArray maxBoost {};
    maxBoost.fill(0);
    const auto top = wce::computeEqGraphPoints(maxBoost);
    for (int y : top)
        CHECK(y == 0);

    wce::BandArray maxCut {};
    maxCut.fill(63);
    const auto bottom = wce::computeEqGraphPoints(maxCut);
    for (int y : bottom)
        CHECK(y == 18);

    CHECK(wce::preampGraphRow(0) == 18);
    CHECK(wce::preampGraphRow(31) == 9);
    CHECK(wce::preampGraphRow(63) == 0);
}

TEST_CASE("ClassicEqWindow buttons, quick-set labels, center-snap, and cross-band sweep match Equi.cpp")
{
    wce::ClassicEqWindow window;
    wce::EqSettings settings = wce::kWinampDefaults;

    // ON button at (14..38, 18..30) toggles settings.enabled.
    window.mouseDown(20, 22, false, settings);
    const auto onUp = window.mouseUp(20, 22, false, settings);
    CHECK(onUp.settingsChanged);
    CHECK_FALSE(settings.enabled);

    // AUTO button at (39..72, 18..30) toggles settings.autoLoad.
    window.mouseDown(50, 22, false, settings);
    const auto autoUp = window.mouseUp(50, 22, false, settings);
    CHECK(autoUp.settingsChanged);
    CHECK(settings.autoLoad);

    // +12 dB quick button at (42..67, 33..42) sets all 10 bands to 0 without touching preamp.
    settings.preamp = 25;
    window.mouseDown(50, 36, false, settings);
    window.mouseUp(50, 36, false, settings);
    CHECK(settings.preamp == 25);
    for (int b : settings.bands)
        CHECK(b == 0);

    // +0 dB quick button at (42..67, 65..74) resets all 10 bands to 31.
    window.mouseDown(50, 70, false, settings);
    window.mouseUp(50, 70, false, settings);
    for (int b : settings.bands)
        CHECK(b == 31);

    // Dragging band 1 across to band 3 sweeps across bands 1, 2, and 3.
    window.mouseDown(82, 44, false, settings);
    window.mouseDrag(100, 44, false, settings);
    window.mouseDrag(118, 44, false, settings);
    window.mouseUp(118, 44, false, settings);
    CHECK(settings.bands[0] < 10);
    CHECK(settings.bands[1] < 10);
    CHECK(settings.bands[2] < 10);

    // Double-clicking the titlebar toggles Windowshade mode (height 14).
    const auto dbl = window.doubleClick(100, 7);
    CHECK(dbl.windowshadeToggled);
    CHECK(window.windowshade());
    CHECK(window.currentHeight() == wce::kShadeHeight);
}
