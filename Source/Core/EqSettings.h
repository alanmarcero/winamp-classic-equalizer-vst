#pragma once

#include "EqConstants.h"

#include <array>

namespace wce {

// Matches WinampAttributes.h: EQ_FREQUENCIES_WINAMP / EQ_FREQUENCIES_ISO
// and EQ_TYPE_4FRONT / EQ_TYPE_CONSTANT_Q.
enum class FrequencyBands { Winamp = 0, Iso = 1 };
enum class EqType { FourFront = 0, ConstantQ = 1 };

using BandArray = std::array<int, kBandCount>;

inline constexpr BandArray kFlatBands {
    kSliderCenter, kSliderCenter, kSliderCenter, kSliderCenter, kSliderCenter,
    kSliderCenter, kSliderCenter, kSliderCenter, kSliderCenter, kSliderCenter,
};

struct EqSettings {
    bool enabled = true;        // config_use_eq (ON button)
    bool autoLoad = false;      // config_autoload_eq (AUTO button)
    int preamp = kSliderCenter; // config_preamp, 0 (+12 dB) to 63 (-12 dB)
    BandArray bands = kFlatBands;
    EqType type = EqType::FourFront;                    // config_eq_type
    FrequencyBands frequencies = FrequencyBands::Winamp; // config_eq_frequencies
    bool limiter = true;                                // config_eq_limiter
};

// The defaults from Src/Winamp/config.h and Src/Winamp/WinampAttributes.cpp,
// with the ON button active so the plugin processes audio out of the box.
inline constexpr EqSettings kWinampDefaults {};

} // namespace wce
