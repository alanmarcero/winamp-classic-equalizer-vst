#pragma once

#include "ConstantQEq.h"
#include "EqSettings.h"
#include "FourFrontEq.h"

#include <array>
#include <span>

namespace wce {

// Matches eq_set and eq_dosamples in Src/Winamp/In.cpp across up to 2 channels.
class EqualizerDsp {
public:
    static inline constexpr int kMaxChannels = 2;

    void prepare(double sampleRate);
    void reset();

    // Processes audio in place using the selected Winamp EQ engine (4Front or Constant-Q).
    // Passes audio through untouched when !settings.enabled or when preamp and all 10 bands are 31.
    void process(std::span<float> left, std::span<float> right, int numChannels, const EqSettings& settings);

private:
    void syncConfiguration(const EqSettings& settings);

    std::array<FourFrontEq, kMaxChannels> fourFront_ {};
    std::array<ConstantQEq, kMaxChannels> constantQ_ {};
    double sampleRate_ = 44100.0;
    double activeSampleRate_ = 0.0; // filter_srate in Src/Winamp/In.cpp (0 when bypassed)
    FrequencyBands activeFrequencies_ = FrequencyBands::Winamp;
    EqType activeType_ = EqType::FourFront;
    BandArray activeBands_ = kFlatBands;
};

} // namespace wce
