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

class SegmentableImage; //header included in cpp -> no trouble with crossreferences. required only for serialisation.

struct TempSound : public juce::SynthesiserSound
{
    TempSound() {}

    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

//==============================================================================
class AudioEngine  : public juce::AudioSource
{
public:
    AudioEngine(juce::MidiKeyboardState& keyState, juce::AudioProcessor& associatedProcessor);
    ~AudioEngine() override;

    void serialise(juce::XmlElement* xml, juce::Array<juce::MemoryBlock>* attachedData);
    void deserialise(juce::XmlElement* xml, juce::Array<juce::MemoryBlock>* attachedData);

    void registerImage(SegmentableImage* newAssociatedImage);

    int getNextRegionID();
    int getLastRegionID();
    int addNewRegion(const juce::Colour& regionColour);
    void resetRegionIDs();

    juce::Colour getRegionColour(int regionID);
    void changeRegionColour(int regionID, juce::Colour newColour);

    void initialiseVoicesForRegion(int regionID, int voiceCount = defaultPolyphony);
    int addVoice(Voice* newVoice);
    juce::Array<Voice*> getVoicesWithID(int regionID);
    bool checkRegionHasVoice(int regionID);
    void removeVoicesWithID(int regionID);

    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_Volume(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_Pitch(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_PlaybackPosition(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoRate(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoPhase(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoUpdateInterval(int regionID);

    void prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate) override;
    void suspendProcessing(bool shouldBeSuspended);

    void releaseResources() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    juce::Synthesiser* getSynth();

    void addLfo(RegionLfo* newLfo);
    RegionLfo* getLfo(int regionID);
    void removeLfo(int regionID);

private:
    juce::AudioProcessor& associatedProcessor;

    juce::MidiKeyboardState& keyboardState;
    juce::Synthesiser synth;
    juce::dsp::ProcessSpec specs;

    SegmentableImage* associatedImage = nullptr;

    int regionIdCounter = -1;
    juce::Array<juce::Colour> regionColours;
    juce::OwnedArray<RegionLfo> lfos; //one LFO per segmented region which represents that region's outline in relation to its focus point

    static const int defaultPolyphony;

    void serialiseRegionColours(juce::XmlElement* xmlAudioEngine);
    void serialiseLFOs(juce::XmlElement* xmlAudioEngine);
    void serialiseVoices(juce::XmlElement* xmlAudioEngine);
    void serialiseImage(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData);

    void deserialiseRegionColours(juce::XmlElement* xmlAudioEngine);
    void deserialiseLFOs_main(juce::XmlElement* xmlAudioEngine);
    void deserialiseLFOs_mods(juce::XmlElement* xmlAudioEngine);
    void deserialiseVoices(juce::XmlElement* xmlAudioEngine);
    void deserialiseImage(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
