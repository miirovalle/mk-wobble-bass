#include "Plugin/PluginProcessor.h"
#include "Plugin/PluginEditor.h"

namespace mk
{
namespace
{

bool isLogarithmic (int index)
{
    const auto& meta = paramMeta (index);
    const bool logUnit = meta.unit == Unit::Hz || meta.unit == Unit::Ms
                      || meta.unit == Unit::Sec || meta.unit == Unit::Ratio;
    return logUnit && meta.lo > 0.0f;
}

juce::String presetFolderName() { return "MKWobbleBass"; }

} // namespace

//==============================================================================
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& stateToUse)
    : apvts (stateToUse)
{
    readFavorites();
    refresh();
    loadDefault();
}

juce::File PresetManager::presetFolder() const
{
    auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                      .getChildFile (presetFolderName())
                      .getChildFile ("Presets");
    if (! folder.exists())
        folder.createDirectory();
    return folder;
}

void PresetManager::refresh()
{
    presets.clear();
    presets.reserve ((size_t) factoryPresetCount() + 32);

    for (int i = 0; i < factoryPresetCount(); ++i)
    {
        const auto& preset = factoryPreset (i);
        PresetEntry entry;
        entry.name = preset.name;
        entry.category = preset.category;
        entry.isFactory = true;
        entry.factoryIndex = i;
        presets.push_back (entry);
    }

    auto folder = presetFolder();
    for (const auto& file : folder.findChildFiles (juce::File::findFiles, false, "*.mkpreset"))
    {
        auto xml = juce::XmlDocument::parse (file);
        if (xml == nullptr)
            continue;

        PresetEntry entry;
        entry.name = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
        entry.category = xml->getStringAttribute ("category", "User");
        entry.isFactory = false;
        entry.factoryIndex = -1;
        entry.file = file;
        presets.push_back (entry);
    }
}

void PresetManager::applySnapshot (const ParamSnapshot& snapshot)
{
    for (int i = 0; i < kParamCount; ++i)
        if (auto* parameter = apvts.getParameter (paramId (i)))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (snapshot.values[i]));
}

bool PresetManager::load (int index)
{
    if (index < 0 || index >= (int) presets.size())
        return false;

    const auto& entry = presets[(size_t) index];

    if (entry.isFactory)
    {
        ParamSnapshot snapshot;
        applyFactoryPreset (entry.factoryIndex, snapshot);
        applySnapshot (snapshot);
    }
    else
    {
        auto xml = juce::XmlDocument::parse (entry.file);
        if (xml == nullptr)
            return false;

        const auto tree = juce::ValueTree::fromXml (*xml);
        if (! tree.isValid())
            return false;

        apvts.replaceState (tree);
    }

    name = entry.name;
    category = entry.category;
    current = index;
    currentIsUser = ! entry.isFactory;

    if (onPresetChanged != nullptr)
        onPresetChanged();

    return true;
}

bool PresetManager::saveAs (const juce::String& presetName, const juce::String& presetCategory)
{
    if (presetName.trim().isEmpty())
        return false;

    auto xml = apvts.copyState().createXml();
    if (xml == nullptr)
        return false;

    xml->setAttribute ("name", presetName);
    xml->setAttribute ("category", presetCategory);
    xml->setAttribute ("manufacturer", "MK Audio");

    const auto file = presetFolder().getChildFile (juce::File::createLegalFileName (presetName + ".mkpreset"));
    if (! xml->writeTo (file))
        return false;

    refresh();
    for (int i = 0; i < (int) presets.size(); ++i)
    {
        if (! presets[(size_t) i].isFactory && presets[(size_t) i].name == presetName)
        {
            current = i;
            currentIsUser = true;
        }
    }

    name = presetName;
    category = presetCategory;

    if (onPresetChanged != nullptr)
        onPresetChanged();

    return true;
}

bool PresetManager::remove (int index)
{
    if (index < 0 || index >= (int) presets.size() || presets[(size_t) index].isFactory)
        return false;

    if (! presets[(size_t) index].file.deleteFile())
        return false;

    refresh();
    if (current >= (int) presets.size())
        current = -1;
    return true;
}

bool PresetManager::importFile (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
        return false;

    const auto target = presetFolder().getChildFile (file.getFileNameWithoutExtension() + ".mkpreset");
    return xml->writeTo (target);
}

bool PresetManager::exportFile (int index, const juce::File& file)
{
    if (index < 0 || index >= (int) presets.size())
        return false;

    const auto& entry = presets[(size_t) index];
    if (! entry.isFactory)
        return entry.file.copyFileTo (file);

    ParamSnapshot snapshot;
    applyFactoryPreset (entry.factoryIndex, snapshot);
    applySnapshot (snapshot);

    auto xml = apvts.copyState().createXml();
    if (xml == nullptr)
        return false;

    xml->setAttribute ("name", entry.name);
    xml->setAttribute ("category", entry.category);
    return xml->writeTo (file);
}

void PresetManager::toggleFavorite (const juce::String& presetName)
{
    if (favorites.contains (presetName))
        favorites.removeString (presetName);
    else
        favorites.add (presetName);
    writeFavorites();
}

bool PresetManager::isFavorite (const juce::String& presetName) const
{
    return favorites.contains (presetName);
}

void PresetManager::loadNext (int direction)
{
    if (presets.empty())
        return;

    int next = current + direction;
    if (next < 0)
        next = (int) presets.size() - 1;
    if (next >= (int) presets.size())
        next = 0;
    load (next);
}

void PresetManager::loadDefault()
{
    if (presets.empty())
        return;

    ParamSnapshot snapshot;
    resetSnapshotToDefaults (snapshot);
    applySnapshot (snapshot);
    name = "Init";
    category = "Init";
    current = -1;
    currentIsUser = false;
}

void PresetManager::readFavorites()
{
    const auto file = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile (presetFolderName())
                          .getChildFile ("favorites.txt");
    if (! file.existsAsFile())
        return;

    juce::StringArray lines;
    file.readLines (lines);
    for (const auto& line : lines)
        if (line.trim().isNotEmpty())
            favorites.addIfNotAlreadyThere (line.trim());
}

void PresetManager::writeFavorites()
{
    const auto file = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile (presetFolderName())
                          .getChildFile ("favorites.txt");
    file.getParentDirectory().createDirectory();
    juce::String text;
    for (const auto& favourite : favorites)
        text << favourite << juce::newLine;
    file.replaceWithText (text);
}

//==============================================================================
WobbleAudioProcessor::WobbleAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MKWobbleBassState", createLayout()),
      presets (apvts)
{
    for (int i = 0; i < kParamCount; ++i)
    {
        rawValues[(size_t) i] = apvts.getRawParameterValue (paramId (i));
        jassert (rawValues[(size_t) i] != nullptr);
    }

    ccMap.fill (-1);
    presets.onPresetChanged = [this] { engine.params().requestSnap(); };
    presets.loadDefault();
}

WobbleAudioProcessor::~WobbleAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout WobbleAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < kParamCount; ++i)
    {
        const auto& meta = paramMeta (i);
        const auto name = juce::String (paramName (i));
        const auto id = juce::ParameterID { paramMeta (i).id, 1 };

        int choiceCount = 0;
        if (const auto* choices = paramChoices (i, choiceCount))
        {
            juce::StringArray items;
            for (int c = 0; c < choiceCount; ++c)
                items.add (choices[c]);
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                id, name, items, juce::jlimit (0, choiceCount - 1, (int) meta.def)));
            continue;
        }

        juce::NormalisableRange<float> range { meta.lo, meta.hi };
        if (isLogarithmic (i))
        {
            const float lo = meta.lo, hi = meta.hi;
            range = juce::NormalisableRange<float> (
                lo, hi,
                [lo, hi] (float, float, float n) { return lo * std::pow (hi / lo, n); },
                [lo, hi] (float, float, float v) { return std::log (juce::jlimit (lo, hi, v) / lo) / std::log (hi / lo); });
        }

        juce::String unit;
        switch (meta.unit)
        {
            case Unit::Hz: unit = " Hz"; break;
            case Unit::Db: unit = " dB"; break;
            case Unit::Ms: unit = " ms"; break;
            case Unit::Semi: unit = " st"; break;
            case Unit::Cents: unit = " ct"; break;
            case Unit::Pct: unit = " %"; break;
            default: break;
        }

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            id, name, range, juce::jlimit (meta.lo, meta.hi, meta.def),
            juce::AudioParameterFloatAttributes().withLabel (unit)));
    }

    return layout;
}

void WobbleAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    keyboardState.reset();
    monoScratch.setSize (1, juce::jmax (samplesPerBlock, 32));
    pushParametersToEngine();
    engine.params().requestSnap();
}

void WobbleAudioProcessor::releaseResources()
{
    engine.reset();
    keyboardState.allNotesOff (1);
}

void WobbleAudioProcessor::reset()
{
    engine.allNotesOff (true);
    keyboardState.allNotesOff (1);
}

bool WobbleAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void WobbleAudioProcessor::pushParametersToEngine()
{
    for (int i = 0; i < kParamCount; ++i)
        snapshotScratch.values[i] = rawValues[(size_t) i]->load (std::memory_order_relaxed);

    engine.params().setAll (snapshotScratch);
}

void WobbleAudioProcessor::handleMidi (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        engine.noteOn (message.getNoteNumber(), message.getFloatVelocity());
        keyboardState.noteOn (1, message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isNoteOff())
    {
        engine.noteOff (message.getNoteNumber());
        keyboardState.noteOff (1, message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isAllNotesOff())
    {
        engine.allNotesOff (false);
        keyboardState.allNotesOff (message.getChannel());
    }
    else if (message.isAllSoundOff())
    {
        engine.allNotesOff (true);
        keyboardState.allNotesOff (message.getChannel());
    }
    else if (message.isPitchWheel())
    {
        engine.pitchBend ((float) (message.getPitchWheelValue() - 8192) / 8192.0f);
    }
    else if (message.isChannelPressure())
    {
        engine.setAftertouch ((float) message.getChannelPressureValue() / 127.0f);
    }
    else if (message.isAftertouch())
    {
        engine.setAftertouch ((float) message.getAfterTouchValue() / 127.0f);
    }
    else if (message.isController())
    {
        const int cc = message.getControllerNumber();
        const int value = message.getControllerValue();

        if (cc == 1)
            engine.setModWheel ((float) value / 127.0f);
        else if (cc == 64)
            engine.setSustainPedal (value >= 64);
        else if (cc == 120 || cc == 123)
        {
            engine.allNotesOff (false);
            keyboardState.allNotesOff (message.getChannel());
        }

        applyCcMap (cc, value);
    }
}

void WobbleAudioProcessor::applyCcMap (int cc, int value)
{
    const int learn = learnParam.load (std::memory_order_relaxed);
    if (learn >= 0)
    {
        ccMap[(size_t) juce::jlimit (0, 127, cc)] = learn;
        learnParam.store (-1, std::memory_order_relaxed);
        return;
    }

    const int target = ccMap[(size_t) juce::jlimit (0, 127, cc)];
    if (target >= 0 && target < kParamCount)
    {
        if (auto* parameter = apvts.getParameter (paramId (target)))
            parameter->setValueNotifyingHost ((float) value / 127.0f);
    }
}

void WobbleAudioProcessor::startMidiLearn (int paramIndex)
{
    learnParam.store (juce::jlimit (0, kParamCount - 1, paramIndex), std::memory_order_relaxed);
}

void WobbleAudioProcessor::stopMidiLearn()
{
    learnParam.store (-1, std::memory_order_relaxed);
}

juce::String WobbleAudioProcessor::midiLearnDescription() const
{
    const int learn = learnParam.load (std::memory_order_relaxed);
    if (learn >= 0)
        return juce::String ("Learning: move a control for ") + paramName (learn);

    juce::StringArray mapped;
    for (int cc = 0; cc < 128; ++cc)
        if (ccMap[(size_t) cc] >= 0)
            mapped.add ("CC" + juce::String (cc) + " > " + juce::String (paramName (ccMap[(size_t) cc])));

    return mapped.isEmpty() ? juce::String ("No MIDI mappings") : mapped.joinIntoString (", ");
}

void WobbleAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    pushParametersToEngine();

    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            if (const auto bpm = position->getBpm())
                engine.setTempo (*bpm);
            engine.setTransportPlaying (position->getIsPlaying());
        }
    }

    const int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : monoScratch.getWritePointer (0);

    // MIDI is split so note events land on the right sample within the block.
    int lastSample = 0;
    for (const auto metadata : midi)
    {
        const int position = juce::jlimit (0, numSamples, metadata.samplePosition);
        if (position > lastSample)
        {
            engine.process (left + lastSample, right + lastSample, position - lastSample);
            lastSample = position;
        }
        handleMidi (metadata.getMessage());
    }

    if (lastSample < numSamples)
        engine.process (left + lastSample, right + lastSample, numSamples - lastSample);

    if (buffer.getNumChannels() == 1)
        buffer.copyFrom (0, 0, monoScratch, 0, 0, numSamples);

    midi.clear();
}

void WobbleAudioProcessor::setCurrentProgram (int index)
{
    presets.load (index);
}

const juce::String WobbleAudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < (int) presets.entries().size())
        return presets.entries()[(size_t) index].name;
    return "Init";
}

juce::AudioProcessorEditor* WobbleAudioProcessor::createEditor()
{
    return new WobbleAudioProcessorEditor (*this);
}

void WobbleAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presets.currentName(), nullptr);

    juce::StringArray mappings;
    for (int cc = 0; cc < 128; ++cc)
        if (ccMap[(size_t) cc] >= 0)
            mappings.add (juce::String (cc) + ":" + juce::String (ccMap[(size_t) cc]));
    state.setProperty ("midiMap", mappings.joinIntoString (";"), nullptr);

    // The drawn wobble curve travels with the session.
    juce::StringArray points;
    for (int i = 0; i < Engine::kWobblePoints; ++i)
        points.add (juce::String (engine.wobblePoint (i), 4));

    state.setProperty ("wobbleCurve", points.joinIntoString (","), nullptr);
    state.setProperty ("gatePattern", (int) engine.tranceGate().patternMask(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destination);
}

void WobbleAudioProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
    {
        juce::Logger::writeToLog ("MK Wobble Bass: rejected invalid state");
        return;
    }

    apvts.replaceState (juce::ValueTree::fromXml (*xml));

    ccMap.fill (-1);
    juce::StringArray tokens;
    tokens.addTokens (xml->getStringAttribute ("midiMap"), ";", "");
    for (const auto& token : tokens)
    {
        const int cc = token.upToFirstOccurrenceOf (":", false, false).getIntValue();
        const int param = token.fromFirstOccurrenceOf (":", false, false).getIntValue();
        if (cc >= 0 && cc < 128 && param >= 0 && param < kParamCount)
            ccMap[(size_t) cc] = param;
    }

    juce::StringArray curvePoints;
    curvePoints.addTokens (xml->getStringAttribute ("wobbleCurve"), ",", "");
    for (int i = 0; i < Engine::kWobblePoints && i < curvePoints.size(); ++i)
        engine.setWobblePoint (i, curvePoints[i].getFloatValue());

    engine.tranceGate().setPattern ((uint16_t) xml->getIntAttribute ("gatePattern", 0x5555));

    engine.params().requestSnap();
}

} // namespace mk

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new mk::WobbleAudioProcessor();
}
