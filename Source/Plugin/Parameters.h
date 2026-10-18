#pragma once

#include "EqSettings.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <algorithm>
#include <array>

namespace params {

inline constexpr auto kEnabled = "enabled";
inline constexpr auto kAutoLoad = "autoLoad";
inline constexpr auto kEqType = "eqType";
inline constexpr auto kFrequencyBands = "frequencyBands";
inline constexpr auto kLimiter = "limiter";
inline constexpr auto kPreamp = "preamp";

inline constexpr std::array<const char*, wce::kBandCount> kBands {
    "band1", "band2", "band3", "band4", "band5",
    "band6", "band7", "band8", "band9", "band10",
};

// Converts between Winamp slider position (0 = +12 dB top, 31 = 0 dB, 63 = -12 dB bottom)
// and host parameter value (0 = -12 dB, 32 = 0 dB, 63 = +12 dB).
constexpr int posToParamValue(int pos)
{
    return wce::kSliderMax - std::clamp(pos, wce::kSliderMin, wce::kSliderMax);
}

constexpr int paramValueToPos(int paramValue)
{
    return wce::kSliderMax - std::clamp(paramValue, wce::kSliderMin, wce::kSliderMax);
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

wce::EqSettings readSettings(const juce::AudioProcessorValueTreeState& state);

void applySettings(juce::AudioProcessorValueTreeState& state, const wce::EqSettings& settings);

} // namespace params
