#include "ConstantQEq.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using Catch::Approx;

TEST_CASE("ConstantQEq bypasses near-unity bands and excludes bands above Nyquist")
{
    wce::ConstantQEq eq;
    eq.reset(44100.0, wce::FrequencyBands::Winamp);
    eq.setBandsFromSliders(wce::kFlatBands, wce::FrequencyBands::Winamp);
    CHECK(eq.activeBandCount() == 10);
    for (int i = 0; i < wce::kBandCount; ++i)
        CHECK(eq.band(i).bypass);

    // At 22050 Hz, 12 kHz, 14 kHz, and 16 kHz have freq * 2 > 22050.
    eq.reset(22050.0, wce::FrequencyBands::Winamp);
    CHECK(eq.activeBandCount() == 7);
}

TEST_CASE("ConstantQEq pair-unrolled biquad matches across odd and even block splits")
{
    constexpr double kRate = 48000.0;
    wce::BandArray curve = wce::kFlatBands;
    curve[0] = 8;
    curve[4] = 52;
    curve[7] = 14;

    std::vector<float> singleBlock(513);
    for (std::size_t i = 0; i < singleBlock.size(); ++i)
        singleBlock[i] = 0.25f * static_cast<float>(std::sin(0.13 * static_cast<double>(i)));
    auto chunked = singleBlock;

    wce::ConstantQEq eqSingle;
    eqSingle.reset(kRate, wce::FrequencyBands::Iso);
    eqSingle.setBandsFromSliders(curve, wce::FrequencyBands::Iso);
    eqSingle.processChannel(singleBlock);

    wce::ConstantQEq eqChunked;
    eqChunked.reset(kRate, wce::FrequencyBands::Iso);
    eqChunked.setBandsFromSliders(curve, wce::FrequencyBands::Iso);
    std::span<float> full(chunked);
    eqChunked.processChannel(full.subspan(0, 1));
    eqChunked.processChannel(full.subspan(1, 7));
    eqChunked.processChannel(full.subspan(8, 250));
    eqChunked.processChannel(full.subspan(258));

    for (std::size_t i = 0; i < singleBlock.size(); ++i)
        CHECK(singleBlock[i] == Approx(chunked[i]).margin(1e-6f));
}
