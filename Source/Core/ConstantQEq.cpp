#include "ConstantQEq.h"

#include "EqGain.h"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace wce {
namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

void ConstantQEq::BiquadState::clearBuffers()
{
    xn = { 0.0, 0.0 };
    yn = { 0.0, 0.0 };
    memPos = 0;
}

void ConstantQEq::BiquadState::transformSToZ()
{
    const double invK = std::tan(f0 * kPi / sampleRate);
    const double k = 1.0 / invK;
    const double kk = k * k;

    const double b1k = sEqB[1] * k;
    const double b2kk = sEqB[2] * kk;
    const double b2kkPlusB0 = b2kk + sEqB[0];
    const double b0z = b2kkPlusB0 + b1k;
    const double b2z = b2kkPlusB0 - b1k;
    const double b1z = 2.0 * (sEqB[0] - b2kk);

    const double a1k = sEqA[1] * k;
    const double a2kk = sEqA[2] * kk;
    const double a2kkPlusA0 = a2kk + sEqA[0];
    const double a0z = a2kkPlusA0 + a1k;
    const double a2z = a2kkPlusA0 - a1k;
    const double a1z = 2.0 * (sEqA[0] - a2kk);

    const double mult = 1.0 / a0z;

    zEqB[0] = static_cast<float>(b0z * mult);
    zEqB[1] = static_cast<float>(b1z * mult);
    zEqB[2] = static_cast<float>(b2z * mult);

    zEqA[0] = 1.0f;
    zEqA[1] = static_cast<float>(a1z * mult);
    zEqA[2] = static_cast<float>(a2z * mult);
}

void ConstantQEq::BiquadState::setParameters(double freq, double gain, double q, double rate)
{
    f0 = freq;
    sampleRate = rate;

    sEqA = { 1.0f, static_cast<float>(1.0 / q), 1.0f };
    sEqB = { 1.0f, static_cast<float>(gain / q), 1.0f };

    transformSToZ();
    bypass = (std::fabs(gain - 1.0) < 0.02);
}

double ConstantQEq::BiquadState::processSample(double x)
{
    const int altPos = 1 - memPos;
    const double y = zEqB[0] * x
                     + (zEqB[1] * xn[static_cast<std::size_t>(memPos)]
                        + zEqB[2] * xn[static_cast<std::size_t>(altPos)])
                     - (zEqA[1] * yn[static_cast<std::size_t>(memPos)]
                        + zEqA[2] * yn[static_cast<std::size_t>(altPos)]);

    xn[static_cast<std::size_t>(altPos)] = x;
    yn[static_cast<std::size_t>(altPos)] = y;
    memPos = altPos;
    return y;
}

void ConstantQEq::BiquadState::processBlock(std::span<float> samples)
{
    if (samples.empty() || bypass)
        return;

    std::size_t offset = 0;
    std::size_t remaining = samples.size();

    if (memPos != 0) {
        samples[offset] = static_cast<float>(processSample(samples[offset]));
        ++offset;
        --remaining;
    }

    std::size_t half = remaining >> 1;
    if (half > 0) {
        double memX0 = xn[0];
        double memX1 = xn[1];
        double memY0 = yn[0];
        double memY1 = yn[1];

        do {
            float x = samples[offset];
            memY1 = zEqB[0] * x + (zEqB[1] * memX0 + zEqB[2] * memX1) - (zEqA[1] * memY0 + zEqA[2] * memY1);
            memX1 = x;
            samples[offset] = static_cast<float>(memY1);

            x = samples[offset + 1];
            memY0 = zEqB[0] * x + (zEqB[1] * memX1 + zEqB[2] * memX0) - (zEqA[1] * memY1 + zEqA[2] * memY0);
            memX0 = x;
            samples[offset + 1] = static_cast<float>(memY0);

            offset += 2;
            --half;
        } while (half > 0);

        xn[0] = memX0;
        xn[1] = memX1;
        yn[0] = memY0;
        yn[1] = memY1;
    }

    if ((remaining & 1) != 0)
        samples[offset] = static_cast<float>(processSample(samples[offset]));
}

double ConstantQEq::valToGain(int pos)
{
    return std::pow(10.0, valToDb(pos) / 20.0);
}

void ConstantQEq::reset(double sampleRate, FrequencyBands table)
{
    sampleRate_ = sampleRate;
    const auto& freqs = (table == FrequencyBands::Winamp) ? kWinampFrequencies : kIsoFrequencies;

    int top = 0;
    while (top < kBandCount && freqs[static_cast<std::size_t>(top)] * 2.0 <= sampleRate_)
        ++top;
    filterTop_ = top;

    std::ranges::for_each(std::views::iota(0, kBandCount), [&](int i) {
        auto& band = bands_[static_cast<std::size_t>(i)];
        band.sampleRate = sampleRate_;
        band.f0 = freqs[static_cast<std::size_t>(i)];
        band.clearBuffers();
        if (i < filterTop_)
            band.transformSToZ();
    });
}

void ConstantQEq::setBandsFromSliders(const BandArray& bands, FrequencyBands table)
{
    const auto& freqs = (table == FrequencyBands::Winamp) ? kWinampFrequencies : kIsoFrequencies;
    int top = 0;
    while (top < kBandCount && freqs[static_cast<std::size_t>(top)] * 2.0 <= sampleRate_)
        ++top;
    filterTop_ = top;

    std::ranges::for_each(std::views::iota(0, kBandCount), [&](int x) {
        const double freq = freqs[static_cast<std::size_t>(x)];
        const double gain = valToGain(bands[static_cast<std::size_t>(x)]);
        auto& bq = bands_[static_cast<std::size_t>(x)];
        bq.f0 = freq;
        bq.sampleRate = sampleRate_;
        bq.sEqA = { 1.0f, static_cast<float>(1.0 / kQ), 1.0f };
        bq.sEqB = { 1.0f, static_cast<float>(gain / kQ), 1.0f };
        if (x < filterTop_)
            bq.transformSToZ();
        bq.bypass = (std::fabs(gain - 1.0) < 0.02);
    });
}

void ConstantQEq::processChannel(std::span<float> samples)
{
    for (int x = 0; x < filterTop_; ++x)
        bands_[static_cast<std::size_t>(x)].processBlock(samples);
}

} // namespace wce
