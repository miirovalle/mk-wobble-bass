#pragma once

#include "DSP/Engine.h"
#include "DSP/Presets.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <atomic>
#include <vector>

namespace mk
{

struct PresetEntry
{
    juce::String name;
    juce::String category;
    bool isFactory = true;
    int factoryIndex = -1;
    juce::File file;
};

class PresetManager
{
public:
    PresetManager (juce::AudioProcessorValueTreeState& stateToUse);

    void refresh();
    const std::vector<PresetEntry>& entries() const noexcept { return presets; }

    bool load (int index);
    bool saveAs (const juce::String& name, const juce::String& category);
    bool remove (int index);
    bool importFile (const juce::File& file);
    bool exportFile (int index, const juce::File& file);

    void toggleFavorite (const juce::String& name);
    bool isFavorite (const juce::String& name) const;

    void loadNext (int direction);
    void loadDefault();
    void setName (const juce::String& newName, const juce::String& newCategory);

    const juce::String& currentName() const noexcept { return name; }
    juce::String currentCategory() const noexcept { return category; }
    int currentIndex() const noexcept { return current; }
    bool isCurrentUserPreset() const noexcept { return currentIsUser; }

    juce::File presetFolder() const;

    std::function<void()> onPresetChanged;

private:
    void applySnapshot (const ParamSnapshot& snapshot);
    void readFavorites();
    void writeFavorites();

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetEntry> presets;
    juce::StringArray favorites;

    juce::String name { "Init" };
    juce::String category { "Init" };
    int current = -1;
    bool currentIsUser = false;
};

class WobbleAudioProcessor final : public juce::AudioProcessor
{
public:
    WobbleAudioProcessor();
    ~WobbleAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
#if defined(JucePlugin_Name)
        return JucePlugin_Name;
#else
        return "WOBBLE MACHINE";
#endif
    }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return juce::jmax (1, factoryPresetCount()); }
    int getCurrentProgram() override { return juce::jmax (0, presets.currentIndex()); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    Engine engine;
    PresetManager presets;

    // RANDOM BASS: generates a new patch on the message thread and pushes it
    // through the parameters, so the host records it like any other edit.
    void randomize();

    // MIDI learn
    void startMidiLearn (int paramIndex);
    void stopMidiLearn();
    int learningParam() const noexcept { return learnParam.load (std::memory_order_relaxed); }
    int paramForCc (int cc) const noexcept { return ccMap[(size_t) juce::jlimit (0, 127, cc)]; }
    juce::String midiLearnDescription() const;

    float cpuLoad() const noexcept { return engine.lastBlockMs(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleMidi (const juce::MidiMessage& message);
    void pushParametersToEngine();
    void applyCcMap (int cc, int value);

    std::array<std::atomic<float>*, (size_t) kParamCount> rawValues {};
    std::atomic<int> learnParam { -1 };
    std::array<int, 128> ccMap {};
    ParamSnapshot snapshotScratch {};
    juce::AudioBuffer<float> monoScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WobbleAudioProcessor)
};

} // namespace mk
