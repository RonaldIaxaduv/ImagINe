/*
  ==============================================================================

    RegionLfo.cpp
    Created: 14 Jul 2022 8:38:00am
    Author:  Aaron

  ==============================================================================
*/

#include "RegionLfo.h"

RegionLfo::RegionLfo(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable, int regionID) :
    Lfo(waveTable, [](float) {; }) //can only initialise waveTable through the base class's constructor...
{
    states[static_cast<int>(RegionLfoStateIndex::unprepared)] = static_cast<RegionLfoState*>(new RegionLfoState_Unprepared(*this));
    states[static_cast<int>(RegionLfoStateIndex::withoutWaveTable)] = static_cast<RegionLfoState*>(new RegionLfoState_WithoutWaveTable(*this));
    states[static_cast<int>(RegionLfoStateIndex::withoutModulatedParameters)] = static_cast<RegionLfoState*>(new RegionLfoState_WithoutModulatedParameters(*this));
    states[static_cast<int>(RegionLfoStateIndex::muted)] = static_cast<RegionLfoState*>(new RegionLfoState_Muted(*this));
    states[static_cast<int>(RegionLfoStateIndex::active)] = static_cast<RegionLfoState*>(new RegionLfoState_Active(*this));

    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];

    this->regionID = regionID;
    setWaveTable(waveTable, polarityOfPassedWaveTable); //calculates buffers for both unipolar and bipolar version of the wavetable
}

void RegionLfo::setWaveTable(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable)
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

    currentState->waveTableSet(waveTable.getNumSamples());
}

void RegionLfo::prepare(const juce::dsp::ProcessSpec& spec)
{
    Lfo::prepare(spec);
    currentState->prepared(spec.sampleRate);
}

void RegionLfo::transitionToState(RegionLfoStateIndex stateToTransitionTo)
{
    bool nonInstantStateFound = false;

    while (!nonInstantStateFound) //check if any states can be skipped (might be the case depending on what has been prepared already)
    {
        switch (stateToTransitionTo)
        {
        case RegionLfoStateIndex::unprepared:
            nonInstantStateFound = true;
            DBG("Region lfo unprepared");
            break;

        case RegionLfoStateIndex::withoutWaveTable:
            if (waveTable.getNumSamples() > 0)
            {
                stateToTransitionTo = RegionLfoStateIndex::withoutModulatedParameters;
                continue;
            }
            else
            {
                nonInstantStateFound = true;
                DBG("Region lfo without wavetable");
            }
            break;

        case RegionLfoStateIndex::withoutModulatedParameters:
            if (modulatedParameterIDs.size() > 0)
            {
                if (depth == 0.0f)
                    stateToTransitionTo = RegionLfoStateIndex::muted;
                else
                    stateToTransitionTo = RegionLfoStateIndex::active;
                continue;
            }
            else
            {
                nonInstantStateFound = true;
                DBG("Region lfo without mods");
            }
            break;

        case RegionLfoStateIndex::muted:
            if (depth > 0.0f)
            {
                stateToTransitionTo = RegionLfoStateIndex::active;
                continue;
            }
            else
            {
                nonInstantStateFound = true;
                DBG("Region lfo muted");
            }
            break;

        case RegionLfoStateIndex::active:
            if (depth == 0.0f)
            {
                stateToTransitionTo = RegionLfoStateIndex::muted;
                continue;
            }
            else
            {
                nonInstantStateFound = true;
                DBG("Region lfo active");
            }
            break;

        default:
            throw std::exception("unhandled state index");
        }
    }

    //finally, update the current state index
    currentStateIndex = stateToTransitionTo;
    currentState = states[static_cast<int>(currentStateIndex)];
}

std::function<void(float semitonesToAdd)> RegionLfo::getFrequencyModulationFunction()
{
    return [this](float semitonesToAdd) { modulateFrequency(semitonesToAdd); };

    //return &modulateFrequency; //type: void (Lfo::*)(float)  ->  would require another include. would that work?
}

juce::Array<int> RegionLfo::getAffectedRegionIDs()
{
    juce::Array<int> regionIDs;
    for (auto it = affectedVoices.begin(); it != affectedVoices.end(); ++it)
    {
        regionIDs.add((*it)->getFirst()->getID()); //all voices within one array are assumed to belong to the same region
    }
    return regionIDs;
}
juce::Array<LfoModulatableParameter> RegionLfo::getModulatedParameterIDs()
{
    juce::Array<LfoModulatableParameter> output;
    output.addArray(modulatedParameterIDs);
    return output;
}

void RegionLfo::addRegionModulation(const juce::Array<Voice*>& newRegionVoices, const std::function<void(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)>& newModulationFunction, LfoModulatableParameter newModulatedParameterID)
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

        currentState->modulatedParameterCountChanged(modulatedParameterIDs.size());
    }
}
void RegionLfo::removeRegionModulation(int regionID)
{
    int index = 0;
    for (auto itRegions = affectedVoices.begin(); itRegions != affectedVoices.end(); ++itRegions, ++index)
    {
        if ((*itRegions)->getFirst()->getID() == regionID) //region found! -> remove modulation
        {
            affectedVoices.remove(index, true);
            modulationFunctions.remove(index);
            modulatedParameterIDs.remove(index);

            currentState->modulatedParameterCountChanged(modulatedParameterIDs.size());

            return; //regions can have only one entry at most
        }
    }
}

int RegionLfo::getRegionID()
{
    return regionID;
}

void RegionLfo::advance()
{
    currentState->advance();
}
void RegionLfo::advanceUnsafeWithUpdate()
{
    evaluateFrequencyModulation();

    currentTablePos += tablePosDelta;
    if (static_cast<int>(currentTablePos) >= waveTable.getNumSamples() - 1) //-1 because the last sample is equal to the first
    {
        currentTablePos -= static_cast<float>(waveTable.getNumSamples() - 1);
    }

    updateCurrentValues();
    updateModulatedParameter();
}
void RegionLfo::advanceUnsafeWithoutUpdate()
{
    evaluateFrequencyModulation();

    currentTablePos += tablePosDelta;
    if (static_cast<int>(currentTablePos) >= waveTable.getNumSamples() - 1) //-1 because the last sample is equal to the first
    {
        currentTablePos -= static_cast<float>(waveTable.getNumSamples() - 1);
    }
}
std::function<void()> RegionLfo::getAdvancerFunction()
{
    return [this] { advance(); };
}

void RegionLfo::updateCurrentValues() //pre-calculates the current LFO values (unipolar, bipolar) for quicker repeated access
{
    int sampleIndex1 = static_cast<int>(currentTablePos);
    int sampleIndex2 = sampleIndex1 + 1;
    float frac = currentTablePos - static_cast<float>(sampleIndex1);

    auto* samplesUnipolar = waveTableUnipolar.getReadPointer(0);
    currentValueUnipolar = samplesUnipolar[sampleIndex1] + frac * (samplesUnipolar[sampleIndex2] - samplesUnipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)

    auto* samplesBipolar = waveTable.getReadPointer(0);
    currentValueBipolar = samplesBipolar[sampleIndex1] + frac * (samplesBipolar[sampleIndex2] - samplesBipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)
}

void RegionLfo::updateModulatedParameter()
{
    if (waveTable.getNumSamples() > 0) //WIP: might be better to create a separate LFO state (-> state model) for when the wavetable is empty. otherwise, this is called during every sample and unnecessarily takes up resources that way
    {
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

void RegionLfo::calculateUnipolarWaveTable()
{
    waveTableUnipolar.makeCopyOf(waveTable);
    auto samples = waveTableUnipolar.getWritePointer(0);

    //re-normalise from [0,1] to [-1,1]
    for (int i = 0; i < waveTableUnipolar.getNumSamples(); ++i)
    {
        samples[i] = (samples[i] + 1.0f) * 0.5f;
    }
}
void RegionLfo::calculateBipolarWaveTable()
{
    waveTable.makeCopyOf(waveTableUnipolar);
    auto samples = waveTable.getWritePointer(0);

    //re-normalise from [0,1] to [-1,1]
    for (int i = 0; i < waveTable.getNumSamples(); ++i)
    {
        samples[i] = samples[i] * 2.0f - 1.0f;
    }
}
