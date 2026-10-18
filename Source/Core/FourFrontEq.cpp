#include "FourFrontEq.h"

#include "EqGain.h"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace wce {
namespace {

constexpr double kPi = 3.1415926535897932384626433832795;

} // namespace

double FourFrontEq::dbToGain(double gainDb)
{
    return std::pow(10.0, gainDb / 20.0) - 1.0;
}

double FourFrontEq::gainToDb(double gain)
{
    return 20.0 * std::log10(gain + 1.0);
}

void FourFrontEq::setupBandDirection(int u, double rate, BandState& band, double freq, double q)
{
    if (rate < 4000.0)
        rate = 4000.0;
    if (rate > 384000.0)
        rate = 384000.0;
    if (freq < 20.0)
        freq = 20.0;
    if (freq >= (rate * 0.499)) {
        band.ua0 = 0.0;
        band.da0 = 0.0;
        return;
    }

    const double angle = 2.0 * kPi * freq / rate;
    const double alpha = std::sin(angle) / (2.0 * q);

    const double b0 = 1.0 / (1.0 + alpha);
    const double a0 = b0 * alpha;
    const double b1 = b0 * 2.0 * std::cos(angle);
    const double b2 = b0 * (alpha - 1.0);

    if (u > 0) {
        band.ua0 = a0;
        band.ub1 = b1;
        band.ub2 = b2;
    } else {
        band.da0 = a0;
        band.db1 = b1;
        band.db2 = b2;
    }
}

void FourFrontEq::setupBand(double rate, BandState& band, double freq, double q)
{
    band = BandState {};
    setupBandDirection(-1, rate, band, freq, q * 0.5);
    setupBandDirection(1, rate, band, freq, q * 2.0);
}

void FourFrontEq::setup(double rate, FrequencyBands table)
{
    const auto& freqs = (table == FrequencyBands::Winamp) ? kWinampFrequencies : kIsoFrequencies;
    std::ranges::for_each(std::views::iota(0, kBandCount), [&](int t) {
        setupBand(rate, bands_[static_cast<std::size_t>(t)], freqs[static_cast<std::size_t>(t)], kQ);
    });
    detect_ = 0.0;
    detectDecay_ = std::pow(0.001, 1.0 / (rate * kTrimRelease));
}

void FourFrontEq::setGainDb(int bandIndex, double gainDb)
{
    if (bandIndex < 0 || bandIndex >= kBandCount)
        return;
    bands_[static_cast<std::size_t>(bandIndex)].gain = dbToGain(gainDb);
}

void FourFrontEq::setBandsFromSliders(const BandArray& bands)
{
    std::ranges::for_each(std::views::iota(0, kBandCount), [&](int x) {
        // eq_set_4front in Src/Winamp/In.cpp stores (float)VALTODB(data[x]) in eqt[x].
        const float eqDb = static_cast<float>(valToDb(bands[static_cast<std::size_t>(x)]));
        setGainDb(x, eqDb);
    });
}

void FourFrontEq::processChannel(std::span<float> samples, bool useLimiter)
{
    for (int k = 0; k < kBandCount; ++k) {
        auto& band = bands_[static_cast<std::size_t>(k)];
        double x1 = band.x1;
        double x2 = band.x2;
        double y1 = band.y1;
        double y2 = band.y2;
        const double gain = band.gain;

        double a0 = 0.0;
        double b1 = 0.0;
        double b2 = 0.0;
        if (gain > 0.0) {
            a0 = band.ua0 * gain;
            b1 = band.ub1;
            b2 = band.ub2;
        } else {
            a0 = band.da0 * gain;
            b1 = band.db1;
            b2 = band.db2;
        }

        if (a0 == 0.0)
            continue;

        for (float& sample : samples) {
            const double in0 = sample;
            const double y0 = (in0 - x2) * a0 + y1 * b1 + y2 * b2 + kDenormalFix;
            x2 = x1;
            x1 = in0;
            y2 = y1;
            y1 = y0;
            sample = static_cast<float>(y0 + in0);
        }

        band.x1 = x1;
        band.x2 = x2;
        band.y1 = y1;
        band.y2 = y2;
    }

    if (useLimiter) {
        double detect = detect_;
        const double detectDecay = detectDecay_;
        for (float& sample : samples) {
            const double absIn = std::fabs(static_cast<double>(sample));
            if (absIn > detect)
                detect = absIn;
            if (detect > kTrimCode)
                sample = sample * static_cast<float>(kTrimCode / detect);
            detect *= detectDecay;
            detect += kDenormalFix;
        }
        detect_ = detect;
    }
}

} // namespace wce
