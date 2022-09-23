/*
  ==============================================================================

    RegionLfo.cpp
    Created: 14 Jul 2022 8:38:00am
    Author:  Aaron

  ==============================================================================
*/

//#pragma once

#include "RegionLfo.h"

#include "ModulatableParameter.h"
//template <typename T>
//class ModulatableParameter;
//
//template <typename T>
//class ModulatableAdditiveParameter;
//
//template <typename T>
//class ModulatableMultiplicativeParameter;




RegionLfo::RegionLfo(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable, int regionID) :
    Lfo(waveTable, [](float) {; }), //can only initialise waveTable through the base class's constructor...
    frequencyModParameter(0.0)
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
    //affectedLfos.clear();
    //affectedVoices.clear(false);

    //voiceModulationFunctions.clear();
    //lfoModulationFunctions.clear();

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

//std::function<void(float semitonesToAdd)> RegionLfo::getFrequencyModulationFunction()
//{
//    return [this](float semitonesToAdd) { modulateFrequency(semitonesToAdd); };
//
//    //return &modulateFrequency; //type: void (Lfo::*)(float)  ->  would require another include. would that work?
//}

juce::Array<int> RegionLfo::getAffectedRegionIDs()
{
    juce::Array<int> regionIDs;
    //for (auto it = affectedVoices.begin(); it != affectedVoices.end(); ++it)
    //{
    //    regionIDs.add((*it)->getFirst()->getID()); //all voices within one array are assumed to belong to the same region
    //}
    //for (auto it = affectedLfos.begin(); it != affectedLfos.end(); ++it)
    //{
    //    regionIDs.add((*it)->regionID);
    //}
    regionIDs.addArray(affectedRegionIDs);
    return regionIDs;
}
juce::Array<LfoModulatableParameter> RegionLfo::getModulatedParameterIDs()
{
    juce::Array<LfoModulatableParameter> output;
    //output.addArray(modulatedVoiceParameterIDs);
    //output.addArray(modulatedLfoParameterIDs);
    output.addArray(modulatedParameterIDs);
    return output;
}
int RegionLfo::getNumModulatedParameterIDs()
{
    //return modulatedVoiceParameterIDs.size() + modulatedLfoParameterIDs.size();
    return modulatedParameterIDs.size();
}

ModulatableAdditiveParameter<double>* RegionLfo::getFrequencyModParameter()
{
    return &frequencyModParameter;
}

float RegionLfo::getModulationDepth()
{
    return depth;
}

//void RegionLfo::addRegionModulation(const juce::Array<Voice*>& newRegionVoices, const std::function<void(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)>& newModulationFunction, LfoModulatableParameter newModulatedParameterID)
//{
//    if (newRegionVoices.size() > 0)
//    {
//        int newRegionID = newRegionVoices.getFirst()->getID();
//        removeRegionModulation(newRegionID); //remove modulation if there already is one
//
//        juce::Array<Voice*>* newVoices = new juce::Array<Voice*>();
//        newVoices->addArray(newRegionVoices);
//        affectedVoices.add(newVoices);
//
//        modulationFunctions.add(newModulationFunction);
//        modulatedParameterIDs.add(newModulatedParameterID);
//
//        currentState->modulatedParameterCountChanged(modulatedParameterIDs.size());
//    }
//}
//void RegionLfo::addRegionModulation(LfoModulatableParameter newModulatedParameterID, const juce::Array<Voice*>& newRegionVoices)
//{
//    if (newRegionVoices.size() > 0)
//    {
//        int newRegionID = newRegionVoices.getFirst()->getID();
//        removeRegionModulation(newRegionID); //remove modulation if there already is one
//
//        if (newModulatedParameterID == LfoModulatableParameter::none)
//        {
//            return;
//        }
//
//        juce::Array<Voice*>* newVoices = new juce::Array<Voice*>(); //THIS MIGHT STILL CAUSE SOME MEMORY LEAKS ATM! couldn't get the delete to work properly...
//        newVoices->addArray(newRegionVoices);
//        affectedVoices.add(newVoices);
//        //newVoices->clear();
//        //delete newVoices;
//        //affectedVoices.addArray(newRegionVoices); //this throws an error at compile time because... hell, idek. it seems to have smth to do with how newRegionVoices is defined compared to affectedVoices, but I didn't find a fix for it so far
//
//        voiceModFunctionPt voiceModulationFunction = nullptr;
//        switch (newModulatedParameterID) //this only needs to handle parameters associated with Voice members
//        {
//        case LfoModulatableParameter::volume:
//            voiceModulationFunction = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
//            {
//                v->modulateLevel(static_cast<double>(lfoValueUnipolar * depth));
//            };
//            break;
//        case LfoModulatableParameter::volume_inverted:
//            voiceModulationFunction = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
//            {
//                v->modulateLevel(1.0 - static_cast<double>(lfoValueUnipolar * depth));
//            };
//            break;
//
//        case LfoModulatableParameter::pitch:
//            voiceModulationFunction = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
//            {
//                v->modulatePitchShift(static_cast<double>(lfoValueBipolar * depth) * 12.0);
//            };
//            break;
//        case LfoModulatableParameter::pitch_inverted:
//            voiceModulationFunction = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
//            {
//                v->modulatePitchShift(static_cast<double>(-lfoValueBipolar * depth) * 12.0);
//            };
//            break;
//
//        default:
//            throw std::exception("Unknown or unimplemented voice modulation parameter");
//        }
//
//        voiceModulationFunctions.add(voiceModulationFunction);
//        modulatedVoiceParameterIDs.add(newModulatedParameterID);
//
//        DBG("added voice modulation. new parameter count: " + juce::String(getNumModulatedParameterIDs()));
//        currentState->modulatedParameterCountChanged(getNumModulatedParameterIDs());
//    }
//}
//void RegionLfo::addRegionModulation(LfoModulatableParameter newModulatedParameterID, RegionLfo* lfo)
//{
//    if (lfo != nullptr)
//    {
//        int newRegionID = lfo->regionID;
//        removeRegionModulation(newRegionID); //remove modulation if there already is one
//
//        if (newModulatedParameterID == LfoModulatableParameter::none)
//        {
//            return;
//        }
//
//        affectedLfos.add(lfo);
//
//        lfoModFunctionPt lfoModulationFunction = nullptr;
//        switch (newModulatedParameterID) //this only needs to handle parameters associated with lfo members
//        {
//        case LfoModulatableParameter::lfoRate:
//            lfoModulationFunction = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, RegionLfo* lfo)
//            {
//                lfo->modulateFrequency(lfoValueBipolar * depth * 12.0f);
//            };
//            break;
//        case LfoModulatableParameter::lfoRate_inverted:
//            lfoModulationFunction = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, RegionLfo* lfo)
//            {
//                lfo->modulateFrequency(-lfoValueBipolar * depth * 12.0f);
//            };
//            break;
//
//        default:
//            throw std::exception("Unknown or unimplemented lfo modulation parameter");
//        }
//
//        lfoModulationFunctions.add(lfoModulationFunction);
//        modulatedLfoParameterIDs.add(newModulatedParameterID);
//
//        DBG("added lfo modulation. new parameter count: " + juce::String(getNumModulatedParameterIDs()));
//        currentState->modulatedParameterCountChanged(getNumModulatedParameterIDs());
//    }
//}
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
//void RegionLfo::removeRegionModulation(int regionID)
//{
//    int index = 0;
//
//    //check voices
//    auto voiceFuncIt = voiceModulationFunctions.begin();
//    for (auto itRegions = affectedVoices.begin(); itRegions != affectedVoices.end(); ++itRegions, ++index, ++voiceFuncIt)
//    {
//        if ((*itRegions)->getFirst()->getID() == regionID) //region found! -> remove modulation
//        {
//            affectedVoices.remove(index, true);
//            voiceModulationFunctions.remove(index);
//            //voiceModulationFunctions.remove(*voiceFuncIt);
//            modulatedVoiceParameterIDs.remove(index);
//
//            currentState->modulatedParameterCountChanged(getNumModulatedParameterIDs());
//
//            return; //regions can have only one entry at most (-> no other voice or LFO modulations for that region)
//        }
//    }
//
//    //check LFOs
//    index = 0;
//    auto lfoFuncIt = lfoModulationFunctions.begin();
//    for (auto itRegions = affectedLfos.begin(); itRegions != affectedLfos.end(); ++itRegions, ++index)
//    {
//        if ((*itRegions)->regionID == regionID) //region found! -> remove modulation
//        {
//            affectedLfos.remove(index);
//            lfoModulationFunctions.remove(index);
//            //lfoModulationFunctions.remove(*lfoFuncIt);
//            modulatedLfoParameterIDs.remove(index);
//
//            currentState->modulatedParameterCountChanged(getNumModulatedParameterIDs());
//
//            return; //regions can have only one entry at most (-> no other LFO modulations for that region)
//        }
//    }
//}
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
    return static_cast<double>(depth * currentValueUnipolar);
}
double RegionLfo::getCurrentValue_Bipolar()
{
    return static_cast<double>(depth * currentValueBipolar);
}

void RegionLfo::resetSamplesUntilUpdate()
{
    samplesUntilUpdate = updateInterval;
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
    int sampleIndex1 = static_cast<int>(currentTablePos);
    int sampleIndex2 = sampleIndex1 + 1;
    float frac = currentTablePos - static_cast<float>(sampleIndex1);

    auto* samplesUnipolar = waveTableUnipolar.getReadPointer(0);
    currentValueUnipolar = samplesUnipolar[sampleIndex1] + frac * (samplesUnipolar[sampleIndex2] - samplesUnipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)

    auto* samplesBipolar = waveTable.getReadPointer(0);
    currentValueBipolar = samplesBipolar[sampleIndex1] + frac * (samplesBipolar[sampleIndex2] - samplesBipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)
}

//void RegionLfo::updateModulatedParameter()
//{
//    if (waveTable.getNumSamples() > 0) //WIP: might be better to create a separate LFO state (-> state model) for when the wavetable is empty. otherwise, this is called during every sample and unnecessarily takes up resources that way
//    {
//        //auto itFunc = modulationFunctions.begin();
//        //for (auto itRegion = affectedVoices.begin(); itRegion != affectedVoices.end(); ++itRegion, ++itFunc)
//        //{
//        //    auto modulationFunction = *itFunc;
//        //    for (auto itVoice = (*itRegion)->begin(); itVoice != (*itRegion)->end(); ++itVoice)
//        //    {
//        //        modulationFunction(currentValueUnipolar, currentValueBipolar, depth, *itVoice); //one modulation function per region, applied to all voices of that region
//        //    }
//        //}
//
//        //voice modulations
//        auto itVoiceFunc = voiceModulationFunctions.begin();
//        for (auto itRegion = affectedVoices.begin(); itRegion != affectedVoices.end(); ++itRegion, ++itVoiceFunc)
//        {
//            auto voiceModulationFunction = *itVoiceFunc;
//            auto regionVoices = *itRegion;
//            for (auto itVoice = regionVoices->begin(); itVoice != regionVoices->end(); ++itVoice)
//            {
//                voiceModulationFunction(currentValueUnipolar, currentValueBipolar, depth, *itVoice); //one modulation function per region. voice mods are applied to all voices of that region
//            }
//        }
//
//        //LFO modulations
//        auto itLfoFunc = lfoModulationFunctions.begin();
//        for (auto itRegion = affectedLfos.begin(); itRegion != affectedLfos.end(); ++itRegion, ++itLfoFunc)
//        {
//            (*itLfoFunc)(currentValueUnipolar, currentValueBipolar, depth, *itRegion);
//        }
//    }
//}
//void RegionLfo::updateModulatedParameterUnsafe()
//{
//    //auto itFunc = modulationFunctions.begin();
//    //for (auto itRegion = affectedVoices.begin(); itRegion != affectedVoices.end(); ++itRegion, ++itFunc)
//    //{
//    //    auto modulationFunction = *itFunc;
//    //    for (auto itVoice = (*itRegion)->begin(); itVoice != (*itRegion)->end(); ++itVoice)
//    //    {
//    //        modulationFunction(currentValueUnipolar, currentValueBipolar, depth, *itVoice); //one modulation function per region, applied to all voices of that region
//    //    }
//    //}
//
//    //voice modulations
//    auto itVoiceFunc = voiceModulationFunctions.begin();
//    for (auto itRegion = affectedVoices.begin(); itRegion != affectedVoices.end(); ++itRegion, ++itVoiceFunc)
//    {
//        auto voiceModulationFunction = *itVoiceFunc;
//        auto regionVoices = *itRegion;
//        for (auto itVoice = regionVoices->begin(); itVoice != regionVoices->end(); ++itVoice)
//        {
//            voiceModulationFunction(currentValueUnipolar, currentValueBipolar, depth, *itVoice); //one modulation function per region. voice mods are applied to all voices of that region
//        }
//    }
//
//    //LFO modulations
//    auto itLfoFunc = lfoModulationFunctions.begin();
//    for (auto itRegion = affectedLfos.begin(); itRegion != affectedLfos.end(); ++itRegion, ++itLfoFunc)
//    {
//        (*itLfoFunc)(currentValueUnipolar, currentValueBipolar, depth, *itRegion);
//    }
//}
void RegionLfo::updateModulatedParameter()
{
    if (waveTable.getNumSamples() > 0) //WIP: might be better to create a separate LFO state (-> state model) for when the wavetable is empty. otherwise, this is called during every sample and unnecessarily takes up resources that way
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
