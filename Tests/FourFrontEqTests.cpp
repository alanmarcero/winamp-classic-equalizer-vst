#include "FourFrontEq.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

using Catch::Approx;

namespace {

std::vector<float> makeSine(double freq, double sampleRate, std::size_t count, float amplitude = 0.2f)
{
    std::vector<float> out(count);
    for (std::size_t i = 0; i < count; ++i) {
        const double phase = 2.0 * std::numbers::pi_v<double> * freq * static_cast<double>(i) / sampleRate;
        out[i] = amplitude * static_cast<float>(std::sin(phase));
    }
    return out;
}

float rmsTail(std::span<const float> signal)
{
    const auto tail = signal.subspan(signal.size() / 2);
    double sum = 0.0;
    for (float x : tail)
        sum += static_cast<double>(x) * static_cast<double>(x);
    return static_cast<float>(std::sqrt(sum / static_cast<double>(tail.size())));
}

} // namespace

TEST_CASE("FourFrontEq dbToGain and gainToDb match eq10dsp.cpp")
{
    CHECK(wce::FourFrontEq::dbToGain(0.0) == Approx(0.0));
    CHECK(wce::FourFrontEq::dbToGain(12.0) == Approx(std::pow(10.0, 12.0 / 20.0) - 1.0));
    CHECK(wce::FourFrontEq::dbToGain(-12.0) == Approx(std::pow(10.0, -12.0 / 20.0) - 1.0));
    CHECK(wce::FourFrontEq::gainToDb(wce::FourFrontEq::dbToGain(6.5)) == Approx(6.5));
}

TEST_CASE("FourFrontEq uses asymmetric Q (Q*2 for boost, Q*0.5 for cut) and disables bands near Nyquist")
{
    wce::FourFrontEq eq;
    eq.setup(44100.0, wce::FrequencyBands::Winamp);

    // Higher Q on boost means smaller alpha, so ua0 < da0.
    for (int i = 0; i < wce::kBandCount; ++i) {
        CHECK(eq.band(i).ua0 > 0.0);
        CHECK(eq.band(i).da0 > eq.band(i).ua0);
    }

    // At 32 kHz sample rate, the 16 kHz band is >= 0.499 * rate and is zeroed.
    eq.setup(32000.0, wce::FrequencyBands::Winamp);
    CHECK(eq.band(9).ua0 == 0.0);
    CHECK(eq.band(9).da0 == 0.0);
    CHECK(eq.band(8).ua0 > 0.0);
}

TEST_CASE("FourFrontEq boosts and cuts the target band frequency")
{
    constexpr double kRate = 44100.0;
    const auto dry = makeSine(1000.0, kRate, 8192, 0.15f);
    const float dryRms = rmsTail(dry);

    wce::BandArray boost1k = wce::kFlatBands;
    boost1k[4] = 0; // +12 dB at 1 kHz
    wce::FourFrontEq eqBoost;
    eqBoost.setup(kRate, wce::FrequencyBands::Winamp);
    eqBoost.setBandsFromSliders(boost1k);
    auto boosted = dry;
    eqBoost.processChannel(boosted, false);
    CHECK(rmsTail(boosted) == Approx(dryRms * std::pow(10.0f, 12.0f / 20.0f)).margin(0.02f));

    wce::BandArray cut1k = wce::kFlatBands;
    cut1k[4] = 63; // -12 dB at 1 kHz
    wce::FourFrontEq eqCut;
    eqCut.setup(kRate, wce::FrequencyBands::Winamp);
    eqCut.setBandsFromSliders(cut1k);
    auto attenuated = dry;
    eqCut.processChannel(attenuated, false);
    CHECK(rmsTail(attenuated) < dryRms * 0.35f);
}

TEST_CASE("FourFrontEq dynamic limiter trims peaks above 0.930")
{
    constexpr double kRate = 44100.0;
    wce::BandArray boostAll {};
    boostAll.fill(0); // +12 dB on all bands

    auto signalLimited = makeSine(1000.0, kRate, 4096, 0.5f);
    auto signalUnclamped = signalLimited;

    wce::FourFrontEq eqLim;
    eqLim.setup(kRate, wce::FrequencyBands::Winamp);
    eqLim.setBandsFromSliders(boostAll);
    eqLim.processChannel(signalLimited, true);

    wce::FourFrontEq eqNoLim;
    eqNoLim.setup(kRate, wce::FrequencyBands::Winamp);
    eqNoLim.setBandsFromSliders(boostAll);
    eqNoLim.processChannel(signalUnclamped, false);

    float maxLimited = 0.0f;
    for (float s : signalLimited)
        maxLimited = std::max(maxLimited, std::fabs(s));
    float maxUnclamped = 0.0f;
    for (float s : signalUnclamped)
        maxUnclamped = std::max(maxUnclamped, std::fabs(s));

    CHECK(maxLimited <= static_cast<float>(wce::FourFrontEq::kTrimCode) + 1e-5f);
    CHECK(maxUnclamped > 1.5f);
}
