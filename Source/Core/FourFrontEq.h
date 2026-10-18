#pragma once

#include "EqConstants.h"
#include "EqSettings.h"

#include <array>
#include <span>

namespace wce {

// Matches eq10_t and eq10_* in Src/Winamp/eq10dsp.h and Src/Winamp/eq10dsp.cpp,
// driven by eq_set_4front and eq_dosamples_4front in Src/Winamp/In.cpp.
class FourFrontEq {
public:
    struct BandState {
        double gain = 0.0;
        double ua0 = 0.0;
        double ub1 = 0.0;
        double ub2 = 0.0;
        double da0 = 0.0;
        double db1 = 0.0;
        double db2 = 0.0;
        double x1 = 0.0;
        double x2 = 0.0;
        double y1 = 0.0;
        double y2 = 0.0;
    };

    static inline constexpr double kQ = 1.41;
    static inline constexpr double kTrimCode = 0.930;
    static inline constexpr double kTrimRelease = 0.700;
    static inline constexpr double kDenormalFix = 1e-30;

    static inline constexpr std::array<double, kBandCount> kWinampFrequencies {
        70.0, 180.0, 320.0, 600.0, 1000.0, 3000.0, 6000.0, 12000.0, 14000.0, 16000.0,
    };
    static inline constexpr std::array<double, kBandCount> kIsoFrequencies {
        31.0, 62.0, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0,
    };

    static double dbToGain(double gainDb);
    static double gainToDb(double gain);

    // eq10_setup: resets filter state and calculates boost (Q * 2) and cut (Q * 0.5)
    // resonator coefficients for the given sample rate and frequency table.
    void setup(double rate, FrequencyBands table = FrequencyBands::Winamp);

    // eq10_setgain: sets internal band gain from dB.
    void setGainDb(int bandIndex, double gainDb);

    // eq_set_4front: converts each 0..63 slider via (float)VALTODB(data[x]) and calls setGainDb.
    void setBandsFromSliders(const BandArray& bands);

    // eq10_processf for a single contiguous channel buffer in place.
    void processChannel(std::span<float> samples, bool useLimiter);

    const BandState& band(int index) const { return bands_[static_cast<std::size_t>(index)]; }
    double detector() const { return detect_; }

private:
    static void setupBandDirection(int u, double rate, BandState& band, double freq, double q);
    static void setupBand(double rate, BandState& band, double freq, double q);

    std::array<BandState, kBandCount> bands_ {};
    double detect_ = 0.0;
    double detectDecay_ = 0.0;
};

} // namespace wce
