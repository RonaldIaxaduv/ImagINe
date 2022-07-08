/*
  ==============================================================================

    DahdsrEnvelopeState.cpp
    Created: 7 Jul 2022 7:15:13pm
    Author:  Aaron

  ==============================================================================
*/

#include "DahdsrEnvelopeStates.h"

#pragma region DahdsrEnvelopeState_Unprepared

DahdsrEnvelopeState_Unprepared::DahdsrEnvelopeState_Unprepared(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
{
    implementedDahdsrEnvelopeStateIndex = DahdsrEnvelopeStateIndex::unprepared;
    this->transitionTo = transitionTo;
}

DahdsrEnvelopeState_Unprepared::~DahdsrEnvelopeState_Unprepared()
{
    transitionTo = nullptr;
}

void DahdsrEnvelopeState_Unprepared::sampleRateChanged(double newSampleRate, bool triggerTransition)
{
    if (triggerTransition)
    {
        //transition to next state: idle
        transitionTo(DahdsrEnvelopeStateIndex::idle, 0.0);
    }
}

void DahdsrEnvelopeState_Unprepared::noteOn()
{
    throw std::exception("Envelope has not been prepared yet, so it cannot handle notes.");
}

double DahdsrEnvelopeState_Unprepared::getNextEnvelopeSample()
{
    throw std::exception("Envelope has not been prepared yet, so it cannot provide envelope samples.");
}

void DahdsrEnvelopeState_Unprepared::noteOff()
{
    throw std::exception("Envelope has not been prepared yet, so it cannot handle notes.");
}

void DahdsrEnvelopeState_Unprepared::forceStop()
{
    //isn't playing yet -> do nothing
}

void DahdsrEnvelopeState_Unprepared::setStartingLevel(double startingLevel)
{
    //doesn't have any volume
}

void DahdsrEnvelopeState_Unprepared::resetState()
{
    //nothing to reset
}

#pragma endregion DahdsrEnvelopeState_Unprepared




#pragma region DahdsrEnvelopeState_Idle

DahdsrEnvelopeState_Idle::DahdsrEnvelopeState_Idle(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
{
    implementedDahdsrEnvelopeStateIndex = DahdsrEnvelopeStateIndex::idle;
    this->transitionTo = transitionTo;
}

DahdsrEnvelopeState_Idle::~DahdsrEnvelopeState_Idle()
{
    transitionTo = nullptr;
}

void DahdsrEnvelopeState_Idle::sampleRateChanged(double newSampleRate, bool triggerTransition)
{
    //already prepared -> do nothing
}

void DahdsrEnvelopeState_Idle::noteOn()
{
    //begin delay time
    transitionTo(DahdsrEnvelopeStateIndex::delay, 0.0);
}

double DahdsrEnvelopeState_Idle::getNextEnvelopeSample()
{
    return 0.0; //no sound playing yet
}

void DahdsrEnvelopeState_Idle::noteOff()
{
    //no sound playing yet -> do nothing
}

void DahdsrEnvelopeState_Idle::forceStop()
{
    //no sound playing yet anyway -> do nothing
}

void DahdsrEnvelopeState_Idle::setStartingLevel(double startingLevel)
{
    //doesn't have any volume
}

void DahdsrEnvelopeState_Idle::resetState()
{
    //nothing to reset
}

#pragma endregion DahdsrEnvelopeState_Idle




#pragma region DahdsrEnvelopeState_Delay

DahdsrEnvelopeState_Delay::DahdsrEnvelopeState_Delay(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
{
    implementedDahdsrEnvelopeStateIndex = DahdsrEnvelopeStateIndex::delay;
    this->transitionTo = transitionTo;
}

DahdsrEnvelopeState_Delay::~DahdsrEnvelopeState_Delay()
{
    transitionTo = nullptr;
}

void DahdsrEnvelopeState_Delay::sampleRateChanged(double newSampleRate, bool triggerTransition)
{
    if (newSampleRate != sampleRate && newSampleRate > 0.0)
    {
        sampleRate = newSampleRate;
        setTime(timeInSeconds); //update time to new sample rate
        resetState();

        if (triggerTransition)
        {
            //return to idle state to reset
            transitionTo(DahdsrEnvelopeStateIndex::idle, 0.0);
        }
    }
}

void DahdsrEnvelopeState_Delay::noteOn()
{
    //restart delay time
    resetState();
}

double DahdsrEnvelopeState_Delay::getNextEnvelopeSample()
{
    if (currentSample < timeInSamples - 1)
    {
        currentSample += 1;
        return 0.0;
    }
    else //last sample -> transition to attack state
    {
        transitionTo(DahdsrEnvelopeStateIndex::attack, 0.0);
        resetState();
        return 0.0;
    }
}

void DahdsrEnvelopeState_Delay::noteOff()
{
    //transition directly to release (since no sound is playing yet, it could also directly transition to idle, but this is more consistent and thus preferable imo)
    transitionTo(DahdsrEnvelopeStateIndex::release, 0.0);
    resetState();
}

void DahdsrEnvelopeState_Delay::forceStop()
{
    //immediately return to idle
    transitionTo(DahdsrEnvelopeStateIndex::idle, 0.0);
    resetState();
}

void DahdsrEnvelopeState_Delay::setStartingLevel(double startingLevel)
{
    //doesn't have any volume
}

//================================================================

void DahdsrEnvelopeState_Delay::setTime(double newTimeInSeconds)
{
    timeInSeconds = newTimeInSeconds;
    timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

    DBG("new delay time: " + juce::String(newTimeInSeconds) + "s (" + juce::String(timeInSamples) + " samples)");

    currentSample = 0;
}
double DahdsrEnvelopeState_Delay::getTimeInSeconds()
{
    return timeInSeconds;
}

void DahdsrEnvelopeState_Delay::resetState()
{
    currentSample = 0;
}

#pragma endregion DahdsrEnvelopeState_Delay




#pragma region DahdsrEnvelopeState_Attack

DahdsrEnvelopeState_Attack::DahdsrEnvelopeState_Attack(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
{
    implementedDahdsrEnvelopeStateIndex = DahdsrEnvelopeStateIndex::attack;
    this->transitionTo = transitionTo;
}

DahdsrEnvelopeState_Attack::~DahdsrEnvelopeState_Attack()
{
    transitionTo = nullptr;
}

void DahdsrEnvelopeState_Attack::sampleRateChanged(double newSampleRate, bool triggerTransition)
{
    if (newSampleRate != sampleRate && newSampleRate > 0.0)
    {
        sampleRate = newSampleRate;
        setTime(timeInSeconds); //update time to new sample rate
        resetState();

        if (triggerTransition)
        {
            //return to idle state to reset
            transitionTo(DahdsrEnvelopeStateIndex::idle, 0.0);
        }
    }
}

void DahdsrEnvelopeState_Attack::noteOn()
{
    //transition back to delay
    transitionTo(DahdsrEnvelopeStateIndex::delay, envelopeLevelCurrent);
    resetState();
}

double DahdsrEnvelopeState_Attack::getNextEnvelopeSample()
{
    if (currentSample < timeInSamples - 1)
    {
        double currentValue = envelopeLevelCurrent;
        envelopeLevelCurrent += envelopeIncrementPerSample;
        currentSample += 1;
        return currentValue;
    }
    else //last sample -> transition to hold state
    {
        transitionTo(DahdsrEnvelopeStateIndex::hold, envelopeLevelEnd);
        resetState();
        return envelopeLevelEnd;
    }
}

void DahdsrEnvelopeState_Attack::noteOff()
{
    //transition directly to release (release starting level is set to the current envelope level)
    transitionTo(DahdsrEnvelopeStateIndex::release, envelopeLevelCurrent);
    resetState();
}

void DahdsrEnvelopeState_Attack::forceStop()
{
    //immediately return to idle
    transitionTo(DahdsrEnvelopeStateIndex::idle, envelopeLevelCurrent);
    resetState();
}

void DahdsrEnvelopeState_Attack::setStartingLevel(double startingLevel)
{
    envelopeLevelStart = startingLevel;
    updateEnvelopeIncrementPerSample();
    resetState();
}
double DahdsrEnvelopeState_Attack::getStartingLevel()
{
    return envelopeLevelStart;
}

//================================================================

void DahdsrEnvelopeState_Attack::setTime(double newTimeInSeconds)
{
    timeInSeconds = newTimeInSeconds;
    timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

    currentSample = 0;

    updateEnvelopeIncrementPerSample();
}
double DahdsrEnvelopeState_Attack::getTimeInSeconds()
{
    return timeInSeconds;
}

void DahdsrEnvelopeState_Attack::setEndLevel(double newEndLevel)
{
    envelopeLevelEnd = newEndLevel;
    updateEnvelopeIncrementPerSample();
    resetState();
}
double DahdsrEnvelopeState_Attack::getEndLevel()
{
    return envelopeLevelEnd;
}

void DahdsrEnvelopeState_Attack::resetState()
{
    currentSample = 0;
    envelopeLevelCurrent = envelopeLevelStart;
}

void DahdsrEnvelopeState_Attack::updateEnvelopeIncrementPerSample()
{
    if (timeInSamples > 0)
    {
        envelopeIncrementPerSample = (envelopeLevelEnd - envelopeLevelStart) / static_cast<double>(timeInSamples);
    }
    else
    {
        envelopeIncrementPerSample = 0.0;
    }
}

#pragma endregion DahdsrEnvelopeState_Attack




#pragma region DahdsrEnvelopeState_Hold

DahdsrEnvelopeState_Hold::DahdsrEnvelopeState_Hold(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
{
    implementedDahdsrEnvelopeStateIndex = DahdsrEnvelopeStateIndex::hold;
    this->transitionTo = transitionTo;
}

DahdsrEnvelopeState_Hold::~DahdsrEnvelopeState_Hold()
{
    transitionTo = nullptr;
}

void DahdsrEnvelopeState_Hold::sampleRateChanged(double newSampleRate, bool triggerTransition)
{
    if (newSampleRate != sampleRate && newSampleRate > 0.0)
    {
        sampleRate = newSampleRate;
        setTime(timeInSeconds); //update time to new sample rate
        resetState();

        if (triggerTransition)
        {
            //return to idle state to reset
            transitionTo(DahdsrEnvelopeStateIndex::idle, 0.0);
        }
    }
}

void DahdsrEnvelopeState_Hold::noteOn()
{
    //transition back to delay
    transitionTo(DahdsrEnvelopeStateIndex::delay, level);
    resetState();
}

double DahdsrEnvelopeState_Hold::getNextEnvelopeSample()
{
    if (currentSample < timeInSamples - 1)
    {
        currentSample += 1;
        return level; //constant level
    }
    else //last sample -> transition to decay state
    {
        transitionTo(DahdsrEnvelopeStateIndex::decay, level);
        resetState();
        return level;
    }
}

void DahdsrEnvelopeState_Hold::noteOff()
{
    //transition directly to release (release starting level is set to the current envelope level)
    transitionTo(DahdsrEnvelopeStateIndex::release, level);
    resetState();
}

void DahdsrEnvelopeState_Hold::forceStop()
{
    //immediately return to idle
    transitionTo(DahdsrEnvelopeStateIndex::idle, level);
    resetState();
}

void DahdsrEnvelopeState_Hold::setStartingLevel(double startingLevel)
{
    //no starting level -> does nothing
}

//================================================================

void DahdsrEnvelopeState_Hold::setTime(double newTimeInSeconds)
{
    timeInSeconds = newTimeInSeconds;
    timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

    currentSample = 0;
}
double DahdsrEnvelopeState_Hold::getTimeInSeconds()
{
    return timeInSeconds;
}

void DahdsrEnvelopeState_Hold::setLevel(double newLevel)
{
    level = newLevel;
}
double DahdsrEnvelopeState_Hold::getLevel()
{
    return level;
}

void DahdsrEnvelopeState_Hold::resetState()
{
    currentSample = 0;
}

#pragma endregion DahdsrEnvelopeState_Hold




#pragma region DahdsrEnvelopeState_Decay

DahdsrEnvelopeState_Decay::DahdsrEnvelopeState_Decay(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
{
    implementedDahdsrEnvelopeStateIndex = DahdsrEnvelopeStateIndex::decay;
    this->transitionTo = transitionTo;
}

DahdsrEnvelopeState_Decay::~DahdsrEnvelopeState_Decay()
{
    transitionTo = nullptr;
}

void DahdsrEnvelopeState_Decay::sampleRateChanged(double newSampleRate, bool triggerTransition)
{
    if (newSampleRate != sampleRate && newSampleRate > 0.0)
    {
        sampleRate = newSampleRate;
        setTime(timeInSeconds); //update time to new sample rate
        resetState();

        if (triggerTransition)
        {
            //return to idle state to reset
            transitionTo(DahdsrEnvelopeStateIndex::idle, 0.0);
        }
    }
}

void DahdsrEnvelopeState_Decay::noteOn()
{
    //transition back to delay
    transitionTo(DahdsrEnvelopeStateIndex::delay, envelopeLevelCurrent);
    resetState();
}

double DahdsrEnvelopeState_Decay::getNextEnvelopeSample()
{
    if (currentSample < timeInSamples - 1)
    {
        double currentValue = envelopeLevelCurrent;
        envelopeLevelCurrent += envelopeIncrementPerSample;
        currentSample += 1;
        return currentValue;
    }
    else //last sample -> transition to sustain state
    {
        transitionTo(DahdsrEnvelopeStateIndex::sustain, envelopeLevelEnd);
        resetState();
        return envelopeLevelEnd;
    }
}

void DahdsrEnvelopeState_Decay::noteOff()
{
    //transition directly to release (release starting level is set to the current envelope level)
    transitionTo(DahdsrEnvelopeStateIndex::release, envelopeLevelCurrent);
    resetState();
}

void DahdsrEnvelopeState_Decay::forceStop()
{
    //immediately return to idle
    transitionTo(DahdsrEnvelopeStateIndex::idle, envelopeLevelCurrent);
    resetState();
}

void DahdsrEnvelopeState_Decay::setStartingLevel(double startingLevel)
{
    envelopeLevelStart = startingLevel;
    updateEnvelopeIncrementPerSample();
    resetState();
}
double DahdsrEnvelopeState_Decay::getStartingLevel()
{
    return envelopeLevelStart;
}

//================================================================

void DahdsrEnvelopeState_Decay::setTime(double newTimeInSeconds)
{
    timeInSeconds = newTimeInSeconds;
    timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

    currentSample = 0;

    updateEnvelopeIncrementPerSample();
}
double DahdsrEnvelopeState_Decay::getTimeInSeconds()
{
    return timeInSeconds;
}

void DahdsrEnvelopeState_Decay::setEndLevel(double newEndLevel)
{
    envelopeLevelEnd = newEndLevel;
    updateEnvelopeIncrementPerSample();
    resetState();
}
double DahdsrEnvelopeState_Decay::getEndLevel()
{
    return envelopeLevelEnd;
}

void DahdsrEnvelopeState_Decay::resetState()
{
    currentSample = 0;
    envelopeLevelCurrent = envelopeLevelStart;
}

void DahdsrEnvelopeState_Decay::updateEnvelopeIncrementPerSample()
{
    if (timeInSamples > 0)
    {
        envelopeIncrementPerSample = (envelopeLevelEnd - envelopeLevelStart) / static_cast<double>(timeInSamples);
    }
    else
    {
        envelopeIncrementPerSample = 0.0;
    }
}

#pragma endregion DahdsrEnvelopeState_Decay




#pragma region DahdsrEnvelopeState_Sustain

DahdsrEnvelopeState_Sustain::DahdsrEnvelopeState_Sustain(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
{
    implementedDahdsrEnvelopeStateIndex = DahdsrEnvelopeStateIndex::sustain;
    this->transitionTo = transitionTo;
}

DahdsrEnvelopeState_Sustain::~DahdsrEnvelopeState_Sustain()
{
    transitionTo = nullptr;
}

void DahdsrEnvelopeState_Sustain::sampleRateChanged(double newSampleRate, bool triggerTransition)
{
    //the sustain state doesn't require a sample rate to work

    if (triggerTransition)
    {
        //return to idle state to reset
        transitionTo(DahdsrEnvelopeStateIndex::idle, 0.0);
    }
}

void DahdsrEnvelopeState_Sustain::noteOn()
{
    //transition back to delay
    transitionTo(DahdsrEnvelopeStateIndex::delay, level);
}

double DahdsrEnvelopeState_Sustain::getNextEnvelopeSample()
{
    return level; //constant level
}

void DahdsrEnvelopeState_Sustain::noteOff()
{
    //only transition to the next state (-> release) after the sound is stopping
    transitionTo(DahdsrEnvelopeStateIndex::release, level);
}

void DahdsrEnvelopeState_Sustain::forceStop()
{
    //immediately return to idle
    transitionTo(DahdsrEnvelopeStateIndex::idle, level);
}

void DahdsrEnvelopeState_Sustain::setStartingLevel(double startingLevel)
{
    //no starting level -> does nothing
}

//================================================================

void DahdsrEnvelopeState_Sustain::setLevel(double newLevel)
{
    level = newLevel;
}
double DahdsrEnvelopeState_Sustain::getLevel()
{
    return level;
}

void DahdsrEnvelopeState_Sustain::resetState()
{
    //nothing to reset
}

#pragma endregion DahdsrEnvelopeState_Sustain




#pragma region DahdsrEnvelopeState_Release

DahdsrEnvelopeState_Release::DahdsrEnvelopeState_Release(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
{
    implementedDahdsrEnvelopeStateIndex = DahdsrEnvelopeStateIndex::release;
    this->transitionTo = transitionTo;
}

DahdsrEnvelopeState_Release::~DahdsrEnvelopeState_Release()
{
    transitionTo = nullptr;
}

void DahdsrEnvelopeState_Release::sampleRateChanged(double newSampleRate, bool triggerTransition)
{
    if (newSampleRate != sampleRate && newSampleRate > 0.0)
    {
        sampleRate = newSampleRate;
        setTime(timeInSeconds); //update time to new sample rate
        resetState();

        if (triggerTransition)
        {
            //return to idle state to reset
            transitionTo(DahdsrEnvelopeStateIndex::idle, 0.0);
        }
    }
}

void DahdsrEnvelopeState_Release::noteOn()
{
    //transition back to delay
    transitionTo(DahdsrEnvelopeStateIndex::delay, envelopeLevelCurrent);
    resetState();
}

double DahdsrEnvelopeState_Release::getNextEnvelopeSample()
{
    if (currentSample < timeInSamples - 1)
    {
        double currentValue = envelopeLevelCurrent;
        envelopeLevelCurrent += envelopeIncrementPerSample;
        currentSample += 1;
        return currentValue;
    }
    else //last sample -> back to idle state
    {
        transitionTo(DahdsrEnvelopeStateIndex::idle, 0.0);
        resetState();
        return 0.0; //end level is always zero
    }
}

void DahdsrEnvelopeState_Release::noteOff()
{
    //note has already been released to get to this state -> does nothing
}

void DahdsrEnvelopeState_Release::forceStop()
{
    //immediately return to idle
    transitionTo(DahdsrEnvelopeStateIndex::idle, envelopeLevelCurrent);
    resetState();
}

void DahdsrEnvelopeState_Release::setStartingLevel(double startingLevel)
{
    envelopeLevelStart = startingLevel;
    updateEnvelopeIncrementPerSample();
    resetState();
}
double DahdsrEnvelopeState_Release::getStartingLevel()
{
    return envelopeLevelStart;
}

//================================================================

void DahdsrEnvelopeState_Release::setTime(double newTimeInSeconds)
{
    timeInSeconds = newTimeInSeconds;
    timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

    currentSample = 0;

    updateEnvelopeIncrementPerSample();
}
double DahdsrEnvelopeState_Release::getTimeInSeconds() //tailoff length
{
    return timeInSeconds;
}

void DahdsrEnvelopeState_Release::resetState()
{
    currentSample = 0;
    envelopeLevelCurrent = envelopeLevelStart;
}

void DahdsrEnvelopeState_Release::updateEnvelopeIncrementPerSample()
{
    if (timeInSamples > 0)
    {
        envelopeIncrementPerSample = (0.0 - envelopeLevelStart) / static_cast<double>(timeInSamples); //envelopeLevelEnd is always zero
    }
    else
    {
        envelopeIncrementPerSample = 0.0;
    }
}

#pragma endregion DahdsrEnvelopeState_Release
