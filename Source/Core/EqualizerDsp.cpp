#include "EqualizerDsp.h"

#include "EqGain.h"

#include <algorithm>
#include <ranges>

namespace wce {

void EqualizerDsp::prepare(double sampleRate)
{
    sampleRate_ = sampleRate;
    activeSampleRate_ = 0.0;
}

void EqualizerDsp::reset()
{
    activeSampleRate_ = 0.0;
}

void EqualizerDsp::syncConfiguration(const EqSettings& settings)
{
    const bool needsSetup = (activeSampleRate_ != sampleRate_)
                            || (activeFrequencies_ != settings.frequencies)
                            || (activeType_ != settings.type);
    const bool bandsChanged = needsSetup || (activeBands_ != settings.bands);

    if (needsSetup) {
        // eq_dosamples_4front passes (float)srate to eq10_setup.
        const double rateFor4Front = static_cast<double>(static_cast<float>(sampleRate_));
        for (int ch = 0; ch < kMaxChannels; ++ch) {
            fourFront_[static_cast<std::size_t>(ch)].setup(rateFor4Front, settings.frequencies);
            constantQ_[static_cast<std::size_t>(ch)].reset(sampleRate_, settings.frequencies);
        }
        activeSampleRate_ = sampleRate_;
        activeFrequencies_ = settings.frequencies;
        activeType_ = settings.type;
    }

    if (bandsChanged) {
        for (int ch = 0; ch < kMaxChannels; ++ch) {
            fourFront_[static_cast<std::size_t>(ch)].setBandsFromSliders(settings.bands);
            constantQ_[static_cast<std::size_t>(ch)].setBandsFromSliders(settings.bands, settings.frequencies);
        }
        activeBands_ = settings.bands;
    }
}

void EqualizerDsp::process(std::span<float> left, std::span<float> right, int numChannels, const EqSettings& settings)
{
    const int channels = std::clamp(numChannels, 0, kMaxChannels);
    if (channels == 0 || left.empty())
        return;

    // In Src/Winamp/In.cpp and benskiQ.cpp, when !filter_enabled, filter_srate is set to 0
    // and samples pass through untouched.
    if (isBypassed(settings)) {
        activeSampleRate_ = 0.0;
        return;
    }

    syncConfiguration(settings);

    const float preamp = preampGain(settings.preamp);
    std::array<std::span<float>, kMaxChannels> channelSpans { left, right };

    for (int ch = 0; ch < channels; ++ch) {
        auto span = channelSpans[static_cast<std::size_t>(ch)];
        for (float& sample : span)
            sample *= preamp;

        if (settings.type == EqType::FourFront)
            fourFront_[static_cast<std::size_t>(ch)].processChannel(span, settings.limiter);
        else
            constantQ_[static_cast<std::size_t>(ch)].processChannel(span);
    }
}

} // namespace wce
