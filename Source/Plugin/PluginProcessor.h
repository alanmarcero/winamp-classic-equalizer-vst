#pragma once

#include "EqPresets.h"
#include "EqSkin.h"
#include "EqualizerDsp.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <optional>
#include <vector>

class EqualizerProcessor final : public juce::AudioProcessor {
public:
    EqualizerProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override { }
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override { }

    void getStateInformation(juce::MemoryBlock& destination) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // An empty buffer restores the built-in classic skin.
    bool setCustomSkinBmp(std::span<const std::uint8_t> bmpBytes);
    void clearCustomSkin();
    std::optional<wce::SkinBitmap> customSkin() const;

    void saveDefaultPreset(const wce::BandArray& bands, int preamp);
    wce::EqPreset defaultPreset() const;

    void addUserPreset(const wce::EqPreset& preset);
    bool deleteUserPreset(std::string_view name);
    std::vector<wce::EqPreset> userPresets() const;

    // Whole-number zoom of the 275 x 116 EQ window, saved with the project.
    // Default 2x matches Winamp's Doublesize mode (550 x 232).
    static constexpr int kMinScale = 1;
    static constexpr int kMaxScale = 5;
    std::atomic<int> editorScale { 2 };
    std::atomic<bool> windowshade { false };

    juce::AudioProcessorValueTreeState parameters;

private:
    wce::EqualizerDsp dsp_;

    mutable juce::CriticalSection stateLock_;
    juce::MemoryBlock customSkinBytes_;
    std::optional<wce::SkinBitmap> customSkin_;
    wce::EqPreset defaultPreset_ { "Default", wce::kFlatBands, wce::kSliderCenter };
    std::vector<wce::EqPreset> userPresets_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerProcessor)
};
