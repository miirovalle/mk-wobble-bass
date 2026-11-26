// Headless wrapper test: drives the real AudioProcessor through prepare, MIDI,
// processBlock, preset loading and state round-trips, then renders the editor
// offscreen to a PNG so layout changes can be reviewed without a DAW.

#include "Plugin/PluginEditor.h"
#include "Plugin/PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{

int failures = 0;

void check (bool condition, const char* message)
{
    if (! condition)
    {
        std::printf ("FAIL: %s\n", message);
        ++failures;
    }
}

double rms (const juce::AudioBuffer<float>& buffer)
{
    double sum = 0.0;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            sum += (double) buffer.getSample (channel, i) * buffer.getSample (channel, i);

    return std::sqrt (sum / (double) juce::jmax (1, buffer.getNumChannels() * buffer.getNumSamples()));
}

} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;

    mk::WobbleAudioProcessor processor;
    processor.prepareToPlay (48000.0, 128);

    juce::AudioBuffer<float> buffer (2, 128);
    juce::MidiBuffer midi;

    // Idle must be true silence.
    for (int block = 0; block < 8; ++block)
    {
        buffer.clear();
        processor.processBlock (buffer, midi);
    }
    check (rms (buffer) == 0.0, "wrapper is silent while idle");

    // Note on, render, measure.
    midi.addEvent (juce::MidiMessage::noteOn (1, 33, 0.9f), 0);
    double peak = 0.0;
    for (int block = 0; block < 120; ++block)
    {
        buffer.clear();
        processor.processBlock (buffer, midi);
        midi.clear();
        peak = juce::jmax (peak, (double) buffer.getMagnitude (0, buffer.getNumSamples()));
    }
    check (peak > 0.02, "wrapper produces audio for a note");
    check (peak <= 1.0, "wrapper output stays inside full scale");

    // Host automation moves a parameter and the sound responds.
    if (auto* parameter = processor.apvts.getParameter (mk::paramId ((int) mk::P::fltCutoff)))
        parameter->setValueNotifyingHost (0.05f);

    for (int block = 0; block < 40; ++block)
    {
        buffer.clear();
        processor.processBlock (buffer, midi);
    }
    check (rms (buffer) >= 0.0, "automation does not produce NaN");

    // Presets: every factory preset loads and renders.
    int checkedPresets = 0;
    for (int i = 0; i < mk::factoryPresetCount(); ++i)
    {
        processor.presets.load (i);
        processor.presets.loadNext (0);

        midi.addEvent (juce::MidiMessage::noteOn (1, 33, 1.0f), 0);
        double presetPeak = 0.0;
        for (int block = 0; block < 40; ++block)
        {
            buffer.clear();
            processor.processBlock (buffer, midi);
            midi.clear();
            presetPeak = juce::jmax (presetPeak, (double) buffer.getMagnitude (0, buffer.getNumSamples()));
        }
        midi.addEvent (juce::MidiMessage::noteOff (1, 33), 0);
        buffer.clear();
        processor.processBlock (buffer, midi);
        midi.clear();

        if (! std::isfinite (presetPeak) || presetPeak > 1.0001)
            std::printf ("preset %d (%s) peak %.3f\n", i, mk::factoryPreset (i).name, presetPeak);
        check (std::isfinite (presetPeak) && presetPeak <= 1.0001, mk::factoryPreset (i).name);
        ++checkedPresets;
    }
    check (checkedPresets >= 50, "factory preset count");

    // In-plug-in keyboard: the on-screen keys drive MidiKeyboardState, which the
    // processor merges into the MIDI stream. Verify that path makes sound.
    {
        processor.engine.allNotesOff (true);
        processor.keyboardState.noteOn (1, 48, 0.9f);
        double keyPeak = 0.0;
        for (int block = 0; block < 60; ++block)
        {
            buffer.clear();
            processor.processBlock (buffer, midi);
            midi.clear();
            keyPeak = juce::jmax (keyPeak, (double) buffer.getMagnitude (0, buffer.getNumSamples()));
        }
        processor.keyboardState.noteOff (1, 48, 0.0f);
        check (keyPeak > 0.02, "on-screen keyboard plays the engine");
    }

    // Built-in piano roll: one programmed note must make sound.
    {
        processor.engine.allNotesOff (true);
        processor.sequencer.clear();
        processor.sequencer.set (0, 0, true);
        processor.sequencer.setPlaying (true);

        double seqPeak = 0.0;
        for (int block = 0; block < 200; ++block)
        {
            buffer.clear();
            midi.clear();
            processor.processBlock (buffer, midi);
            seqPeak = juce::jmax (seqPeak, (double) buffer.getMagnitude (0, buffer.getNumSamples()));
        }

        processor.sequencer.setPlaying (false);
        check (seqPeak > 0.02, "piano roll sequencer plays the engine");
    }

    // State round-trip.
    juce::MemoryBlock state;
    processor.getStateInformation (state);
    processor.presets.loadDefault();
    processor.setStateInformation (state.getData(), (int) state.getSize());

    midi.addEvent (juce::MidiMessage::noteOn (1, 40, 0.8f), 0);
    for (int block = 0; block < 20; ++block)
    {
        buffer.clear();
        processor.processBlock (buffer, midi);
        midi.clear();
    }
    check (rms (buffer) > 0.002, "restored state still makes sound");

    // Off-screen render of the editor, then every page via its tab button.
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    check (editor != nullptr, "editor is created");

    if (editor != nullptr)
    {
        editor->setSize (1180, 760);

        const auto renderTo = [] (juce::Component& component, const juce::String& fileName)
        {
            juce::Image image (juce::Image::ARGB, component.getWidth(), component.getHeight(), true);
            {
                juce::Graphics g (image);
                component.paintEntireComponent (g, true);
            }

            const auto output = juce::File::getCurrentWorkingDirectory().getChildFile (fileName);
            juce::PNGImageFormat png;
            if (auto stream = std::unique_ptr<juce::FileOutputStream> (output.createOutputStream()))
                png.writeImageToStream (image, *stream);

            std::printf ("snapshot: %s\n", output.getFullPathName().toRawUTF8());
        };

        renderTo (*editor, "editor.png");
        renderTo (*editor, "editor_after.png");


        // A musical preset plus a held note so the scope and meter show data.
        processor.presets.load (1);
        processor.presets.loadNext (0);

        processor.engine.noteOn (33, 1.0f);
        juce::AudioBuffer<float> snapshotBuffer (2, 128);
        juce::MidiBuffer emptyMidi;
        for (int block = 0; block < 60; ++block)
        {
            snapshotBuffer.clear();
            processor.processBlock (snapshotBuffer, emptyMidi);
        }

        for (auto* child : editor->getChildren())
        {
            if (auto* button = dynamic_cast<juce::TextButton*> (child))
            {
                if (button->getButtonText() == "ADVANCED")
                {
                    if (button->onClick != nullptr) button->onClick();
                    renderTo (*editor, "editor_advanced.png");
                    if (button->onClick != nullptr) button->onClick();
                }
            }
        }
    }

    if (failures == 0)
        std::printf ("PASS: wrapper audio, automation, %d presets, state, editor\n", checkedPresets);
    else
        std::printf ("%d failure(s)\n", failures);

    juce::ignoreUnused (argc, argv);
    return failures == 0 ? 0 : 1;
}
