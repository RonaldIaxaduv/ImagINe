/*
  ==============================================================================

    AudioEngine.h
    Created: 9 Jun 2022 9:48:45am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SamplerOscillator.h"
#include "ModulatableParameter.h"
#include "Voice.h"
#include "RegionLfo.h"

class SegmentableImage; //don't include the header here yet (crossreferences), it'll be in the cpp

struct TempSound : public juce::SynthesiserSound
{
    TempSound() {}

    bool appliesToNote(int) override { return false; }
    bool appliesToChannel(int) override { return false; }
};

//==============================================================================
class AudioEngine  : public juce::AudioSource
{
public:
    AudioEngine(juce::MidiKeyboardState& keyState, juce::MidiMessageCollector& midiCollector, juce::AudioProcessor& associatedProcessor);
    ~AudioEngine() override;

    bool serialise(juce::XmlElement* xml, juce::Array<juce::MemoryBlock>* attachedData);
    bool deserialise(juce::XmlElement* xml, juce::Array<juce::MemoryBlock>* attachedData);

    SegmentableImage* getImage();

    int getNextRegionID();
    int getHighestRegionID();
    int addNewRegion(const juce::Colour& regionColour, juce::MidiKeyboardState::Listener* listenerRegion);
    void resetRegionIDs();
    void removeRegion(juce::MidiKeyboardState::Listener* listenerRegion);
    bool tryChangeRegionID(int regionID, int newRegionID);

    void addMidiListener(juce::MidiKeyboardState::Listener* newMidiListener);
    void removeMidiListener(juce::MidiKeyboardState::Listener* midiListener);

    juce::Colour getRegionColour(int regionID);
    void changeRegionColour(int regionID, juce::Colour newColour);

    void initialiseVoicesForRegion(int regionID, int voiceCount = defaultPolyphony);
    int addVoice(Voice* newVoice);
    juce::Array<Voice*> getVoicesWithID(int regionID);
    bool checkRegionHasVoice(int regionID);
    void removeVoicesWithID(int regionID);

    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_Volume(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_Pitch(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_PlaybackPositionStart(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_PlaybackPositionInterval(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_PlaybackPositionCurrent(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_FilterPosition(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoRate(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoStartingPhase(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoPhaseInterval(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoCurrentPhase(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoUpdateInterval(int regionID);

    bool updateLfoParameter(int lfoID, int targetRegionID, bool shouldBeModulated, LfoModulatableParameter modulatedParameter);

    void prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate) override;
    void suspendProcessing(bool shouldBeSuspended);
    bool isSuspended();
    void panic();

    void releaseResources() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    juce::Synthesiser* getSynth();

    void addLfo(RegionLfo* newLfo);
    RegionLfo* getLfo(int regionID);
    void removeLfo(int regionID);

private:
    juce::AudioProcessor& associatedProcessor;
    juce::dsp::ProcessSpec specs;

    juce::MidiKeyboardState& keyboardState;
    juce::MidiMessageCollector& midiCollector;
    //juce::AudioDeviceManager& deviceManager;
    juce::Synthesiser synth;

    SegmentableImage* associatedImage = nullptr; //image that the editor will display. it's important to save it here, in the AudioEngine, and not in the editor, because otherwise, it would be deleted (and couldn't be restored) whenever the editor closes

    int regionIdCounter = -1;
    juce::Array<juce::Colour> regionColours;
    juce::Array<int> takenRegionIDs;
    juce::OwnedArray<RegionLfo> lfos; //one LFO per segmented region which represents that region's outline in relation to its focus point

    static const int defaultPolyphony;

    bool serialiseImage(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData);
    bool serialiseRegionColours(juce::XmlElement* xmlAudioEngine);
    bool serialiseLFOs(juce::XmlElement* xmlAudioEngine);
    bool serialiseVoices(juce::XmlElement* xmlAudioEngine);

    bool deserialiseImage(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData);
    bool deserialiseRegionColours(juce::XmlElement* xmlAudioEngine);
    bool deserialiseLFOs_main(juce::XmlElement* xmlAudioEngine);
    bool deserialiseLFOs_mods(juce::XmlElement* xmlAudioEngine);
    bool deserialiseVoices(juce::XmlElement* xmlAudioEngine);

    void resetAll();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
