#include "PluginProcessor.h"

#include "Parameters.h"
#include "PluginEditor.h"

#include <algorithm>

namespace {

constexpr auto kSkinBmpProperty = "skinBmpBase64";
constexpr auto kScaleProperty = "scale";
constexpr auto kWindowshadeProperty = "windowshade";
constexpr auto kDefaultPresetProperty = "defaultPresetEqf";
constexpr auto kUserPresetsProperty = "userPresetsEqf";

} // namespace

EqualizerProcessor::EqualizerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "WinampClassicEq", params::createLayout())
{
}

void EqualizerProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    dsp_.prepare(sampleRate);
}

void EqualizerProcessor::releaseResources()
{
    dsp_.reset();
}

bool EqualizerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    const bool monoOrStereo = output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
    return monoOrStereo && output == layouts.getMainInputChannelSet();
}

void EqualizerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int channels = getTotalNumInputChannels();
    if (channels == 0)
        return;

    const auto samples = static_cast<std::size_t>(buffer.getNumSamples());
    std::span<float> left(buffer.getWritePointer(0), samples);
    std::span<float> right(buffer.getWritePointer(channels > 1 ? 1 : 0), samples);
    dsp_.process(left, right, std::min(channels, 2), params::readSettings(parameters));
}

juce::AudioProcessorEditor* EqualizerProcessor::createEditor()
{
    return new EqualizerEditor(*this);
}

void EqualizerProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    {
        const juce::ScopedLock lock(stateLock_);
        state.setProperty(kSkinBmpProperty, customSkinBytes_.toBase64Encoding(), nullptr);

        const auto defBytes = wce::serializeSingleEqf(defaultPreset_.bands, defaultPreset_.preamp, "Default");
        state.setProperty(kDefaultPresetProperty,
                          juce::MemoryBlock(defBytes.data(), defBytes.size()).toBase64Encoding(), nullptr);

        const auto userBytes = wce::serializeEqfFile(userPresets_);
        state.setProperty(kUserPresetsProperty,
                          juce::MemoryBlock(userBytes.data(), userBytes.size()).toBase64Encoding(), nullptr);
    }
    state.setProperty(kScaleProperty, editorScale.load(), nullptr);
    state.setProperty(kWindowshadeProperty, windowshade.load(), nullptr);
    if (const auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void EqualizerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr || !xml->hasTagName(parameters.state.getType()))
        return;

    const auto state = juce::ValueTree::fromXml(*xml);
    parameters.replaceState(state);

    const auto skinB64 = state.getProperty(kSkinBmpProperty).toString();
    if (skinB64.isNotEmpty()) {
        juce::MemoryBlock block;
        if (block.fromBase64Encoding(skinB64)) {
            setCustomSkinBmp({ static_cast<const std::uint8_t*>(block.getData()), block.getSize() });
        }
    } else {
        clearCustomSkin();
    }

    const auto defB64 = state.getProperty(kDefaultPresetProperty).toString();
    if (defB64.isNotEmpty()) {
        juce::MemoryBlock block;
        if (block.fromBase64Encoding(defB64)) {
            const auto parsed = wce::parseEqfFile({ static_cast<const std::uint8_t*>(block.getData()), block.getSize() });
            if (!parsed.empty()) {
                const juce::ScopedLock lock(stateLock_);
                defaultPreset_ = parsed.front();
            }
        }
    }

    const auto userB64 = state.getProperty(kUserPresetsProperty).toString();
    if (userB64.isNotEmpty()) {
        juce::MemoryBlock block;
        if (block.fromBase64Encoding(userB64)) {
            const juce::ScopedLock lock(stateLock_);
            userPresets_ = wce::parseEqfFile({ static_cast<const std::uint8_t*>(block.getData()), block.getSize() });
        }
    }

    const int scale = state.getProperty(kScaleProperty, editorScale.load());
    editorScale = std::clamp(scale, kMinScale, kMaxScale);
    windowshade = static_cast<bool>(state.getProperty(kWindowshadeProperty, windowshade.load()));
}

bool EqualizerProcessor::setCustomSkinBmp(std::span<const std::uint8_t> bmpBytes)
{
    if (bmpBytes.empty()) {
        clearCustomSkin();
        return true;
    }
    const auto parsed = wce::parseBmp(bmpBytes);
    if (!parsed.has_value())
        return false;

    const juce::ScopedLock lock(stateLock_);
    customSkinBytes_.replaceAll(bmpBytes.data(), bmpBytes.size());
    customSkin_ = parsed;
    return true;
}

void EqualizerProcessor::clearCustomSkin()
{
    const juce::ScopedLock lock(stateLock_);
    customSkinBytes_.reset();
    customSkin_.reset();
}

std::optional<wce::SkinBitmap> EqualizerProcessor::customSkin() const
{
    const juce::ScopedLock lock(stateLock_);
    return customSkin_;
}

void EqualizerProcessor::saveDefaultPreset(const wce::BandArray& bands, int preamp)
{
    const juce::ScopedLock lock(stateLock_);
    defaultPreset_ = { "Default", bands, preamp };
}

wce::EqPreset EqualizerProcessor::defaultPreset() const
{
    const juce::ScopedLock lock(stateLock_);
    return defaultPreset_;
}

void EqualizerProcessor::addUserPreset(const wce::EqPreset& preset)
{
    const juce::ScopedLock lock(stateLock_);
    auto it = std::ranges::find_if(userPresets_, [&](const wce::EqPreset& p) {
        return juce::String(p.name).equalsIgnoreCase(juce::String(preset.name));
    });
    if (it != userPresets_.end())
        *it = preset;
    else
        userPresets_.push_back(preset);
}

bool EqualizerProcessor::deleteUserPreset(std::string_view name)
{
    const juce::ScopedLock lock(stateLock_);
    const auto oldSize = userPresets_.size();
    std::erase_if(userPresets_, [&](const wce::EqPreset& p) {
        return juce::String(p.name).equalsIgnoreCase(juce::String(std::string(name)));
    });
    return userPresets_.size() != oldSize;
}

std::vector<wce::EqPreset> EqualizerProcessor::userPresets() const
{
    const juce::ScopedLock lock(stateLock_);
    return userPresets_;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EqualizerProcessor();
}
