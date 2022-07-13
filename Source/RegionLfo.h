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
    enum Polarity : int
    {
        unipolar, //LFO values lie within [0,1].
        bipolar //LFO values lie within [-1,1]. This is the default.
    };

    RegionLfo(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable, int regionID) :
        Lfo(waveTable, [](float) {; }) //can only initialise waveTable through the base class's constructor...
    {
        this->regionID = regionID;
        setWaveTable(waveTable, polarityOfPassedWaveTable); //calculates buffers for both unipolar and bipolar version of the wavetable
        updateCurrentValues();
        //updateModulatedParameter();
        //currentPolarity = bipolar;
    }

    void setWaveTable(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable)
    {
        //the waveTable member of the base class is hereby defined to be exclusively bipolar, waveTableUnipolar will be unipolar

        switch (polarityOfPassedWaveTable)
        {
        case unipolar:
            this->waveTableUnipolar.makeCopyOf(waveTable);
            calculateBipolarWaveTable();
            break;

        case bipolar:
            this->waveTable.makeCopyOf(waveTable);
            calculateUnipolarWaveTable();
            break;

        default:
            throw std::exception("invalid polarity");
        }

        setBaseFrequency(baseFrequency);
        resetPhase();
    }

    std::function<void(float semitonesToAdd)> getFrequencyModulationFunction()
    {
        return [this](float semitonesToAdd) { modulateFrequency(semitonesToAdd); };
    }

    /*void setModulationFunction(std::function<void(float lfoValue, Voice* v)>& modulationFunction)
    {
        this->modulationFunction.swap(modulationFunction);
        updateModulatedParameter();
    }*/

    /*juce::Array<Voice*> getVoices()
    {
        return affectedVoices;
    }*/
    juce::Array<int> getAffectedRegionIDs()
    {
        juce::Array<int> regionIDs;
        for (auto it = affectedVoices.begin(); it != affectedVoices.end(); ++it)
        {
            regionIDs.add((*it)->getFirst()->getID()); //all voices within one array are assumed to belong to the same region
        }
        return regionIDs;
    }
    juce::Array<LfoModulatableParameter> getModulatedParameterIDs()
    {
        juce::Array<LfoModulatableParameter> output;
        output.addArray(modulatedParameterIDs);
        return output;
    }
    /*void setVoices(const juce::Array<Voice*>& newVoices)
    {
        affectedVoices.clear();
        affectedVoices.addArray(newVoices);
    }*/

    void addRegionModulation(const juce::Array<Voice*>& newRegionVoices, const std::function<void(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)>& newModulationFunction, LfoModulatableParameter newModulatedParameterID)
    {
        if (newRegionVoices.size() > 0)
        {
            int newRegionID = newRegionVoices.getFirst()->getID();
            removeRegionModulation(newRegionID); //remove modulation if there already is one

            juce::Array<Voice*>* newVoices = new juce::Array<Voice*>();
            newVoices->addArray(newRegionVoices);
            affectedVoices.add(newVoices);

            modulationFunctions.add(newModulationFunction);

            modulatedParameterIDs.add(newModulatedParameterID);
        }
    }
    void removeRegionModulation(int regionID)
    {
        int index = 0;
        for (auto itRegions = affectedVoices.begin(); itRegions != affectedVoices.end(); ++itRegions, ++index)
        {
            if ((*itRegions)->getFirst()->getID() == regionID) //region found! -> remove modulation
            {
                affectedVoices.remove(index, true);
                modulationFunctions.remove(index);
                modulatedParameterIDs.remove(index);
                return; //regions can have only one entry at most
            }
        }
    }

    int getRegionID()
    {
        return regionID;
    }

    void advance() override
    {
        evaluateFrequencyModulation(); //automatically evaluate freq modulation before advancing -> saves having to use another std::function
        Lfo::advance();
        updateCurrentValues();
    }
    std::function<void()> getAdvancerFunction()
    {
        return [this] { advance(); };
    }

    //void setPolarity(Polarity newPolarity)
    //{
    //    if (currentPolarity != newPolarity)
    //    {
    //        juce::AudioBuffer<float> newWaveTable;
    //        newWaveTable.makeCopyOf(waveTable);
    //        auto samples = newWaveTable.getWritePointer(0);

    //        switch (newPolarity)
    //        {
    //        case unipolar:
    //            //re-normalise from [-1,1] to [0,1]
    //            for (int i = 0; i < newWaveTable.getNumSamples(); ++i)
    //            {
    //                samples[i] = (samples[i] + 1.0f) * 0.5f;
    //            }
    //            break;

    //        case bipolar:
    //            //re-normalise from [0,1] to [-1,1]
    //            for (int i = 0; i < newWaveTable.getNumSamples(); ++i)
    //            {
    //                samples[i] = samples[i] * 2.0f - 1.0f;
    //            }
    //            break;

    //        default:
    //            return;
    //        }

    //        waveTable = newWaveTable;
    //    }
    //}
    //Polarity getPolarity()
    //{
    //    return currentPolarity;
    //}

protected:
    int regionID;
    //std::function<void(float lfoValue, Voice* v)> modulationFunction; //shadows the original modulation function. IMPORTANT: this is only valid because the only method that references this (-> updatedModulatedParameter) is overwritten in this class. if it were used in the superclass, it would still take the old definition!
    //juce::Array<Voice*> affectedVoices;

    //ideas for enabling having different modulation functions for different voices:
    //# juce::OwnedArray<juce::Array<Voice*>> affectedVoices
    //# juce::Array<std::function<void(float lfoValue, Voice* v)>> modulationFunctions
    //# both arrays have the same number of entries. for entry i, all voices in affectedVoices[i] are modulated with modulationFunctions[i] (all these voices always belong to the same region!).
    //when adding new entries, check whether the region ID of the new voices matches that of affectedVoices[i] for any i. if so, remove the i-th item of affectedVoices and modulationFunctions before adding the new items.
    //# note that it's probably also necessary to add a third array containing the polarity required for the respective modulation function (or maybe use them as a third parameter in the function - this would make it possible to use a pointer to the correct buffer, which might actually be quicker than using an if- or switch-case...)
    
    //# optimisation: pre-calculate the unipolar and bipolar LFO values *when advancing*, so they don't have to be calculated several times when calling updateModulatedParameter

    juce::OwnedArray<juce::Array<Voice*>> affectedVoices; //one array per modulated region (contains all voices of that region)
    juce::Array<std::function<void(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)>> modulationFunctions; //one function per modulated region; having both polarities' values in the function makes it easy and efficient (no ifs or pointers necessary!) to choose between them when defining the modulation function
    juce::Array<LfoModulatableParameter> modulatedParameterIDs;

    //Polarity currentPolarity;

    juce::AudioBuffer<float> waveTableUnipolar;

    float currentValueUnipolar = 0.0f;
    float currentValueBipolar = 0.0f;

    float depth = 1.0f; //make this modulatable later

    void updateCurrentValues() //pre-calculates the current LFO values (unipolar, bipolar) for quicker repeated access
    {
        int sampleIndex1 = static_cast<int>(currentTablePos);
        int sampleIndex2 = sampleIndex1 + 1;
        float frac = currentTablePos - static_cast<float>(sampleIndex1);

        auto* samplesUnipolar = waveTableUnipolar.getReadPointer(0);
        currentValueUnipolar = samplesUnipolar[sampleIndex1] + frac * (samplesUnipolar[sampleIndex2] - samplesUnipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)

        auto* samplesBipolar = waveTable.getReadPointer(0);
        currentValueBipolar = samplesBipolar[sampleIndex1] + frac * (samplesBipolar[sampleIndex2] - samplesBipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)
    }

    void updateModulatedParameter() override
    {
        if (waveTable.getNumSamples() > 0) //WIP: might be better to create a separate LFO state (-> state model) for when the wavetable is empty. otherwise, this is called during every sample and unnecessarily takes up resources that way
        {
            //for (auto it = affectedVoices.begin(); it != affectedVoices.end(); ++it)
            //{
            //    modulationFunction(sample, *it); //process modulation function for each affected voice
            //}

            auto itFunc = modulationFunctions.begin();
            for (auto itRegion = affectedVoices.begin(); itRegion != affectedVoices.end(); ++itRegion, ++itFunc)
            {
                auto modulationFunction = *itFunc;
                for (auto itVoice = (*itRegion)->begin(); itVoice != (*itRegion)->end(); ++itVoice)
                {
                    modulationFunction(currentValueUnipolar, currentValueBipolar, depth, *itVoice); //one modulation function per region, applied to all voices of that region
                }
            }
        }
    }

    void calculateUnipolarWaveTable()
    {
        waveTableUnipolar.makeCopyOf(waveTable);
        auto samples = waveTableUnipolar.getWritePointer(0);

        //re-normalise from [0,1] to [-1,1]
        for (int i = 0; i < waveTableUnipolar.getNumSamples(); ++i)
        {
            samples[i] = (samples[i] + 1.0f) * 0.5f;
        }
    }
    void calculateBipolarWaveTable()
    {
        waveTable.makeCopyOf(waveTableUnipolar);
        auto samples = waveTable.getWritePointer(0);

        //re-normalise from [0,1] to [-1,1]
        for (int i = 0; i < waveTable.getNumSamples(); ++i)
        {
            samples[i] = samples[i] * 2.0f - 1.0f;
        }
    }
};