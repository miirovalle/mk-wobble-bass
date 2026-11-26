// Renders the real editor off-screen at 2x for the iOS marketing assets.
// Output: Assets/ui-render@2x.png (2360 x 1520 for a 1180 x 760 editor).

#include "Plugin/PluginEditor.h"
#include "Plugin/PluginProcessor.h"

#include <cstdio>

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    mk::WobbleAudioProcessor processor;
    processor.prepareToPlay (48000.0, 128);

    // A musical preset with live macro positions reads best in a thumbnail.
    processor.presets.load (4);            // Dubstep Growler
    processor.presets.loadNext (0);

    juce::AudioBuffer<float> buffer (2, 128);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 33, 1.0f), 0);

    for (int block = 0; block < 200; ++block)
    {
        buffer.clear();
        processor.processBlock (buffer, midi);
        midi.clear();
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    if (editor == nullptr)
        return 1;

    editor->setSize (1180, 760);



    constexpr int scale = 2;
    const int width = editor->getWidth() * scale;
    const int height = editor->getHeight() * scale;

    juce::Image image (juce::Image::ARGB, width, height, true);

    // Paint twice: the first pass lets JUCE settle child layout, the second is
    // the one that is saved.
    for (int pass = 0; pass < 2; ++pass)
    {
        juce::Graphics g (image);
        g.addTransform (juce::AffineTransform::scale ((float) scale));
        editor->paintEntireComponent (g, true);
    }

    const auto assets = juce::File::getCurrentWorkingDirectory().getChildFile ("Assets");
    assets.createDirectory();
    const auto output = assets.getChildFile ("ui-render@2x.png");

    juce::PNGImageFormat png;
    if (auto stream = std::unique_ptr<juce::FileOutputStream> (output.createOutputStream()))
    {
        if (png.writeImageToStream (image, *stream))
        {
            std::printf ("wrote %s (%d x %d)\n", output.getFullPathName().toRawUTF8(), width, height);
            return 0;
        }
    }

    std::printf ("failed to write %s\n", output.getFullPathName().toRawUTF8());
    return 1;
}
