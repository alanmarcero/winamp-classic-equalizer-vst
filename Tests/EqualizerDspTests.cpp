#include "EqualizerDsp.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

using Catch::Approx;

TEST_CASE("EqualizerDsp passes audio untouched when bypassed and applies preamp when active")
{
    wce::EqualizerDsp dsp;
    dsp.prepare(44100.0);

    std::vector<float> left { 0.1f, -0.2f, 0.3f, -0.4f };
    std::vector<float> right { -0.1f, 0.2f, -0.3f, 0.4f };
    const auto origLeft = left;
    const auto origRight = right;

    // Default settings have enabled = true, preamp = 31, and all bands = 31 -> bypassed.
    dsp.process(left, right, 2, wce::kWinampDefaults);
    CHECK(left == origLeft);
    CHECK(right == origRight);

    // Preamp at 0 (+12 dB = 4.0x) with flat bands and limiter off scales by 4.0x.
    wce::EqSettings preampOnly = wce::kWinampDefaults;
    preampOnly.preamp = 0;
    preampOnly.limiter = false;
    dsp.process(left, right, 2, preampOnly);
    for (std::size_t i = 0; i < left.size(); ++i) {
        CHECK(left[i] == Approx(origLeft[i] * 4.0f));
        CHECK(right[i] == Approx(origRight[i] * 4.0f));
    }
}
