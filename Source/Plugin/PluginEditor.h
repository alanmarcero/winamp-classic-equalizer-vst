#pragma once

#include "ClassicEqWindow.h"
#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

// The 275 x 116 (or 275 x 14 windowshade) Winamp Equalizer window at a whole-number zoom,
// drawn with nearest-neighbour pixels. Right-clicking or selecting the Settings button
// opens the Equalizer options menu; selecting the PRESETS button opens Winamp's presets menu.
class EqualizerEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit EqualizerEditor(EqualizerProcessor& owner);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    juce::Point<int> toWinampCoords(juce::Point<int> localPos) const;
    void syncFromProcessor(bool forceRender = false);
    void applySliderChange(int activeSlider, const wce::EqSettings& updated);
    void renderImage();

    void showPresetsMenu();
    void showOptionsMenu();
    void addChoiceMenu(juce::PopupMenu& menu, const char* parameterId, const juce::String& title);
    void setParameterIndex(const char* parameterId, int index);
    void applyPreset(const wce::EqPreset& preset);

    void loadEqfFile();
    void saveEqfFile(bool addToLibrary);
    void chooseSkinBmp();
    void setScale(int scale);
    void updateEditorSize();

    EqualizerProcessor& processor_;
    wce::ClassicEqWindow eqWindow_;
    wce::EqSettings lastSettings_ {};
    juce::Image image_ { juce::Image::RGB, wce::kWindowWidth, wce::kWindowHeight, true };
    std::unique_ptr<juce::FileChooser> chooser_;
    juce::TextButton settingsButton_ { "Settings" };
    bool dragging_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerEditor)
};
