/*
  ==============================================================================

    RegionLfo.cpp
    Created: 14 Jul 2022 8:38:00am
    Author:  Aaron

  ==============================================================================
*/

//#pragma once

#include "RegionLfo.h"


RegionLfo::RegionLfo(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable, int regionID) :
    Lfo(waveTable, [](float) {; }), //can only initialise waveTable through the base class's constructor...
    frequencyModParameter(0.0), phaseModParameter(1.0), updateIntervalParameter(1.0)
{
    states[static_cast<int>(RegionLfoStateIndex::unprepared)] = static_cast<RegionLfoState*>(new RegionLfoState_Unprepared(*this));
    states[static_cast<int>(RegionLfoStateIndex::withoutWaveTable)] = static_cast<RegionLfoState*>(new RegionLfoState_WithoutWaveTable(*this));
    states[static_cast<int>(RegionLfoStateIndex::withoutModulatedParameters)] = static_cast<RegionLfoState*>(new RegionLfoState_WithoutModulatedParameters(*this));
    states[static_cast<int>(RegionLfoStateIndex::muted)] = static_cast<RegionLfoState*>(new RegionLfoState_Muted(*this));
    states[static_cast<int>(RegionLfoStateIndex::active)] = static_cast<RegionLfoState*>(new RegionLfoState_Active(*this));
    states[static_cast<int>(RegionLfoStateIndex::activeRealTime)] = static_cast<RegionLfoState*>(new RegionLfoState_ActiveRealTime(*this));

    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];

    this->regionID = regionID;
    setWaveTable(waveTable, polarityOfPassedWaveTable); //calculates buffers for both unipolar and bipolar version of the wavetable
}
RegionLfo::~RegionLfo()
{
    modulatedParameters.clear();
    modulatedParameterIDs.clear();
    affectedRegionIDs.clear();

    waveTableUnipolar.setSize(0, 0);

    //release states
    delete states[static_cast<int>(RegionLfoStateIndex::unprepared)];
    delete states[static_cast<int>(RegionLfoStateIndex::withoutWaveTable)];
    delete states[static_cast<int>(RegionLfoStateIndex::withoutModulatedParameters)];
    delete states[static_cast<int>(RegionLfoStateIndex::muted)];
    delete states[static_cast<int>(RegionLfoStateIndex::active)];
    delete states[static_cast<int>(RegionLfoStateIndex::activeRealTime)];
    
    Lfo::~Lfo();
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
                prepareUpdateInterval(); //make sure updateInterval is prepared (could be delayed if updateIntervalMs was set while the LFO was unprepared!)
                DBG("Region lfo without wavetable");
            }
            break;

        case RegionLfoStateIndex::withoutModulatedParameters:
            prepareUpdateInterval(); //make sure updateInterval is prepared (could be delayed if updateIntervalMs was set while the LFO was unprepared!)
            
            if (getNumModulatedParameterIDs() > 0)
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
                if (updateInterval == 0)
                {
                    stateToTransitionTo = RegionLfoStateIndex::activeRealTime;
                    continue;
                }
                else
                {
                    nonInstantStateFound = true;
                    samplesUntilUpdate = 0; //immediately update -> should feel more responsive
                    DBG("Region lfo active");
                }
            }
            break;

        case RegionLfoStateIndex::activeRealTime:
            if (depth == 0.0f)
            {
                stateToTransitionTo = RegionLfoStateIndex::muted;
                continue;
            }
            else
            {
                if (updateInterval != 0)
                {
                    stateToTransitionTo = RegionLfoStateIndex::active;
                    continue;
                }
                else
                {
                    nonInstantStateFound = true;
                    DBG("Region lfo active (real time)");
                }
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

juce::Array<int> RegionLfo::getAffectedRegionIDs()
{
    juce::Array<int> regionIDs;
    regionIDs.addArray(affectedRegionIDs);
    return regionIDs;
}
juce::Array<LfoModulatableParameter> RegionLfo::getModulatedParameterIDs()
{
    juce::Array<LfoModulatableParameter> output;
    output.addArray(modulatedParameterIDs);
    return output;
}
int RegionLfo::getNumModulatedParameterIDs()
{
    return modulatedParameterIDs.size();
}

ModulatableAdditiveParameter<double>* RegionLfo::getFrequencyModParameter()
{
    return &frequencyModParameter;
}
ModulatableMultiplicativeParameter<double>* RegionLfo::getPhaseModParameter()
{
    return &phaseModParameter;
}
ModulatableMultiplicativeParameter<double>* RegionLfo::getUpdateIntervalParameter()
{
    return &updateIntervalParameter;
}

float RegionLfo::getModulationDepth()
{
    return depth;
}

void RegionLfo::addRegionModulation(LfoModulatableParameter newModulatedParameterID, int newRegionID, const juce::Array<ModulatableParameter<double>*>& newParameters)
{
    if (newParameters.size() > 0)
    {
        removeRegionModulation(newRegionID); //remove modulation if there already is one

        if (newModulatedParameterID == LfoModulatableParameter::none)
        {
            return;
        }

        //determine LFO evaluation function
        ModulatableParameter<double>::lfoEvalFuncPt lfoEvaluationFunction = nullptr;
        switch (newModulatedParameterID) //this only needs to handle parameters associated with Voice members
        {
        case LfoModulatableParameter::volume:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getModulationDepth());
            };
            break;
        case LfoModulatableParameter::volume_inverted:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 1.0 - static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getModulationDepth());
            };
            break;

        case LfoModulatableParameter::pitch:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 12.0 * static_cast<double>(lfo->getCurrentValue_Bipolar() * lfo->getModulationDepth());
            };
            break;
        case LfoModulatableParameter::pitch_inverted:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 12.0 * static_cast<double>(-lfo->getCurrentValue_Bipolar() * lfo->getModulationDepth());
            };
            break;

        case LfoModulatableParameter::playbackPosition:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getModulationDepth());
            };
            break;
        case LfoModulatableParameter::playbackPosition_inverted:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 1.0 - static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getModulationDepth());
            };
            break;

        


        case LfoModulatableParameter::lfoRate:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 12.0 * static_cast<double>(lfo->getCurrentValue_Bipolar() * lfo->getModulationDepth());
            };
            break;
        case LfoModulatableParameter::lfoRate_inverted:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 12.0 * static_cast<double>(-lfo->getCurrentValue_Bipolar() * lfo->getModulationDepth());
            };
            break;

        case LfoModulatableParameter::lfoPhase:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getModulationDepth());
            };
            break;
        case LfoModulatableParameter::lfoPhase_inverted:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 1.0 - static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getModulationDepth());
            };
            break;

        case LfoModulatableParameter::lfoUpdateInterval:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getModulationDepth());
            };
            break;
        case LfoModulatableParameter::lfoUpdateInterval_inverted:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 1.0 - static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getModulationDepth());
            };
            break;




        default:
            throw std::exception("Unknown or unimplemented voice modulation parameter");
        }

        //register this LFO as a modulator in all new parameters
        for (auto* it = newParameters.begin(); it != newParameters.end(); it++)
        {
            //(*it)->addModulator(this, lfoEvaluationFunction);
            (*it)->addModulator(this, getRegionID(), lfoEvaluationFunction);
        }

        //remember these parameters to update them whenever the LFO updates
        juce::Array<ModulatableParameter<double>*>* newParamsCopy = new juce::Array<ModulatableParameter<double>*>();
        newParamsCopy->addArray(newParameters);
        modulatedParameters.add(newParamsCopy);

        modulatedParameterIDs.add(newModulatedParameterID);
        affectedRegionIDs.add(newRegionID);

        DBG("added modulation. new parameter count: " + juce::String(getNumModulatedParameterIDs()));

        currentState->modulatedParameterCountChanged(getNumModulatedParameterIDs());
    }
}
void RegionLfo::removeRegionModulation(int regionID)
{
    int index = 0;

    //check voices
    for (auto itRegions = affectedRegionIDs.begin(); itRegions != affectedRegionIDs.end(); ++itRegions, ++index)
    {
        if (*itRegions == regionID) //region found! -> remove modulation
        {
            auto* params = modulatedParameters[index];
            for (auto itParam = params->begin(); itParam != params->end(); itParam++)
            {
                //(*itParam)->removeModulator(this);
                (*itParam)->removeModulator(getRegionID());
            }

            modulatedParameters.remove(index, true);
            modulatedParameterIDs.remove(index);
            affectedRegionIDs.remove(index);

            currentState->modulatedParameterCountChanged(getNumModulatedParameterIDs());

            return; //regions can have only one entry at most (-> no other voice or LFO modulations for that region)
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
    //doesn't check whether the wavetable actually contains samples, saving one if case per sample

    evaluateFrequencyModulation();

    currentTablePos += tablePosDelta;
    if (static_cast<int>(currentTablePos) >= waveTable.getNumSamples() - 1) //-1 because the last sample is equal to the first
    {
        currentTablePos -= static_cast<float>(waveTable.getNumSamples() - 1);
    }

    updateCurrentValues();
    updateModulatedParameterUnsafe(); //saves one more if case compared to the normal update
}
void RegionLfo::advanceUnsafeWithoutUpdate()
{
    //update currentTablePos without updating the current LFO value nor the modulated parameters
    //doesn't check whether the wavetable actually contains samples, saving one if case per sample

    evaluateFrequencyModulation();

    currentTablePos += tablePosDelta;
    if (static_cast<int>(currentTablePos) >= waveTable.getNumSamples() - 1) //-1 because the last sample is equal to the first
    {
        currentTablePos -= static_cast<float>(waveTable.getNumSamples() - 1);
    }
}

double RegionLfo::getCurrentValue_Unipolar()
{
    return static_cast<double>(currentValueUnipolar); //static_cast<double>(depth * currentValueUnipolar);
}
double RegionLfo::getCurrentValue_Bipolar()
{
    return static_cast<double>(currentValueBipolar); //static_cast<double>(depth * currentValueBipolar);
}

void RegionLfo::resetSamplesUntilUpdate()
{
    samplesUntilUpdate = updateInterval * updateIntervalParameter.getModulatedValue();
}
void RegionLfo::setUpdateInterval_Milliseconds(float newUpdateIntervalMs)
{
    updateIntervalMs = newUpdateIntervalMs;
    
    if (isPrepared())
    {
        prepareUpdateInterval();
    }
}
float RegionLfo::getUpdateInterval_Milliseconds()
{
    return updateIntervalMs;
}
void RegionLfo::prepareUpdateInterval()
{
    updateInterval = static_cast<int>(updateIntervalMs * 0.001f * static_cast<float>(sampleRate));
    resetSamplesUntilUpdate();

    if (currentStateIndex == RegionLfoStateIndex::active && updateInterval == 0)
    {
        transitionToState(RegionLfoStateIndex::activeRealTime); //improves performance (disables evaluation of samplesUntilUpdate)
    }
    else if (currentStateIndex == RegionLfoStateIndex::activeRealTime && updateInterval > 0)
    {
        transitionToState(RegionLfoStateIndex::active); //re-enables evaluation of samplesUntilUpdate
    }
}

bool RegionLfo::isPrepared()
{
    return currentStateIndex > RegionLfoStateIndex::unprepared; //this yields the desired result, because all states that follow this one will have had to be prepared beforehand
}

void RegionLfo::evaluateFrequencyModulation()
{
    auto cyclesPerSample = baseFrequency / static_cast<float>(sampleRate); //0.0f if frequency is 0
    tablePosDelta = static_cast<float>(waveTable.getNumSamples()) * cyclesPerSample; //0.0f if waveTable empty
    tablePosDelta *= playbackMultApprox(frequencyModParameter.getModulatedValue()); //includes all frequency modulations; approximation -> faster (power functions would be madness efficiency-wise)
}

void RegionLfo::updateCurrentValues() //pre-calculates the current LFO values (unipolar, bipolar) for quicker repeated access
{
    float effectiveTablePos = currentTablePos + static_cast<float>(waveTable.getNumSamples() - 1) * phaseModParameter.getModulatedValue();
    if (static_cast<int>(effectiveTablePos) >= waveTable.getNumSamples() - 1)
    {
        effectiveTablePos -= static_cast<float>(waveTable.getNumSamples() - 1);
    }
    /*else if (static_cast<int>(effectiveTablePos) < 0)
    {
        effectiveTablePos += static_cast<float>(waveTable.getNumSamples() - 1);
    }*/
    //float effectiveTablePos = currentTablePos * static_cast<float>(phaseModParameter.getModulatedValue()); //much cheaper and more reliable than the above version, although perhaps not quite as good-sounding

    int sampleIndex1 = static_cast<int>(effectiveTablePos);
    int sampleIndex2 = sampleIndex1 + 1;
    float frac = effectiveTablePos - static_cast<float>(sampleIndex1);

    auto* samplesUnipolar = waveTableUnipolar.getReadPointer(0);
    currentValueUnipolar = samplesUnipolar[sampleIndex1] + frac * (samplesUnipolar[sampleIndex2] - samplesUnipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)

    auto* samplesBipolar = waveTable.getReadPointer(0);
    currentValueBipolar = samplesBipolar[sampleIndex1] + frac * (samplesBipolar[sampleIndex2] - samplesBipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)
}

void RegionLfo::updateModulatedParameter()
{
    if (waveTable.getNumSamples() > 0)
    {
        auto itParamArrays = modulatedParameters.begin();
        for (auto itRegion = affectedRegionIDs.begin(); itRegion != affectedRegionIDs.end(); ++itRegion, ++itParamArrays) //iterate over all affected regions
        {
            auto regionParams = *itParamArrays;
            for (auto itParam = regionParams->begin(); itParam != regionParams->end(); ++itParam)
            {
                (*itParam)->signalModulatorUpdated(); //signal an update to all modulated parameters of that region
            }
        }
    }
}
void RegionLfo::updateModulatedParameterUnsafe()
{
    auto itParamArrays = modulatedParameters.begin();
    for (auto itRegion = affectedRegionIDs.begin(); itRegion != affectedRegionIDs.end(); ++itRegion, ++itParamArrays) //iterate over all affected regions
    {
        auto regionParams = *itParamArrays;
        for (auto itParam = regionParams->begin(); itParam != regionParams->end(); ++itParam)
        {
            (*itParam)->signalModulatorUpdated(); //signal an update to all modulated parameters of that region
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
