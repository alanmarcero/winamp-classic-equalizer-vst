#include "PluginEditor.h"

#include "Parameters.h"

#include <algorithm>
#include <ranges>

namespace {

constexpr int kButtonHeight = 24;
constexpr int kUiRefreshHz = 30;

} // namespace

EqualizerEditor::EqualizerEditor(EqualizerProcessor& owner)
    : AudioProcessorEditor(owner)
    , processor_(owner)
{
    settingsButton_.onClick = [this] { showOptionsMenu(); };
    addAndMakeVisible(settingsButton_);

    versionLabel_.setText("v1.1.0", juce::dontSendNotification);
    versionLabel_.setJustificationType(juce::Justification::centred);
    versionLabel_.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    versionLabel_.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(versionLabel_);

    eqWindow_.setCustomSkin(processor_.customSkin());
    eqWindow_.setWindowshade(processor_.windowshade.load());
    lastSettings_ = params::readSettings(processor_.parameters);
    eqWindow_.render(lastSettings_, true);
    renderImage();
    updateEditorSize();

    startTimerHz(kUiRefreshHz);
}

void EqualizerEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);

    const int h = eqWindow_.currentHeight();
    const auto destBounds = getLocalBounds().withTrimmedBottom(kButtonHeight);
    g.drawImage(image_,
                0, 0, destBounds.getWidth(), destBounds.getHeight(),
                0, 0, wce::kWindowWidth, h);
}

void EqualizerEditor::resized()
{
    auto bottomBar = getLocalBounds().removeFromBottom(kButtonHeight);
    versionLabel_.setBounds(bottomBar.removeFromRight(60));
    settingsButton_.setBounds(bottomBar);
}

juce::Point<int> EqualizerEditor::toWinampCoords(juce::Point<int> localPos) const
{
    const int scale = std::max(1, processor_.editorScale.load());
    return {
        std::clamp(localPos.x / scale, 0, wce::kWindowWidth - 1),
        std::clamp(localPos.y / scale, 0, eqWindow_.currentHeight() - 1),
    };
}

void EqualizerEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu()) {
        showOptionsMenu();
        return;
    }

    const int canvasPixelHeight = eqWindow_.currentHeight() * processor_.editorScale.load();
    if (event.position.y >= static_cast<float>(canvasPixelHeight))
        return;

    dragging_ = true;
    auto settings = params::readSettings(processor_.parameters);
    const auto pt = toWinampCoords(event.getPosition());
    const auto result = eqWindow_.mouseDown(pt.x, pt.y, event.mods.isShiftDown(), settings);

    if (result.settingsChanged)
        applySliderChange(result.activeSlider, settings);
    if (!result.statusText.empty())
        settingsButton_.setButtonText(result.statusText);

    lastSettings_ = settings;
    renderImage();
    repaint();
}

void EqualizerEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (!dragging_ || event.mods.isPopupMenu())
        return;

    auto settings = params::readSettings(processor_.parameters);
    const auto pt = toWinampCoords(event.getPosition());
    const auto result = eqWindow_.mouseDrag(pt.x, pt.y, event.mods.isShiftDown(), settings);

    if (result.settingsChanged)
        applySliderChange(result.activeSlider, settings);
    if (!result.statusText.empty())
        settingsButton_.setButtonText(result.statusText);
    else
        settingsButton_.setButtonText("Settings");

    lastSettings_ = settings;
    renderImage();
    repaint();
}

void EqualizerEditor::mouseUp(const juce::MouseEvent& event)
{
    if (!dragging_)
        return;
    dragging_ = false;
    settingsButton_.setButtonText("Settings");

    auto settings = params::readSettings(processor_.parameters);
    const auto pt = toWinampCoords(event.getPosition());
    const auto result = eqWindow_.mouseUp(pt.x, pt.y, event.mods.isShiftDown(), settings);

    if (result.settingsChanged)
        params::applySettings(processor_.parameters, settings);

    if (result.windowshadeToggled) {
        processor_.windowshade = eqWindow_.windowshade();
        eqWindow_.render(settings, true);
        updateEditorSize();
    }

    lastSettings_ = settings;
    renderImage();
    repaint();

    if (result.openPresetsMenu)
        showPresetsMenu();
}

void EqualizerEditor::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
        return;

    const auto pt = toWinampCoords(event.getPosition());
    const auto result = eqWindow_.doubleClick(pt.x, pt.y);
    if (result.windowshadeToggled) {
        processor_.windowshade = eqWindow_.windowshade();
        lastSettings_ = params::readSettings(processor_.parameters);
        eqWindow_.render(lastSettings_, true);
        renderImage();
        updateEditorSize();
        repaint();
    }
}

void EqualizerEditor::timerCallback()
{
    if (dragging_)
        return;
    syncFromProcessor(false);
}

void EqualizerEditor::syncFromProcessor(bool forceRender)
{
    const auto current = params::readSettings(processor_.parameters);
    if (!forceRender && current.enabled == lastSettings_.enabled
        && current.autoLoad == lastSettings_.autoLoad
        && current.preamp == lastSettings_.preamp
        && current.bands == lastSettings_.bands
        && current.type == lastSettings_.type
        && current.frequencies == lastSettings_.frequencies
        && current.limiter == lastSettings_.limiter) {
        return;
    }

    lastSettings_ = current;
    eqWindow_.render(lastSettings_, true);
    renderImage();
    repaint();
}

void EqualizerEditor::applySliderChange(int activeSlider, const wce::EqSettings& updated)
{
    if (activeSlider == 0) {
        setParameterIndex(params::kPreamp, params::posToParamValue(updated.preamp));
    } else if (activeSlider >= 1 && activeSlider <= wce::kBandCount) {
        const auto idx = static_cast<std::size_t>(activeSlider - 1);
        setParameterIndex(params::kBands[idx], params::posToParamValue(updated.bands[idx]));
    } else {
        params::applySettings(processor_.parameters, updated);
    }
}

void EqualizerEditor::renderImage()
{
    const juce::Image::BitmapData pixels(image_, juce::Image::BitmapData::writeOnly);
    std::ranges::for_each(std::views::iota(0, wce::kWindowHeight), [&](int y) {
        std::ranges::for_each(std::views::iota(0, wce::kWindowWidth), [&](int x) {
            const auto rgb = eqWindow_.pixelAt(x, y);
            pixels.setPixelColour(x, y, juce::Colour(rgb[0], rgb[1], rgb[2]));
        });
    });
}

// Mirrors the Presets popup menu in Src/Winamp/Winamp.rc (EQpresets) and Src/Winamp/Eq.cpp.
void EqualizerEditor::showPresetsMenu()
{
    juce::PopupMenu menu;

    juce::PopupMenu loadPresetMenu;
    for (const auto& preset : wce::builtInPresets()) {
        loadPresetMenu.addItem(preset.name, [this, preset] { applyPreset(preset); });
    }
    const auto userPresets = processor_.userPresets();
    if (!userPresets.empty()) {
        loadPresetMenu.addSeparator();
        for (const auto& preset : userPresets) {
            loadPresetMenu.addItem(preset.name, [this, preset] { applyPreset(preset); });
        }
    }

    juce::PopupMenu loadMenu;
    loadMenu.addSubMenu("Preset", loadPresetMenu);
    loadMenu.addItem("Default", [this] { applyPreset(processor_.defaultPreset()); });
    loadMenu.addSeparator();
    loadMenu.addItem("From EQF...", [this] { loadEqfFile(); });
    menu.addSubMenu("Load", loadMenu);

    juce::PopupMenu saveMenu;
    saveMenu.addItem("Preset...", [this] { saveEqfFile(true); });
    saveMenu.addItem("Default", [this] {
        const auto current = params::readSettings(processor_.parameters);
        processor_.saveDefaultPreset(current.bands, current.preamp);
    });
    saveMenu.addSeparator();
    saveMenu.addItem("To EQF...", [this] { saveEqfFile(false); });
    menu.addSubMenu("Save", saveMenu);

    juce::PopupMenu deleteMenu;
    if (userPresets.empty()) {
        deleteMenu.addItem("No saved presets", false, false, nullptr);
    } else {
        for (const auto& preset : userPresets) {
            deleteMenu.addItem(preset.name, [this, name = preset.name] { processor_.deleteUserPreset(name); });
        }
    }
    menu.addSubMenu("Delete", deleteMenu);

    const int scale = processor_.editorScale.load();
    const auto anchor = localPointToGlobal(juce::Point<int>(218 * scale, 19 * scale));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ anchor.x, anchor.y, 1, 1 }));
}

void EqualizerEditor::showOptionsMenu()
{
    const auto settings = params::readSettings(processor_.parameters);
    juce::PopupMenu menu;

    menu.addItem("Equalizer enabled (ON)", true, settings.enabled, [this, enabled = settings.enabled] {
        setParameterIndex(params::kEnabled, enabled ? 0 : 1);
    });
    menu.addItem("Auto-load (AUTO)", true, settings.autoLoad, [this, autoLoad = settings.autoLoad] {
        setParameterIndex(params::kAutoLoad, autoLoad ? 0 : 1);
    });
    menu.addSeparator();

    addChoiceMenu(menu, params::kEqType, "EQ type");
    addChoiceMenu(menu, params::kFrequencyBands, "Frequency bands");
    menu.addItem("Use limiter", true, settings.limiter, [this, limiter = settings.limiter] {
        setParameterIndex(params::kLimiter, limiter ? 0 : 1);
    });
    menu.addItem("Windowshade mode", true, eqWindow_.windowshade(), [this] {
        eqWindow_.setWindowshade(!eqWindow_.windowshade());
        processor_.windowshade = eqWindow_.windowshade();
        syncFromProcessor(true);
        updateEditorSize();
    });
    menu.addSeparator();

    juce::PopupMenu presets;
    for (const auto& preset : wce::builtInPresets()) {
        presets.addItem(preset.name, [this, preset] { applyPreset(preset); });
    }
    const auto userPresets = processor_.userPresets();
    if (!userPresets.empty()) {
        presets.addSeparator();
        for (const auto& preset : userPresets) {
            presets.addItem(preset.name, [this, preset] { applyPreset(preset); });
        }
    }
    presets.addSeparator();
    presets.addItem("Load from EQF...", [this] { loadEqfFile(); });
    presets.addItem("Save to EQF...", [this] { saveEqfFile(true); });
    menu.addSubMenu("Presets", presets);

    juce::PopupMenu skin;
    skin.addItem("Load eqmain.bmp...", [this] { chooseSkinBmp(); });
    skin.addItem("Default skin", [this] {
        processor_.clearCustomSkin();
        eqWindow_.setCustomSkin(std::nullopt);
        syncFromProcessor(true);
    });
    menu.addSubMenu("Skin", skin);

    juce::PopupMenu size;
    std::ranges::for_each(
        std::views::iota(EqualizerProcessor::kMinScale, EqualizerProcessor::kMaxScale + 1), [&](int scale) {
            size.addItem(juce::String(scale) + "x", true, processor_.editorScale == scale,
                         [this, scale] { setScale(scale); });
        });
    menu.addSubMenu("Size", size);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition());
}

void EqualizerEditor::addChoiceMenu(juce::PopupMenu& menu, const char* parameterId, const juce::String& title)
{
    const auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(processor_.parameters.getParameter(parameterId));
    juce::PopupMenu submenu;
    std::ranges::for_each(std::views::iota(0, parameter->choices.size()), [&](int index) {
        submenu.addItem(parameter->choices[index], true, parameter->getIndex() == index,
                        [this, parameterId, index] { setParameterIndex(parameterId, index); });
    });
    menu.addSubMenu(title, submenu);
}

void EqualizerEditor::setParameterIndex(const char* parameterId, int index)
{
    auto* parameter = processor_.parameters.getParameter(parameterId);
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(index)));
    parameter->endChangeGesture();
}

void EqualizerEditor::applyPreset(const wce::EqPreset& preset)
{
    auto settings = params::readSettings(processor_.parameters);
    settings.preamp = preset.preamp;
    settings.bands = preset.bands;
    params::applySettings(processor_.parameters, settings);
    syncFromProcessor(true);
}

void EqualizerEditor::loadEqfFile()
{
    chooser_ = std::make_unique<juce::FileChooser>("Load EQ Preset (.eqf, .q1)", juce::File {}, "*.eqf;*.q1");
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this](const juce::FileChooser& chooser) {
                              const auto file = chooser.getResult();
                              if (file == juce::File {})
                                  return;
                              juce::MemoryBlock block;
                              if (!file.loadFileAsData(block))
                                  return;
                              const auto presets = wce::parseEqfFile(
                                  { static_cast<const std::uint8_t*>(block.getData()), block.getSize() });
                              if (presets.empty())
                                  return;
                              applyPreset(presets.front());
                              for (const auto& preset : presets) {
                                  const auto name = (presets.size() == 1 && preset.name == "Entry1")
                                                        ? file.getFileNameWithoutExtension().toStdString()
                                                        : preset.name;
                                  processor_.addUserPreset({ name, preset.bands, preset.preamp });
                              }
                          });
}

void EqualizerEditor::saveEqfFile(bool addToLibrary)
{
    chooser_ = std::make_unique<juce::FileChooser>("Save EQ Preset (.eqf)", juce::File {}, "*.eqf");
    chooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, addToLibrary](const juce::FileChooser& chooser) {
                              const auto file = chooser.getResult();
                              if (file == juce::File {})
                                  return;
                              const auto current = params::readSettings(processor_.parameters);
                              const auto bytes = wce::serializeSingleEqf(current.bands, current.preamp, "Entry1");
                              file.replaceWithData(bytes.data(), bytes.size());
                              if (addToLibrary) {
                                  const auto name = file.getFileNameWithoutExtension().toStdString();
                                  if (!name.empty())
                                      processor_.addUserPreset({ name, current.bands, current.preamp });
                              }
                          });
}

void EqualizerEditor::chooseSkinBmp()
{
    chooser_ = std::make_unique<juce::FileChooser>("Load eqmain.bmp", juce::File {}, "*.bmp");
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this](const juce::FileChooser& chooser) {
                              const auto file = chooser.getResult();
                              if (file == juce::File {})
                                  return;
                              juce::MemoryBlock block;
                              if (!file.loadFileAsData(block))
                                  return;
                              if (processor_.setCustomSkinBmp(
                                      { static_cast<const std::uint8_t*>(block.getData()), block.getSize() })) {
                                  eqWindow_.setCustomSkin(processor_.customSkin());
                                  syncFromProcessor(true);
                              }
                          });
}

void EqualizerEditor::setScale(int scale)
{
    processor_.editorScale = scale;
    updateEditorSize();
}

void EqualizerEditor::updateEditorSize()
{
    const int scale = processor_.editorScale.load();
    setSize(wce::kWindowWidth * scale, eqWindow_.currentHeight() * scale + kButtonHeight);
}
