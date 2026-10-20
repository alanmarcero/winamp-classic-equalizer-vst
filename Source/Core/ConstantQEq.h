#pragma once

#include "EqConstants.h"
#include "EqSettings.h"

#include <array>
#include <span>

namespace wce {

// Matches Biquad, EqBand, and benskiQ in Src/Winamp/benskiQ/.
class ConstantQEq {
public:
    struct BiquadState {
        std::array<float, 3> sEqB { 1.0f, 2.0f, 1.0f };
        std::array<float, 3> sEqA { 1.0f, 2.0f, 1.0f };
        std::array<double, 3> zEqB { 1.0, 0.0, 0.0 };
        std::array<double, 3> zEqA { 1.0, 0.0, 0.0 };
        std::array<double, 2> xn { 0.0, 0.0 };
        std::array<double, 2> yn { 0.0, 0.0 };
        int memPos = 0;
        double sampleRate = 44100.0;
        double f0 = 1000.0;
        bool bypass = true;

        void clearBuffers();
        void transformSToZ();
        void setParameters(double freq, double gain, double q, double rate);
        double processSample(double x);
        void processBlock(std::span<float> samples);
    };

    static inline constexpr double kQ = 1.41;

    static inline constexpr std::array<double, kBandCount> kWinampFrequencies {
        70.0, 180.0, 320.0, 600.0, 1000.0, 3000.0, 6000.0, 12000.0, 14000.0, 16000.0,
    };
    static inline constexpr std::array<double, kBandCount> kIsoFrequencies {
        31.5, 63.0, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0,
    };

    static double valToGain(int pos);

    void reset(double sampleRate, FrequencyBands table);
    void setBandsFromSliders(const BandArray& bands, FrequencyBands table);
    void processChannel(std::span<float> samples);

    int activeBandCount() const { return filterTop_; }
    const BiquadState& band(int index) const { return bands_[static_cast<std::size_t>(index)]; }

private:
    std::array<BiquadState, kBandCount> bands_ {};
    double sampleRate_ = 44100.0;
    int filterTop_ = kBandCount;
};

} // namespace wce
