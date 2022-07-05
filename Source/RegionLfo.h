/*
  ==============================================================================

    RegionLfo.h
    Created: 29 Jun 2022 1:47:03pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Lfo.h"
#include "Voice.h"

/// <summary>
/// an LFO which is associated with a SegmentedRegion and its Voice(s)
/// </summary>
class RegionLfo : public Lfo
{
public:
    //RegionLfo() :
    //    Lfo()
    //{ }

    /*RegionLfo(const juce::AudioBuffer<float>& waveTable, const std::function<void(float lfoValue)>& modulationFunction) :
        Lfo(waveTable, modulationFunction)
    { }*/

    RegionLfo(const juce::AudioBuffer<float>& waveTable, const std::function<void(float, Voice*)>& modulationFunction, int regionID) :
        Lfo(waveTable, [](float) {; }), //can only initialise waveTable through the base class's constructor...
        modulationFunction(modulationFunction)
    {
        this->regionID = regionID;
        updateModulatedParameter();
        currentPolarity = bipolar;
    }

    void setWaveTable(const juce::AudioBuffer<float>& waveTable)
    {
        this->waveTable.makeCopyOf(waveTable);
        setBaseFrequency(baseFrequency);
        resetPhase();
    }

    void setModulationFunction(std::function<void(float lfoValue, Voice* v)>& modulationFunction)
    {
        this->modulationFunction.swap(modulationFunction);
        updateModulatedParameter();
    }

    std::function<void(float semitonesToAdd)> getFrequencyModulationFunction()
    {
        return [this](float semitonesToAdd) { modulateFrequency(semitonesToAdd); };
    }

    juce::Array<Voice*> getVoices()
    {
        return affectedVoices;
    }
    juce::Array<int> getRegionIDs()
    {
        juce::Array<int> regionIDs;
        for (auto it = affectedVoices.begin(); it != affectedVoices.end(); ++it)
        {
            regionIDs.add((*it)->getID());
        }
        return regionIDs;
    }
    void setVoices(const juce::Array<Voice*>& newVoices)
    {
        affectedVoices.clear();
        affectedVoices.addArray(newVoices);
    }

    int getRegionID()
    {
        return regionID;
    }

    void advance() override
    {
        evaluateFrequencyModulation(); //automatically evaluate freq modulation before advancing -> saves having to use another std::function
        Lfo::advance();
    }
    std::function<void()> getAdvancerFunction()
    {
        return [this] { advance(); };
    }

    enum Polarity : int
    {
        unipolar, //LFO values lie within [0,1].
        bipolar //LFO values lie within [-1,1]. This is the default.
    };
    void setPolarity(Polarity newPolarity)
    {
        if (currentPolarity != newPolarity)
        {
            juce::AudioBuffer<float> newWaveTable;
            newWaveTable.makeCopyOf(waveTable);
            auto samples = newWaveTable.getWritePointer(0);

            switch (newPolarity)
            {
            case unipolar:
                //re-normalise from [-1,1] to [0,1]
                for (int i = 0; i < newWaveTable.getNumSamples(); ++i)
                {
                    samples[i] = (samples[i] + 1.0f) * 0.5f;
                }
                break;

            case bipolar:
                //re-normalise from [0,1] to [-1,1]
                for (int i = 0; i < newWaveTable.getNumSamples(); ++i)
                {
                    samples[i] = samples[i] * 2.0f - 1.0f;
                }
                break;

            default:
                return;
            }

            waveTable = newWaveTable;
        }
    }
    Polarity getPolarity()
    {
        return currentPolarity;
    }

protected:
    int regionID;
    std::function<void(float lfoValue, Voice* v)> modulationFunction; //shadows the original modulation function. IMPORTANT: this is only valid because the only method that references this (-> updatedModulatedParameter) is overwritten in this class. if it were used in the superclass, it would still take the old definition!
    juce::Array<Voice*> affectedVoices;

    Polarity currentPolarity;

    void updateModulatedParameter() override
    {
        if (waveTable.getNumSamples() > 0) //WIP: might be better to create a separate LFO state (interface) for when the wavetable is empty. otherwise, this is called during every sample and unnecessarily takes up resources that way
        {
            auto* samples = waveTable.getReadPointer(0);

            int sampleIndex1 = static_cast<int>(currentTablePos);
            int sampleIndex2 = sampleIndex1 + 1;
            float frac = currentTablePos - static_cast<float>(sampleIndex1);

            auto sample = samples[sampleIndex1] + frac * (samples[sampleIndex2] - samples[sampleIndex1]); //interpolate between samples (good especially at slower freqs)

            for (auto it = affectedVoices.begin(); it != affectedVoices.end(); ++it)
            {
                modulationFunction(sample, *it); //process modulation function for each affected voice
            }
        }
    }
};