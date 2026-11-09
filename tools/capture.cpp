#include <juce_gui_basics/juce_gui_basics.h>
#include "../Source/Plugin/PluginProcessor.h"
#include <iostream>

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    EqualizerProcessor processor;
    processor.editorScale = 4;
    processor.prepareToPlay(48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    if (!editor) {
        std::cerr << "Failed to create editor\n";
        return 1;
    }
    const juce::Image snapshot = editor->createComponentSnapshot(editor->getLocalBounds(), false, 1.0f);
    juce::File file = juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "assets/native-ui.png");
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::FileOutputStream stream(file);
    if (!stream.openedOk()) {
        std::cerr << "Cannot open file\n";
        return 1;
    }
    juce::PNGImageFormat png;
    if (png.writeImageToStream(snapshot, stream)) {
        std::cout << "Saved " << file.getFullPathName() << " (" << snapshot.getWidth() << "x" << snapshot.getHeight() << ")\n";
        return 0;
    }
    return 1;
}
