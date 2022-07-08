/*
  ==============================================================================

    DahdsrEnvelopeStates.h
    Created: 7 Jul 2022 4:33:56pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

//forward reference to the envelope class (required to enable circular dependencies - see https://stackoverflow.com/questions/994253/two-classes-that-refer-to-each-other )
#include "DahdsrEnvelope.h"
class DahdsrEnvelope;


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

    DahdsrEnvelopeStateIndex implementedDahdsrEnvelopeStateIndex; //corresponds to the index of the state that the derived class implements
    //DahdsrEnvelopeStateIndex* parentDahdsrEnvelopeStateIndex = nullptr; //index of the actual DahdsrEnvelope class. the state can change this value and thus make the envelope enter a different state

    std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo; //this function is used to make the actual envelope transition into a different state. the current envelope value is passed so that the next state may set its starting level if desired
};

/// <summary>
/// implements the state of the envelope while it's unprepared, i.e. while the reference sample rate has not yet been set
/// </summary>
class DahdsrEnvelopeState_Unprepared : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Unprepared(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo);
    ~DahdsrEnvelopeState_Unprepared();

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override;
  
    void noteOn() override;
    double getNextEnvelopeSample() override;
    void noteOff() override;

    void forceStop() override;

    void setStartingLevel(double startingLevel) override;

protected:
    void resetState() override;
};

/// <summary>
/// implements the state of the envelope while it's idle, i.e. while the sample rate has been set, but no sound is playing yet
/// </summary>
class DahdsrEnvelopeState_Idle : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Idle(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo);
    ~DahdsrEnvelopeState_Idle();

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override;

    void noteOn() override;
    double getNextEnvelopeSample() override;
    void noteOff() override;

    void forceStop() override;

    void setStartingLevel(double startingLevel) override;

protected:
    void resetState() override;
};

/// <summary>
/// implements the delay time of the envelope
/// </summary>
class DahdsrEnvelopeState_Delay : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Delay(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo);
    ~DahdsrEnvelopeState_Delay();

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override;

    void noteOn() override;
    double getNextEnvelopeSample() override;
    void noteOff() override;

    void forceStop() override;

    void setStartingLevel(double startingLevel) override;

    //================================================================

    void setTime(double newTimeInSeconds);
    double getTimeInSeconds();

protected:
    void resetState() override;

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
    DahdsrEnvelopeState_Attack(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo);
    ~DahdsrEnvelopeState_Attack();

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override;

    void noteOn() override;
    double getNextEnvelopeSample() override;
    void noteOff() override;

    void forceStop() override;

    void setStartingLevel(double startingLevel) override;
    double getStartingLevel();

    //================================================================

    void setTime(double newTimeInSeconds);
    double getTimeInSeconds();

    void setEndLevel(double newEndLevel);
    double getEndLevel();

protected:
    void resetState() override;

private:
    void updateEnvelopeIncrementPerSample();

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
    DahdsrEnvelopeState_Hold(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo);
    ~DahdsrEnvelopeState_Hold();

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override;

    void noteOn() override;
    double getNextEnvelopeSample() override;
    void noteOff() override;

    void forceStop() override;

    void setStartingLevel(double startingLevel) override;

    //================================================================

    void setTime(double newTimeInSeconds);
    double getTimeInSeconds();

    void setLevel(double newLevel);
    double getLevel();

protected:
    void resetState() override;

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
    DahdsrEnvelopeState_Decay(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo);
    ~DahdsrEnvelopeState_Decay();

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override;

    void noteOn() override;
    double getNextEnvelopeSample() override;
    void noteOff() override;

    void forceStop() override;

    void setStartingLevel(double startingLevel) override;
    double getStartingLevel();

    //================================================================

    void setTime(double newTimeInSeconds);
    double getTimeInSeconds();

    void setEndLevel(double newEndLevel);
    double getEndLevel();

protected:
    void resetState() override;

private:
    void updateEnvelopeIncrementPerSample();

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
    DahdsrEnvelopeState_Sustain(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo);
    ~DahdsrEnvelopeState_Sustain();

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override;

    void noteOn() override;
    double getNextEnvelopeSample() override;
    void noteOff() override;

    void forceStop() override;

    void setStartingLevel(double startingLevel) override;

    //================================================================

    void setLevel(double newLevel);
    double getLevel();

protected:
    void resetState() override;

private:
    double level = 0.0;
};

/// <summary>
/// implements the release time of the envelope
/// </summary>
class DahdsrEnvelopeState_Release : public DahdsrEnvelopeState
{
public:
    DahdsrEnvelopeState_Release(std::function<void(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)> transitionTo);
    ~DahdsrEnvelopeState_Release();

    void sampleRateChanged(double newSampleRate, bool triggerTransition) override;

    void noteOn() override;
    double getNextEnvelopeSample() override;
    void noteOff() override;

    void forceStop() override;

    void setStartingLevel(double startingLevel) override;
    double getStartingLevel();

    //================================================================

    void setTime(double newTimeInSeconds);
    double getTimeInSeconds();

protected:
    void resetState() override;

private:
    void updateEnvelopeIncrementPerSample();

    double sampleRate = 0.0;
    double timeInSeconds = 0.0;
    int timeInSamples = 0;
    int currentSample = 0;

    double envelopeLevelStart = 0.0;
    double envelopeLevelCurrent = 0.0;
    //double envelopeLevelEnd = 0.0; //for the release state, this is *always* zero

    double envelopeIncrementPerSample = 0.0; //this will always be negative for this state
};
