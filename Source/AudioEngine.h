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
//template <typename T> class ModulatableParameter;
//template <typename T> class ModulatableAdditiveParameter;
//template <typename T> class ModulatableMultiplicativeParameter;

#include "Voice.h"
#include "RegionLfo.h"

struct TempSound : public juce::SynthesiserSound
{
    TempSound() {}

    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

//==============================================================================
/*
*/
class AudioEngine  : public juce::AudioSource
{
public:
    AudioEngine(juce::MidiKeyboardState& keyState);

    int getNextRegionID();
    int getLastRegionID();
    int addNewRegion(const juce::Colour& regionColour);

    juce::Colour getRegionColour(int regionID);
    void changeRegionColour(int regionID, juce::Colour newColour);

    int addVoice(Voice* newVoice);
    juce::Array<Voice*> getVoicesWithID(int regionID);
    bool checkRegionHasVoice(int regionID);
    void removeVoicesWithID(int regionID);

    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_Volume(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_Pitch(int regionID);
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoRate(int regionID);

    void prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate) override;

    void releaseResources() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    juce::Synthesiser* getSynth();

    void addLfo(RegionLfo* newLfo);
    RegionLfo* getLfo(int regionID);
    void removeLfo(int regionID);

private:
    juce::MidiKeyboardState& keyboardState;
    juce::Synthesiser synth;
    juce::dsp::ProcessSpec specs;

    juce::Array<juce::Colour> regionColours;
    juce::OwnedArray<RegionLfo> lfos; //one LFO per segmented region which represents that region's outline in relation to its focus point

    int regionIdCounter = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
