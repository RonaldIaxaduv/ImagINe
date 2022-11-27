/*
  ==============================================================================

    RegionLfo.cpp
    Created: 14 Jul 2022 8:38:00am
    Author:  Aaron

  ==============================================================================
*/

//#pragma once

#include "RegionLfo.h"

//constants
const float RegionLfo::defaultUpdateIntervalMs = 10.0f;


RegionLfo::RegionLfo(int regionID) :
    Lfo(juce::AudioSampleBuffer(), [](float) {; }), //can only initialise waveTable through the base class's constructor...
    waveTableUnipolar(0, 0),
    frequencyModParameter(0.0),
    startingPhaseModParameter(0.0), phaseIntervalModParameter(1.0, 0.001), currentPhaseModParameter(0.0),
    updateIntervalParameter(1.0)
{
    states[static_cast<int>(RegionLfoStateIndex::unprepared)] = static_cast<RegionLfoState*>(new RegionLfoState_Unprepared(*this));
    states[static_cast<int>(RegionLfoStateIndex::withoutWaveTable)] = static_cast<RegionLfoState*>(new RegionLfoState_WithoutWaveTable(*this));
    states[static_cast<int>(RegionLfoStateIndex::withoutModulatedParameters)] = static_cast<RegionLfoState*>(new RegionLfoState_WithoutModulatedParameters(*this));
    states[static_cast<int>(RegionLfoStateIndex::muted)] = static_cast<RegionLfoState*>(new RegionLfoState_Muted(*this));
    states[static_cast<int>(RegionLfoStateIndex::active)] = static_cast<RegionLfoState*>(new RegionLfoState_Active(*this));
    states[static_cast<int>(RegionLfoStateIndex::activeRealTime)] = static_cast<RegionLfoState*>(new RegionLfoState_ActiveRealTime(*this));
    int initialisedStates = 6;
    jassert(initialisedStates == static_cast<int>(RegionLfoStateIndex::StateIndexCount));

    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];

    updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_continuous; //default: no quantisation (cheapest)

    this->regionID = regionID;
}
RegionLfo::RegionLfo(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable, int regionID) :
    RegionLfo(regionID)
{
    setWaveTable(waveTable, polarityOfPassedWaveTable); //calculates buffers for both unipolar and bipolar version of the wavetable
}

RegionLfo::~RegionLfo()
{
    DBG("destroying RegionLfo...");

    //unsubscribe all modulated parameters
    for (auto itRegion = modulatedParameters.begin(); itRegion != modulatedParameters.end(); itRegion++)
    {
        auto* paramArray = *itRegion;

        for (auto itParam = paramArray->begin(); itParam != paramArray->end(); itParam++)
        {
            (*itParam)->removeModulator(getRegionID());
        }

        paramArray->clear();
    }

    //clear arrays
    modulatedParameters.clear(true);
    modulatedParameterIDs.clear();
    affectedRegionIDs.clear();

    //unsubscribe all modulators (otherwise, access violations will happen when the window is closed while regions are playing!)
    int unsubscribedModulators = 0;

    auto frequencyModulators = frequencyModParameter.getModulators();
    for (auto* it = frequencyModulators.begin(); it != frequencyModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getRegionID());
    }
    unsubscribedModulators++;

    auto startingPhaseModulators = startingPhaseModParameter.getModulators();
    for (auto* it = startingPhaseModulators.begin(); it != startingPhaseModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getRegionID());
    }
    unsubscribedModulators++;

    auto phaseIntervalModulators = phaseIntervalModParameter.getModulators();
    for (auto* it = phaseIntervalModulators.begin(); it != phaseIntervalModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getRegionID());
    }
    unsubscribedModulators++;

    auto currentPhaseModulators = currentPhaseModParameter.getModulators();
    for (auto* it = currentPhaseModulators.begin(); it != currentPhaseModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getRegionID());
    }
    unsubscribedModulators++;

    auto updateIntervalModulators = updateIntervalParameter.getModulators();
    for (auto* it = updateIntervalModulators.begin(); it != updateIntervalModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getRegionID());
    }
    unsubscribedModulators++;

    jassert(unsubscribedModulators == 5);

    //release states
    currentState = nullptr;
    delete states[static_cast<int>(RegionLfoStateIndex::unprepared)];
    delete states[static_cast<int>(RegionLfoStateIndex::withoutWaveTable)];
    delete states[static_cast<int>(RegionLfoStateIndex::withoutModulatedParameters)];
    delete states[static_cast<int>(RegionLfoStateIndex::muted)];
    delete states[static_cast<int>(RegionLfoStateIndex::active)];
    delete states[static_cast<int>(RegionLfoStateIndex::activeRealTime)];
    int deletedStates = 6;
    jassert(deletedStates == static_cast<int>(RegionLfoStateIndex::StateIndexCount));

    //waveTableUnipolar.setSize(0, 0); //not necessary
    //waveTable.setSize(0, 0); //not necessary

    //Lfo::~Lfo(); //this causes a heap exception for some reason -> don't do it

    DBG("RegionLfo destroyed.");
}

void RegionLfo::setWaveTable(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable)
{
    //the waveTable member of the base class is hereby defined to be exclusively bipolar, waveTableUnipolar will be unipolar

    switch (polarityOfPassedWaveTable)
    {
    case Polarity::unipolar:
        this->waveTableUnipolar.makeCopyOf(waveTable);
        calculateBipolarWaveTable();
        break;

    case Polarity::bipolar:
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
ModulatableAdditiveParameter<double>* RegionLfo::getStartingPhaseModParameter()
{
    return &startingPhaseModParameter;
}
ModulatableMultiplicativeParameterLowerCap<double>* RegionLfo::getPhaseIntervalModParameter()
{
    return &phaseIntervalModParameter;
}
ModulatableAdditiveParameter<double>* RegionLfo::getCurrentPhaseModParameter()
{
    return &currentPhaseModParameter;
}
ModulatableMultiplicativeParameter<double>* RegionLfo::getUpdateIntervalParameter()
{
    return &updateIntervalParameter;
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
        case LfoModulatableParameter::playbackPositionStart:
        case LfoModulatableParameter::playbackPositionInterval:
        case LfoModulatableParameter::playbackPositionCurrent:
        case LfoModulatableParameter::lfoStartingPhase:
        case LfoModulatableParameter::lfoPhaseInterval:
        case LfoModulatableParameter::lfoCurrentPhase:
        case LfoModulatableParameter::lfoUpdateInterval:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                //return static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getDepth());
                return 1.0 - static_cast<double>(lfo->getDepth()) * (1.0 - static_cast<double>(lfo->getCurrentValue_Unipolar())); //interval: [1.0-depth, 1.0]
            };
            break;

        case LfoModulatableParameter::volume_inverted:
        case LfoModulatableParameter::playbackPositionStart_inverted:
        case LfoModulatableParameter::playbackPositionInterval_inverted:
        case LfoModulatableParameter::playbackPositionCurrent_inverted:
        case LfoModulatableParameter::lfoStartingPhase_inverted:
        case LfoModulatableParameter::lfoPhaseInterval_inverted:
        case LfoModulatableParameter::lfoCurrentPhase_inverted:
        case LfoModulatableParameter::lfoUpdateInterval_inverted:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                //return 1.0 - static_cast<double>(lfo->getCurrentValue_Unipolar() * lfo->getDepth());
                //return static_cast<double>(lfo->getDepth()) * (1.0 - static_cast<double>(lfo->getCurrentValue_Unipolar())); //interval: [1.0, 1.0-depth]
                return 1.0 - static_cast<double>(lfo->getDepth()) * static_cast<double>(lfo->getCurrentValue_Unipolar()); //interval: [1.0, 1.0-depth]
            };
            break;




        case LfoModulatableParameter::pitch:
        case LfoModulatableParameter::lfoRate:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 12.0 * static_cast<double>(lfo->getCurrentValue_Bipolar()) * static_cast<double>(lfo->getDepth()); //12.0: one octave
            };
            break;

        case LfoModulatableParameter::pitch_inverted:
        case LfoModulatableParameter::lfoRate_inverted:
            lfoEvaluationFunction = [](RegionLfo* lfo)
            {
                return 12.0 * static_cast<double>(-lfo->getCurrentValue_Bipolar()) * static_cast<double>(lfo->getDepth()); //12.0: one octave
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

        DBG("added a modulation to LFO " + juce::String(getRegionID()) + ": " + juce::String(static_cast<int>(newModulatedParameterID)) + " for region " + juce::String(newRegionID) + " (" + juce::String(newParameters.size()) + " voices). new parameter count: " + juce::String(getNumModulatedParameterIDs()));

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

            DBG("removed a modulation from LFO " + juce::String(getRegionID()) + ": " + juce::String(regionID) + " (" + juce::String(params->size()) + " voices). new parameter count: " + juce::String(getNumModulatedParameterIDs()));
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
void RegionLfo::setRegionID(int newRegionID)
{
    int oldRegionID = regionID;
    regionID = newRegionID;

    int i = 0;
    for (auto itRegion = affectedRegionIDs.begin(); itRegion != affectedRegionIDs.end(); ++itRegion, ++i)
    {
        if ((*itRegion) == oldRegionID)
        {
            affectedRegionIDs.set(i, newRegionID);
            break; //only one modulation per region
        }
    }
}
void RegionLfo::otherRegionIDHasChanged(int oldRegionID, int newRegionID)
{
    int i = 0;
    for (auto itRegion = affectedRegionIDs.begin(); itRegion != affectedRegionIDs.end(); ++itRegion, ++i)
    {
        if ((*itRegion) == oldRegionID)
        {
            affectedRegionIDs.set(i, newRegionID);
            break; //only one modulation per region
        }
    }
}

void RegionLfo::advance()
{
    currentState->advance();
}
void RegionLfo::advanceUnsafeWithUpdate()
{
    //doesn't check whether the wavetable actually contains samples, saving one if case per sample

    evaluateFrequencyModulation();
    evaluateTablePosModulation(); //increments currentTablePos if required

    updateCurrentValues(); //also updates the current modulated phase
    updateModulatedParameterUnsafe(); //saves one more if case compared to the normal update
}
void RegionLfo::advanceUnsafeWithoutUpdate()
{
    //update currentTablePos without updating the current LFO value nor the modulated parameters
    //doesn't check whether the wavetable actually contains samples, saving one if case per sample

    evaluateFrequencyModulation();
    evaluateTablePosModulation(); //increments currentTablePos if required

    ////update phase
    //updateLatestModulatedPhase(); //required to keep the LFO line updated
}

void RegionLfo::setPhase(float relativeTablePos)
{
    currentState->setPhase(relativeTablePos);
}
float RegionLfo::getPhase()
{
    return currentState->getPhase();
}
void RegionLfo::resetPhase(bool updateParameters)
{
    currentState->resetPhase(updateParameters);
}
void RegionLfo::resetPhaseUnsafe_WithoutUpdate()
{
    //currentTablePos = 0.0f;
    latestModulatedPhase = std::fmod(startingPhaseModParameter.getModulatedValue(), 1.0);
    currentTablePos = latestModulatedPhase * static_cast<float>(getNumSamplesUnsafe() - 1);
}
void RegionLfo::resetPhaseUnsafe_WithUpdate()
{
    //currentTablePos = 0.0f;
    currentTablePos = std::fmod(startingPhaseModParameter.getModulatedValue(), 1.0) * (getNumSamplesUnsafe() - 1);
    updateModulatedParameterUnsafe();
}

float RegionLfo::getLatestModulatedPhase()
{
    //return latestModulatedPhase * getPhase();
    return latestModulatedPhase;
}
float RegionLfo::getLatestModulatedStartingPhase()
{
    return latestModulatedStartingPhase;
}
float RegionLfo::getLatestModulatedPhaseInterval()
{
    return latestModulatedPhaseInterval;
}

double RegionLfo::getCurrentValue_Unipolar()
{
    //calculated in advance in updateCurrentValues()
    return static_cast<double>(currentValueUnipolar); //static_cast<double>(depth * currentValueUnipolar);
}
double RegionLfo::getCurrentValue_Bipolar()
{
    //calculated in advance in updateCurrentValues()
    return static_cast<double>(currentValueBipolar); //static_cast<double>(depth * currentValueBipolar);
}

void RegionLfo::resetSamplesUntilUpdate()
{
    samplesUntilUpdate = static_cast<int>(static_cast<double>(updateInterval) * (*this.*updateRateQuantisationFuncPt)());
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
double RegionLfo::getMsUntilUpdate()
{
    return 1000.0 * static_cast<double>(samplesUntilUpdate) / sampleRate;
}

void RegionLfo::setUpdateRateQuantisationMethod(UpdateRateQuantisationMethod newUpdateRateQuantisationMethod)
{
    switch (newUpdateRateQuantisationMethod)
    {
    case UpdateRateQuantisationMethod::full:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(1.0);
        break;
    case UpdateRateQuantisationMethod::full_dotted:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(1.5 * 1.0);
        break;
    case UpdateRateQuantisationMethod::full_triole:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor((2.0/3.0) * 1.0);
        break;

    case UpdateRateQuantisationMethod::half:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(2.0);
        break;
    case UpdateRateQuantisationMethod::half_dotted:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(1.5 * 2.0);
        break;
    case UpdateRateQuantisationMethod::half_triole:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor((2.0/3.0) * 2.0);
        break;

    case UpdateRateQuantisationMethod::quarter:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(4.0);
        break;
    case UpdateRateQuantisationMethod::quarter_dotted:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(1.5 * 4.0);
        break;
    case UpdateRateQuantisationMethod::quarter_triole:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor((2.0/3.0) * 4.0);
        break;

    case UpdateRateQuantisationMethod::eighth:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(8.0);
        break;
    case UpdateRateQuantisationMethod::eighth_dotted:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(1.5 * 8.0);
        break;
    case UpdateRateQuantisationMethod::eighth_triole:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor((2.0/3.0) * 8.0);
        break;

    case UpdateRateQuantisationMethod::sixteenth:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(16.0);
        break;
    case UpdateRateQuantisationMethod::sixteenth_dotted:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(1.5 * 16.0);
        break;
    case UpdateRateQuantisationMethod::sixteenth_triole:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor((2.0/3.0) * 16.0);
        break;

    case UpdateRateQuantisationMethod::thirtysecond:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(32.0);
        break;
    case UpdateRateQuantisationMethod::thirtysecond_dotted:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(1.5 * 32.0);
        break;
    case UpdateRateQuantisationMethod::thirtysecond_triole:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor((2.0/3.0) * 32.0);
        break;

    case UpdateRateQuantisationMethod::sixtyfourth:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(64.0);
        break;
    case UpdateRateQuantisationMethod::sixtyfourth_dotted:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor(1.5 * 64.0);
        break;
    case UpdateRateQuantisationMethod::sixtyfourth_triole:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_fraction;
        calculateUpdateRateQuantisationFactor((2.0/3.0) * 64.0);
        break;

    case UpdateRateQuantisationMethod::continuous:
        updateRateQuantisationFuncPt = &RegionLfo::getQuantisedUpdateRate_continuous;
        break;

    default:
        throw std::exception("Unknown or unhandled value of UpdateRateQuantisationMethod.");
    }

    DBG("new update rate quantisation method: " + juce::String(static_cast<int>(newUpdateRateQuantisationMethod)));

    currentUpdateRateQuantisationMethod = newUpdateRateQuantisationMethod;
    resetSamplesUntilUpdate();
}
UpdateRateQuantisationMethod RegionLfo::getUpdateRateQuantisationMethod()
{
    return currentUpdateRateQuantisationMethod;
}

double RegionLfo::getQuantisedUpdateRate_continuous()
{
    return updateIntervalParameter.getModulatedValue(); //no special processing needed
}
double RegionLfo::getQuantisedUpdateRate_fraction()
{
    double modVal = updateIntervalParameter.getModulatedValue();
    return std::ceil(modVal * updateRateQuantisationFactor) * updateRateQuantisationFactor_denom; //quantise to an integer multiple of updateRateQuantisationFactor_denom * modVal
}

double RegionLfo::getBaseStartingPhase()
{
    return startingPhaseModParameter.getBaseValue();
}
void RegionLfo::setBaseStartingPhase(double newBaseStartingPhase)
{
    startingPhaseModParameter.setBaseValue(newBaseStartingPhase);
}
double RegionLfo::getBasePhaseInterval()
{
    return phaseIntervalModParameter.getBaseValue();
}
void RegionLfo::setBasePhaseInterval(double newBasePhaseInterval)
{
    phaseIntervalModParameter.setBaseValue(newBasePhaseInterval);
}

float RegionLfo::getDepth()
{
    return depth;
}
void RegionLfo::setDepth(float newDepth)
{
    depth = newDepth;
}

bool RegionLfo::serialise(juce::XmlElement* xmlLfo)
{
    DBG("serialising LFO...");
    bool serialisationSuccessful = true;

    xmlLfo->setAttribute("regionID", regionID);

    xmlLfo->setAttribute("currentTablePos", currentTablePos);
    xmlLfo->setAttribute("depth", depth);
    xmlLfo->setAttribute("updateIntervalMs", updateIntervalMs);
    xmlLfo->setAttribute("currentUpdateRateQuantisationMethod", static_cast<int>(currentUpdateRateQuantisationMethod));
    xmlLfo->setAttribute("baseFrequency", getBaseFrequency());

    xmlLfo->setAttribute("phaseInterval_base", phaseIntervalModParameter.getBaseValue());
    xmlLfo->setAttribute("startingPhase_base", startingPhaseModParameter.getBaseValue());
    //current phase: fixed base value

    juce::XmlElement* xmlParameterIDs = xmlLfo->createNewChildElement("modulatedParameterIDs");
    xmlParameterIDs->setAttribute("size", modulatedParameterIDs.size());
    int i = 0;
    for (auto itParamID = modulatedParameterIDs.begin(); itParamID != modulatedParameterIDs.end(); ++itParamID, ++i)
    {
        xmlParameterIDs->setAttribute("ID_" + juce::String(i), static_cast<int>(*itParamID));
    }

    juce::XmlElement* xmlRegionIDs = xmlLfo->createNewChildElement("affectedRegionIDs");
    xmlRegionIDs->setAttribute("size", affectedRegionIDs.size());
    i = 0;
    for (auto itRegionID = affectedRegionIDs.begin(); itRegionID != affectedRegionIDs.end(); ++itRegionID, ++i)
    {
        xmlRegionIDs->setAttribute("ID_" + juce::String(i), *itRegionID);
    }

    //wavetable/wavetableUnipolar not needed, because the LFO will be initialised with its corresponding SegmentedRegion beforehand

    DBG(juce::String(serialisationSuccessful ? "LFO has been serialised." : "LFO could not be serialised."));
    return serialisationSuccessful;
}
bool RegionLfo::deserialise_main(juce::XmlElement* xmlLfo)
{
    DBG("deserialising LFO (excluding mods)..."); //reasons why mods are excluded: all LFOs and voices first have to be initialised, and access to the AudioEngine would be necessary (which the LFO doesn't have -> easier to do from the AudioEngine itself)
    bool deserialisationSuccessful = true;

    regionID = xmlLfo->getIntAttribute("regionID", -1);

    currentTablePos = xmlLfo->getDoubleAttribute("currentTablePos", 0.0);
    depth = xmlLfo->getDoubleAttribute("depth", 0.0);
    setBaseFrequency(xmlLfo->getDoubleAttribute("baseFrequency", 0.2));

    setUpdateInterval_Milliseconds(xmlLfo->getDoubleAttribute("updateIntervalMs", defaultUpdateIntervalMs));
    setUpdateRateQuantisationMethod(static_cast<UpdateRateQuantisationMethod>(xmlLfo->getIntAttribute("currentUpdateRateQuantisationMethod", static_cast<int>(UpdateRateQuantisationMethod::continuous))));

    phaseIntervalModParameter.setBaseValue(xmlLfo->getDoubleAttribute("phaseInterval_base", 1.0));
    startingPhaseModParameter.setBaseValue(xmlLfo->getDoubleAttribute("startingPhase_base", 0.0));

    DBG(juce::String(deserialisationSuccessful ? "LFO has been deserialised (except for mods)." : "LFO could not be deserialised (main)."));
    return deserialisationSuccessful;
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
void RegionLfo::evaluateTablePosModulation()
{
    //double currentPhase = static_cast<double>(currentTablePos) / static_cast<double>(getNumSamplesUnsafe());
    //currentPhaseModParameter.modulateValueIfUpdated(&currentPhase); //if any current phase modulator updated, currentTablePos will be set to the new value. otherwise, its value will remain the same
    //currentPhase = std::fmod(currentPhase, 1.0); //keep within [0.0, 1.0)
    //currentTablePos = currentPhase * (getNumSamplesUnsafe() - 1); //currentTablePos actually needs to be affected by the modulation to ensure that the LFO keeps running from the phase it was set to (it won't update every time, only whenever a modulator updated)
    ////1 div, 1 mult, 1 mod

    double modulatedTablePos = currentTablePos;
    if (currentPhaseModParameter.modulateValueIfUpdated(&modulatedTablePos)) //true if it updated the value
    {
        currentTablePos = static_cast<float>(std::fmod(modulatedTablePos, 1.0)) * static_cast<float>(getNumSamplesUnsafe() - 1); //subtracting -1 should *theoretically* not be necessary here bc it will be multiplied with a value within [0,1), *but* due to rounding, it would be possible that it takes on an out-of-range value! it shouldn't make a noticable difference sound-wise.
        //worst case: 1 if, 1 mod, 1 mult (-> saving 1 div for 1 if -> float division is probably slower than an if(?))

        //don't advance; stick to the target phase!
    }
    else
    {
        //no need to adjust currentTablePos (it didn't change).
        //best case: 1 if

        //advance
        currentTablePos += tablePosDelta;
        if (currentTablePos >= latestModulatedPhaseInterval * static_cast<float>(waveTable.getNumSamples() - 1)) //-1 because the last sample is equal to the first; multiplied with the phase interval because otherwise, there will be doubling!
        {
            currentTablePos -= latestModulatedPhaseInterval * static_cast<float>(waveTable.getNumSamples() - 1);
        }
    }
}

void RegionLfo::updateCurrentValues() //pre-calculates the current LFO values (unipolar, bipolar) for quicker repeated access
{
    //note: modulation of the current phase has been applied in evaluateTablePosModulation already.
     
    //important goal: the phase interval squishing should not decrease the value of deltaTablePos! 
    
    ////idea 1:
    //float effectiveTablePos = std::fmod(currentTablePos, getNumSamplesUnsafe() * phaseIntervalModParameter.getModulatedValue()); //needs to be modded instead of multiplied, because otherwise it would effectively decrease deltaTablePos (i.e. the frequency), which wouldn't be intuitive
    //effectiveTablePos = std::fmod(effectiveTablePos + startingPhaseModParameter.getModulatedValue() * getNumSamplesUnsafe(), getNumSamplesUnsafe());
    //latestModulatedPhase = effectiveTablePos / static_cast<float>(getNumSamplesUnsafe()); //update this variable to keep the LFO line drawn updated that's drawn over regions; normally, division by phaseModParameter.getBaseValue() would be necessary, but that value is fixed at 1.0
    ////^- 2 mod, 2 mult, 1 div (oof...)

    //idea 2: effectivePhase = (((currentTablePos/numSamples) mod phaseInterval) + startingPhase) mod 1.0
    updateLatestModulatedPhase();
    float effectiveTablePos = static_cast<float>(latestModulatedPhase) * static_cast<float>(getNumSamplesUnsafe() - 1); //convert phase back to index within wavetable (-1 at the end to ensure that floating-point rounding won't let the variable take on out-of-range values!)
    //^- 2 mod, 1 mult, 1 div -> slightly better

    int sampleIndex1 = static_cast<int>(effectiveTablePos);
    int sampleIndex2 = sampleIndex1 + 1;
    float frac = effectiveTablePos - static_cast<float>(sampleIndex1);

    auto* samplesUnipolar = waveTableUnipolar.getReadPointer(0);
    currentValueUnipolar = samplesUnipolar[sampleIndex1] + frac * (samplesUnipolar[sampleIndex2] - samplesUnipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)

    auto* samplesBipolar = waveTable.getReadPointer(0);
    currentValueBipolar = samplesBipolar[sampleIndex1] + frac * (samplesBipolar[sampleIndex2] - samplesBipolar[sampleIndex1]); //interpolate between samples (good especially at slower freqs)
}
void RegionLfo::updateLatestModulatedPhase()
{
    latestModulatedStartingPhase = startingPhaseModParameter.getModulatedValue();
    latestModulatedPhaseInterval = phaseIntervalModParameter.getModulatedValue(); //note that phaseIntervalModParameter is a capped parameter that cannot become 0.0 
    double effectivePhase = static_cast<double>(currentTablePos / static_cast<float>(getNumSamplesUnsafe())); //convert currentTablePos to currentPhase
    effectivePhase = std::fmod(effectivePhase, latestModulatedPhaseInterval); //convert to new interval while preserving deltaTablePos (i.e. the frequency)!
    effectivePhase = std::fmod(effectivePhase + latestModulatedStartingPhase, 1.0); //shift the starting phase from 0 to the value stated by startingPhaseModParameter and wrap, so that the value stays within [0,1) (i.e. within wavetable later on)
    latestModulatedPhase = effectivePhase; //remember modulated phase to keep the LFO line updated that's drawn over regions
}

void RegionLfo::calculateUpdateRateQuantisationFactor(double quantisationValue)
{
    updateRateQuantisationFactor = quantisationValue;
    updateRateQuantisationFactor_denom = 1.0 / quantisationValue;
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
