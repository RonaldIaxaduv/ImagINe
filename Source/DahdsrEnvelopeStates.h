/*
  ==============================================================================

    DahdsrEnvelopeStates.h
    Created: 7 Jul 2022 4:33:56pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once
#include "DahdsrEnvelope.h"

/// <summary>
/// abstract base class of all DAHDSR state implementations, containing all the methods that a state must provide
/// </summary>
class DahdsrEnvelopeState
{
public:
    //envelope events to implement
    virtual void sampleRateChanged(double newSampleRate, bool triggerTransition) = 0;

    virtual void noteOn() = 0;
    virtual double getNextEnvelopeSample() = 0;
    virtual void noteOff() = 0;

    virtual void forceStop() = 0;

    //state events to implement (especially important for the release state)
    virtual void setStartingLevel(double startingLevel) = 0;

protected:
    virtual void resetState() = 0;

    static DahdsrEnvelope::StateIndex implementedStateIndex; //corresponds to the index of the state that the derived class implements
    //DahdsrEnvelope::StateIndex* parentStateIndex = nullptr; //index of the actual DahdsrEnvelope class. the state can change this value and thus make the envelope enter a different state

    std::function<void(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo; //this function is used to make the actual envelope transition into a different state. the current envelope value is passed so that the next state may set its starting level if desired
};

/// <summary>
/// implements the state of the envelope while it's unprepared, i.e. while the reference sample rate has not yet been set
/// </summary>
class DahdsrEnvelopeState_Unprepared : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Unprepared(std::function<void(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
    {
        implementedStateIndex = DahdsrEnvelope::StateIndex::unprepared;
        this->transitionTo = transitionTo;
    }

    ~DahdsrEnvelopeState_Unprepared()
    {
        transitionTo = nullptr;
    }

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override
    {
        if (triggerTransition)
        {
            //transition to next state: idle
            transitionTo(DahdsrEnvelope::StateIndex::idle, 0.0);
        }
    }

    void noteOn() override
    {
        std::exception("Envelope has not been prepared yet, so it cannot handle notes.");
    }

    double getNextEnvelopeSample() override
    {
        std::exception("Envelope has not been prepared yet, so it cannot provide envelope samples.");
    }

    void noteOff() override
    {
        std::exception("Envelope has not been prepared yet, so it cannot handle notes.");
    }

    void forceStop() override
    {
        //isn't playing yet -> do nothing
    }

    void setStartingLevel(double startingLevel) override
    {
        //doesn't have any volume
    }

protected:
    void resetState() override
    {
        //nothing to reset
    }
};

/// <summary>
/// implements the state of the envelope while it's idle, i.e. while the sample rate has been set, but no sound is playing yet
/// </summary>
class DahdsrEnvelopeState_Idle : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Idle(std::function<void(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
    {
        implementedStateIndex = DahdsrEnvelope::StateIndex::idle;
        this->transitionTo = transitionTo;
    }

    ~DahdsrEnvelopeState_Idle()
    {
        transitionTo = nullptr;
    }

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override
    {
        //already prepared -> do nothing
    }

    void noteOn() override
    {
        //begin delay time
        transitionTo(DahdsrEnvelope::StateIndex::delay, 0.0);
    }

    double getNextEnvelopeSample() override
    {
        return 0.0; //no sound playing yet
    }

    void noteOff() override
    {
        //no sound playing yet -> do nothing
    }

    void forceStop() override
    {
        //no sound playing yet anyway -> do nothing
    }

    void setStartingLevel(double startingLevel) override
    {
        //doesn't have any volume
    }

protected:
    void resetState() override
    {
        //nothing to reset
    }
};

/// <summary>
/// implements the delay time of the envelope
/// </summary>
class DahdsrEnvelopeState_Delay : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Delay(std::function<void(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
    {
        implementedStateIndex = DahdsrEnvelope::StateIndex::delay;
        this->transitionTo = transitionTo;
    }

    ~DahdsrEnvelopeState_Delay()
    {
        transitionTo = nullptr;
    }

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override
    {
        if (newSampleRate != sampleRate && newSampleRate > 0.0)
        {
            sampleRate = newSampleRate;
            setTime(timeInSeconds); //update time to new sample rate
            resetState();

            if (triggerTransition)
            {
                //return to idle state to reset
                transitionTo(DahdsrEnvelope::StateIndex::idle, 0.0);
            }
        }
    }

    void noteOn() override
    {
        //restart delay time
        resetState();
    }

    double getNextEnvelopeSample() override
    {
        if (currentSample < timeInSamples - 1)
        {
            currentSample += 1;
            return 0.0;
        }
        else //last sample -> transition to attack state
        {
            transitionTo(DahdsrEnvelope::StateIndex::attack, 0.0);
            resetState();
            return 0.0;
        }
    }

    void noteOff() override
    {
        //transition directly to release (since no sound is playing yet, it could also directly transition to idle, but this is more consistent and thus preferable imo)
        transitionTo(DahdsrEnvelope::StateIndex::release, 0.0);
        resetState();
    }

    void forceStop() override
    {
        //immediately return to idle
        transitionTo(DahdsrEnvelope::StateIndex::idle, 0.0);
        resetState();
    }

    void setStartingLevel(double startingLevel) override
    {
        //doesn't have any volume
    }

    //================================================================

    void setTime(double newTimeInSeconds)
    {
        timeInSeconds = newTimeInSeconds;
        timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

        currentSample = juce::jmin<int>(currentSample, timeInSamples - 1);
    }
    double getTimeInSeconds()
    {
        return timeInSeconds;
    }

protected:
    void resetState() override
    {
        currentSample = 0;
    }

private:
    double sampleRate = 0.0;
    double timeInSeconds = 0.0;
    int timeInSamples = 0;
    int currentSample = 0;
};

/// <summary>
/// implements the attack time of the envelope
/// </summary>
class DahdsrEnvelopeState_Attack : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Attack(std::function<void(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
    {
        implementedStateIndex = DahdsrEnvelope::StateIndex::attack;
        this->transitionTo = transitionTo;
    }

    ~DahdsrEnvelopeState_Attack()
    {
        transitionTo = nullptr;
    }

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override
    {
        if (newSampleRate != sampleRate && newSampleRate > 0.0)
        {
            sampleRate = newSampleRate;
            setTime(timeInSeconds); //update time to new sample rate
            resetState();

            if (triggerTransition)
            {
                //return to idle state to reset
                transitionTo(DahdsrEnvelope::StateIndex::idle, 0.0);
            }
        }
    }

    void noteOn() override
    {
        //transition back to delay
        transitionTo(DahdsrEnvelope::StateIndex::delay, envelopeLevelCurrent);
        resetState();
    }

    double getNextEnvelopeSample() override
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
            transitionTo(DahdsrEnvelope::StateIndex::hold, envelopeLevelEnd);
            resetState();
            return envelopeLevelEnd;
        }
    }

    void noteOff() override
    {
        //transition directly to release (release starting level is set to the current envelope level)
        transitionTo(DahdsrEnvelope::StateIndex::release, envelopeLevelCurrent);
        resetState();
    }

    void forceStop() override
    {
        //immediately return to idle
        transitionTo(DahdsrEnvelope::StateIndex::idle, envelopeLevelCurrent);
        resetState();
    }

    void setStartingLevel(double startingLevel) override
    {
        envelopeLevelStart = startingLevel;
        updateEnvelopeIncrementPerSample();
        resetState();
    }
    double getStartingLevel()
    {
        return envelopeLevelStart;
    }

    //================================================================

    void setTime(double newTimeInSeconds)
    {
        timeInSeconds = newTimeInSeconds;
        timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

        currentSample = juce::jmin<int>(currentSample, timeInSamples - 1);

        updateEnvelopeIncrementPerSample();
    }
    double getTimeInSeconds()
    {
        return timeInSeconds;
    }

    void setEndLevel(double newEndLevel)
    {
        envelopeLevelEnd = newEndLevel;
        updateEnvelopeIncrementPerSample();
        resetState();
    }
    double getEndLevel()
    {
        return envelopeLevelEnd;
    }

protected:
    void resetState() override
    {
        currentSample = 0;
        envelopeLevelCurrent = envelopeLevelStart;
    }

private:
    void updateEnvelopeIncrementPerSample()
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

    double sampleRate = 0.0;
    double timeInSeconds = 0.0;
    int timeInSamples = 0;
    int currentSample = 0;

    double envelopeLevelStart = 0.0;
    double envelopeLevelCurrent = 0.0;
    double envelopeLevelEnd = 0.0;

    double envelopeIncrementPerSample = 0.0;
};

/// <summary>
/// implements the hold time of the envelope
/// </summary>
class DahdsrEnvelopeState_Hold : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Hold(std::function<void(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
    {
        implementedStateIndex = DahdsrEnvelope::StateIndex::hold;
        this->transitionTo = transitionTo;
    }

    ~DahdsrEnvelopeState_Hold()
    {
        transitionTo = nullptr;
    }

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override
    {
        if (newSampleRate != sampleRate && newSampleRate > 0.0)
        {
            sampleRate = newSampleRate;
            setTime(timeInSeconds); //update time to new sample rate
            resetState();

            if (triggerTransition)
            {
                //return to idle state to reset
                transitionTo(DahdsrEnvelope::StateIndex::idle, 0.0);
            }
        }
    }

    void noteOn() override
    {
        //transition back to delay
        transitionTo(DahdsrEnvelope::StateIndex::delay, level);
        resetState();
    }

    double getNextEnvelopeSample() override
    {
        if (currentSample < timeInSamples - 1)
        {
            currentSample += 1;
            return level; //constant level
        }
        else //last sample -> transition to decay state
        {
            transitionTo(DahdsrEnvelope::StateIndex::decay, level);
            resetState();
            return level;
        }
    }

    void noteOff() override
    {
        //transition directly to release (release starting level is set to the current envelope level)
        transitionTo(DahdsrEnvelope::StateIndex::release, level);
        resetState();
    }

    void forceStop() override
    {
        //immediately return to idle
        transitionTo(DahdsrEnvelope::StateIndex::idle, level);
        resetState();
    }

    void setStartingLevel(double startingLevel) override
    {
        //no starting level -> does nothing
    }

    //================================================================

    void setTime(double newTimeInSeconds)
    {
        timeInSeconds = newTimeInSeconds;
        timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

        currentSample = juce::jmin<int>(currentSample, timeInSamples - 1);
    }
    double getTimeInSeconds()
    {
        return timeInSeconds;
    }

    void setLevel(double newLevel)
    {
        level = newLevel;
    }
    double getLevel()
    {
        return level;
    }

protected:
    void resetState() override
    {
        currentSample = 0;
    }

private:

    double sampleRate = 0.0;
    double timeInSeconds = 0.0;
    int timeInSamples = 0;
    int currentSample = 0;

    double level = 0.0;
};

/// <summary>
/// implements the decay time of the envelope
/// </summary>
class DahdsrEnvelopeState_Decay : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Decay(std::function<void(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
    {
        implementedStateIndex = DahdsrEnvelope::StateIndex::decay;
        this->transitionTo = transitionTo;
    }

    ~DahdsrEnvelopeState_Decay()
    {
        transitionTo = nullptr;
    }

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override
    {
        if (newSampleRate != sampleRate && newSampleRate > 0.0)
        {
            sampleRate = newSampleRate;
            setTime(timeInSeconds); //update time to new sample rate
            resetState();

            if (triggerTransition)
            {
                //return to idle state to reset
                transitionTo(DahdsrEnvelope::StateIndex::idle, 0.0);
            }
        }
    }

    void noteOn() override
    {
        //transition back to delay
        transitionTo(DahdsrEnvelope::StateIndex::delay, envelopeLevelCurrent);
        resetState();
    }

    double getNextEnvelopeSample() override
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
            transitionTo(DahdsrEnvelope::StateIndex::sustain, envelopeLevelEnd);
            resetState();
            return envelopeLevelEnd;
        }
    }

    void noteOff() override
    {
        //transition directly to release (release starting level is set to the current envelope level)
        transitionTo(DahdsrEnvelope::StateIndex::release, envelopeLevelCurrent);
        resetState();
    }

    void forceStop() override
    {
        //immediately return to idle
        transitionTo(DahdsrEnvelope::StateIndex::idle, envelopeLevelCurrent);
        resetState();
    }

    void setStartingLevel(double startingLevel) override
    {
        envelopeLevelStart = startingLevel;
        updateEnvelopeIncrementPerSample();
        resetState();
    }
    double getStartingLevel()
    {
        return envelopeLevelStart;
    }

    //================================================================

    void setTime(double newTimeInSeconds)
    {
        timeInSeconds = newTimeInSeconds;
        timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

        currentSample = juce::jmin<int>(currentSample, timeInSamples - 1);

        updateEnvelopeIncrementPerSample();
    }
    double getTimeInSeconds()
    {
        return timeInSeconds;
    }

    void setEndLevel(double newEndLevel)
    {
        envelopeLevelEnd = newEndLevel;
        updateEnvelopeIncrementPerSample();
        resetState();
    }
    double getEndLevel()
    {
        return envelopeLevelEnd;
    }

protected:
    void resetState() override
    {
        currentSample = 0;
        envelopeLevelCurrent = envelopeLevelStart;
    }

private:
    void updateEnvelopeIncrementPerSample()
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

    double sampleRate = 0.0;
    double timeInSeconds = 0.0;
    int timeInSamples = 0;
    int currentSample = 0;

    double envelopeLevelStart = 0.0;
    double envelopeLevelCurrent = 0.0;
    double envelopeLevelEnd = 0.0;

    double envelopeIncrementPerSample = 0.0;
};

/// <summary>
/// implements the sustain time of the envelope
/// </summary>
class DahdsrEnvelopeState_Sustain : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Sustain(std::function<void(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
    {
        implementedStateIndex = DahdsrEnvelope::StateIndex::sustain;
        this->transitionTo = transitionTo;
    }

    ~DahdsrEnvelopeState_Sustain()
    {
        transitionTo = nullptr;
    }

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override
    {
        if (newSampleRate != sampleRate && newSampleRate > 0.0)
        {
            sampleRate = newSampleRate;
            setTime(timeInSeconds); //update time to new sample rate
            resetState();

            if (triggerTransition)
            {
                //return to idle state to reset
                transitionTo(DahdsrEnvelope::StateIndex::idle, 0.0);
            }
        }
    }

    void noteOn() override
    {
        //transition back to delay
        transitionTo(DahdsrEnvelope::StateIndex::delay, level);
        resetState();
    }

    double getNextEnvelopeSample() override
    {
        if (currentSample < timeInSamples - 1)
        {
            currentSample += 1;
            return level; //constant level
        }
        else //last sample -> transition to release state
        {
            transitionTo(DahdsrEnvelope::StateIndex::release, level);
            resetState();
            return level;
        }
    }

    void noteOff() override
    {
        //transition directly to release a little earlier (release starting level is set to the current envelope level)
        transitionTo(DahdsrEnvelope::StateIndex::release, level);
        resetState();
    }

    void forceStop() override
    {
        //immediately return to idle
        transitionTo(DahdsrEnvelope::StateIndex::idle, level);
        resetState();
    }

    void setStartingLevel(double startingLevel) override
    {
        //no starting level -> does nothing
    }

    //================================================================

    void setTime(double newTimeInSeconds)
    {
        timeInSeconds = newTimeInSeconds;
        timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

        currentSample = juce::jmin<int>(currentSample, timeInSamples - 1);
    }

    void setLevel(double newLevel)
    {
        level = newLevel;
    }
    double getLevel()
    {
        return level;
    }

protected:
    void resetState() override
    {
        currentSample = 0;
    }

private:

    double sampleRate = 0.0;
    double timeInSeconds = 0.0;
    int timeInSamples = 0;
    int currentSample = 0;

    double level = 0.0;
};

/// <summary>
/// implements the release time of the envelope
/// </summary>
class DahdsrEnvelopeState_Release : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Release(std::function<void(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo)
    {
        implementedStateIndex = DahdsrEnvelope::StateIndex::release;
        this->transitionTo = transitionTo;
    }

    ~DahdsrEnvelopeState_Release()
    {
        transitionTo = nullptr;
    }

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override
    {
        if (newSampleRate != sampleRate && newSampleRate > 0.0)
        {
            sampleRate = newSampleRate;
            setTime(timeInSeconds); //update time to new sample rate
            resetState();

            if (triggerTransition)
            {
                //return to idle state to reset
                transitionTo(DahdsrEnvelope::StateIndex::idle, 0.0);
            }
        }
    }

    void noteOn() override
    {
        //transition back to delay
        transitionTo(DahdsrEnvelope::StateIndex::delay, envelopeLevelCurrent);
        resetState();
    }

    double getNextEnvelopeSample() override
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
            transitionTo(DahdsrEnvelope::StateIndex::idle, 0.0);
            resetState();
            return 0.0; //end level is always zero
        }
    }

    void noteOff() override
    {
        //note has already been released to get to this state -> does nothing
    }

    void forceStop() override
    {
        //immediately return to idle
        transitionTo(DahdsrEnvelope::StateIndex::idle, envelopeLevelCurrent);
        resetState();
    }

    void setStartingLevel(double startingLevel) override
    {
        envelopeLevelStart = startingLevel;
        updateEnvelopeIncrementPerSample();
        resetState();
    }
    double getStartingLevel()
    {
        return envelopeLevelStart;
    }

    //================================================================

    void setTime(double newTimeInSeconds)
    {
        timeInSeconds = newTimeInSeconds;
        timeInSamples = timeInSeconds * sampleRate; //0 if sample rate not set -> should not happen if the states are implemented and updated correctly

        currentSample = juce::jmin<int>(currentSample, timeInSamples - 1);

        updateEnvelopeIncrementPerSample();
    }
    double getTimeInSeconds() //tailoff length
    {
        return timeInSeconds;
    }

protected:
    void resetState() override
    {
        currentSample = 0;
        envelopeLevelCurrent = envelopeLevelStart;
    }

private:
    void updateEnvelopeIncrementPerSample()
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

    double sampleRate = 0.0;
    double timeInSeconds = 0.0;
    int timeInSamples = 0;
    int currentSample = 0;

    double envelopeLevelStart = 0.0;
    double envelopeLevelCurrent = 0.0;
    //double envelopeLevelEnd = 0.0; //for the release state, this is *always* zero

    double envelopeIncrementPerSample = 0.0; //this will always be negative for this state
};