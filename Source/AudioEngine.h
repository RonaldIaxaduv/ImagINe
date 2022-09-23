/*
  ==============================================================================

    AudioEngine.h
    Created: 9 Jun 2022 9:48:45am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Voice.h"
#include "SamplerOscillator.h"
#include "ModulatableParameter.h"
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
    AudioEngine(juce::MidiKeyboardState& keyState) :
        keyboardState(keyState),
        specs()
    {
        //for (auto i = 0; i < 4; ++i)                // [1]
        //    synth.addVoice(new Voice());

        //synth.addSound(new SamplerOscillator());       // [2]
        
        synth.addSound(new TempSound());
    }

    int getNextRegionID()
    {
        return ++regionIdCounter;
    }
    int getLastRegionID()
    {
        return regionIdCounter;
    }
    int addNewRegion(const juce::Colour& regionColour)
    {
        regionColours.add(regionColour);
        return getNextRegionID();
    }

    juce::Colour getRegionColour(int regionID)
    {
        if (regionID >= 0 && regionID <= getLastRegionID())
            return regionColours[regionID];
        else
            return juce::Colours::transparentBlack;
    }
    void changeRegionColour(int regionID, juce::Colour newColour)
    {
        if (regionID >= 0 && regionID <= getLastRegionID())
        {
            regionColours.set(regionID, newColour);
        }
    }

    int addVoice(Voice* newVoice)
    {
        auto* associatedLfo = getLfo(newVoice->getID());
        if (associatedLfo != nullptr)
        {
            //newVoice->setLfoAdvancer(associatedLfo->getAdvancerFunction()); //make the new voice advance the LFO associated with the same region
            newVoice->setLfo(associatedLfo); //make the new voice advance the LFO associated with the same region
        }
        newVoice->prepare(specs);

        synth.addVoice(newVoice);
        return synth.getNumVoices() - 1;
    }
    juce::Array<Voice*> getVoicesWithID(int regionID)
    {
        juce::Array<Voice*> voiceList;

        for (int i = 0; i < synth.getNumVoices(); ++i)
        {
            auto curVoice = static_cast<Voice*>(synth.getVoice(i));

            if (curVoice->getID() == regionID)
            {
                voiceList.add(curVoice);
            }
        }

        return voiceList;
    }
    bool checkRegionHasVoice(int regionID)
    {
        if (getVoicesWithID(regionID).size() > 0)
            return true;
        else
            return false;
    }
    void removeVoicesWithID(int regionID)
    {
        for (int i = 0; i < synth.getNumVoices(); ++i)
        {
            auto* curVoice = static_cast<Voice*>(synth.getVoice(i));
            if (curVoice->getID() == regionID)
            {
                synth.removeVoice(i);
                --i;
            }
        }
    }

    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_Volume(int regionID)
    {
        juce::Array<Voice*> voices = getVoicesWithID(regionID);
        juce::Array<ModulatableParameter<double>*> parameters;

        for (auto* it = voices.begin(); it != voices.end(); it++)
        {
            parameters.add((*it)->getLevelParameter());
        }

        return parameters;
    }
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_Pitch(int regionID)
    {
        juce::Array<Voice*> voices = getVoicesWithID(regionID);
        juce::Array<ModulatableParameter<double>*> parameters;

        for (auto* it = voices.begin(); it != voices.end(); it++)
        {
            parameters.add((*it)->getPitchShiftParameter());
        }

        return parameters;
    }
    juce::Array<ModulatableParameter<double>*> getParameterOfRegion_LfoRate(int regionID)
    {
        juce::Array<ModulatableParameter<double>*> parameters;

        RegionLfo* lfo = getLfo(regionID);
        if (lfo != nullptr)
        {
            parameters.add(lfo->getFrequencyModParameter()); //only one LFO per region, so only one entry in parameters necessary
        }

        return parameters;
    }

    void prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate) override
    {
        synth.setCurrentPlaybackSampleRate(sampleRate); // [3]

        specs.sampleRate = sampleRate;

        for (auto it = lfos.begin(); it != lfos.end(); ++it)
        {
            (*it)->prepare(specs);
        }
    }

    void releaseResources() override
    {
        lfos.clear();
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();

        juce::MidiBuffer incomingMidi;
        keyboardState.processNextMidiBuffer(incomingMidi, bufferToFill.startSample,
            bufferToFill.numSamples, true);       // [4]

        //synth.renderNextBlock(*bufferToFill.buffer, incomingMidi,
        //    bufferToFill.startSample, bufferToFill.numSamples); // [5]

        //evaluate all voices sample by sample! this is crucial since every voice *must* be rendered in sync with its LFO.
        //but since different voices' LFOs can influence one another, all voices must be rendered perfectly in sync as well!
        //the main disadvantage of this is that some processing (e.g. many types of filters, EQs or convolution-based effects)
        //might not be possible anymore like this since they need to be rendered in block (since they require an FFT).
        //there may be workarounds for this, though.
        for (int i = bufferToFill.startSample; i < bufferToFill.startSample + bufferToFill.numSamples; ++i)
        {
            synth.renderNextBlock(*bufferToFill.buffer, incomingMidi, i, 1); //sample by sample
        }
    }

    juce::Synthesiser* getSynth()
    {
        return &synth;
    }

    void addLfo(RegionLfo* newLfo)
    {
        int regionID = newLfo->getRegionID(); //the LFO belongs to this region
        int newLfoIndex = lfos.size();
        newLfo->prepare(specs);
        lfos.add(newLfo);

        for (int i = 0; i < synth.getNumVoices(); ++i)
        {
            auto* curVoice = static_cast<Voice*>(synth.getVoice(i));
            if (curVoice->getID() == regionID)
            {
                //all voices belonging to the same region as the LFO will now advance this LFO
                //(as long as they aren't tailing off, i.e. it's only 1 voice advancing it, but it could be any of these voices)
                //curVoice->setLfoIndex(newLfoIndex);
                //curVoice->setLfoAdvancer(newLfo->getAdvancerFunction());
                curVoice->setLfo(newLfo);
            }
        }
    }
    RegionLfo* getLfo(int regionID)
    {
        for (auto it = lfos.begin(); it != lfos.end(); ++it)
        {
            if ((*it)->getRegionID() == regionID)
            {
                return (*it);
            }
        }
        return nullptr; //no LFO associated with this region ID found
    }
    void removeLfo(int regionID) //removes the LFO of the region with the given ID
    {
        int lfoIndex = -1;

        for (auto it = lfos.begin(); it != lfos.end(); ++it)
        {
            if ((*it)->getRegionID() == regionID)
            {
                //LFO found -> remove
                lfoIndex = lfos.indexOf((*it));
                break;
            }
        }

        for (int i = 0; i < synth.getNumVoices(); ++i)
        {
            auto* curVoice = static_cast<Voice*>(synth.getVoice(i));
            if (curVoice->getID() == regionID)
            {
                //curVoice->setLfoAdvancer([] {; }); //clear advancer function
                curVoice->setLfo(nullptr); //clear associated LFO
            }
        }

        lfos.remove(lfoIndex, true); //removed and deleted
    }

private:
    juce::MidiKeyboardState& keyboardState;
    juce::Synthesiser synth;
    juce::dsp::ProcessSpec specs;

    juce::Array<juce::Colour> regionColours;
    juce::OwnedArray<RegionLfo> lfos; //one LFO per segmented region which represents that region's outline in relation to its focus point

    int regionIdCounter = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
