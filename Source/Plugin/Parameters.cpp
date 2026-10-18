#include "Parameters.h"

#include "EqGain.h"

#include <algorithm>
#include <cstdio>
#include <ranges>

namespace params {
namespace {

constexpr std::array<const char*, wce::kBandCount> kBandNames {
    "Band 1 (70 Hz)",
    "Band 2 (180 Hz)",
    "Band 3 (320 Hz)",
    "Band 4 (600 Hz)",
    "Band 5 (1 kHz)",
    "Band 6 (3 kHz)",
    "Band 7 (6 kHz)",
    "Band 8 (12 kHz)",
    "Band 9 (14 kHz)",
    "Band 10 (16 kHz)",
};

juce::String formatSliderDb(int paramValue, int)
{
    const int pos = paramValueToPos(paramValue);
    const double db = wce::valToDb(pos);
    char buf[32] {};
    std::snprintf(buf, sizeof(buf), "%s%.1f dB", db >= 0.0 ? "+" : "", db);
    return juce::String(buf);
}

std::unique_ptr<juce::AudioParameterChoice> choice(const char* id, const char* name,
                                                   const juce::StringArray& labels, int defaultIndex)
{
    return std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { id, 1 }, name, labels, defaultIndex);
}

std::unique_ptr<juce::AudioParameterInt> sliderParam(const char* id, const char* name, int defaultPos)
{
    const auto attributes = juce::AudioParameterIntAttributes().withStringFromValueFunction(formatSliderDb);
    return std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { id, 1 }, name, wce::kSliderMin, wce::kSliderMax, posToParamValue(defaultPos), attributes);
}

int rawInt(const juce::AudioProcessorValueTreeState& state, const char* id)
{
    return static_cast<int>(state.getRawParameterValue(id)->load());
}

void setIntParam(juce::AudioProcessorValueTreeState& state, const char* id, int value)
{
    if (auto* parameter = state.getParameter(id)) {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(value)));
        parameter->endChangeGesture();
    }
}

} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    constexpr auto defaults = wce::kWinampDefaults;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kEnabled, 1 }, "Equalizer enabled", defaults.enabled));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kAutoLoad, 1 }, "Auto-load", defaults.autoLoad));
    layout.add(choice(kEqType, "EQ type", { "Winamp 4Front EQ", "Constant-Q EQ" },
                      static_cast<int>(defaults.type)));
    layout.add(choice(kFrequencyBands, "Frequency bands",
                      { "Winamp frequency bands", "ISO Standard frequency bands" },
                      static_cast<int>(defaults.frequencies)));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kLimiter, 1 }, "Use limiter", defaults.limiter));

    layout.add(sliderParam(kPreamp, "Preamp", defaults.preamp));
    std::ranges::for_each(std::views::iota(0, wce::kBandCount), [&](int i) {
        const auto idx = static_cast<std::size_t>(i);
        layout.add(sliderParam(kBands[idx], kBandNames[idx], defaults.bands[idx]));
    });

    return layout;
}

wce::EqSettings readSettings(const juce::AudioProcessorValueTreeState& state)
{
    wce::EqSettings settings;
    settings.enabled = rawInt(state, kEnabled) != 0;
    settings.autoLoad = rawInt(state, kAutoLoad) != 0;
    settings.type = static_cast<wce::EqType>(rawInt(state, kEqType));
    settings.frequencies = static_cast<wce::FrequencyBands>(rawInt(state, kFrequencyBands));
    settings.limiter = rawInt(state, kLimiter) != 0;
    settings.preamp = paramValueToPos(rawInt(state, kPreamp));
    std::ranges::for_each(std::views::iota(0, wce::kBandCount), [&](int i) {
        const auto idx = static_cast<std::size_t>(i);
        settings.bands[idx] = paramValueToPos(rawInt(state, kBands[idx]));
    });
    return settings;
}

void applySettings(juce::AudioProcessorValueTreeState& state, const wce::EqSettings& settings)
{
    setIntParam(state, kEnabled, settings.enabled ? 1 : 0);
    setIntParam(state, kAutoLoad, settings.autoLoad ? 1 : 0);
    setIntParam(state, kEqType, static_cast<int>(settings.type));
    setIntParam(state, kFrequencyBands, static_cast<int>(settings.frequencies));
    setIntParam(state, kLimiter, settings.limiter ? 1 : 0);
    setIntParam(state, kPreamp, posToParamValue(settings.preamp));
    std::ranges::for_each(std::views::iota(0, wce::kBandCount), [&](int i) {
        const auto idx = static_cast<std::size_t>(i);
        setIntParam(state, kBands[idx], posToParamValue(settings.bands[idx]));
    });
}

} // namespace params
