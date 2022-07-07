/*
  ==============================================================================

    DahdsrEnvelope.h
    Created: 6 Jul 2022 9:46:48am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DahdsrEnvelopeStates.h"

/// <summary>
/// Implements a DAHDSR (delay, attack, hold, decay, sustain, release) envelope curve
/// </summary>
class DahdsrEnvelope
{
public:
    DahdsrEnvelope(double delayTimeSeconds = 0.0,
                   double attackTimeSeconds = 0.5, double initialLevel = 0.0,
                   double holdTimeSeconds = 0.1, double peakLevel = 1.0,
                   double decayTimeSeconds = 1.0, double sustainLevel = 0.5,
                   double releaseTimeSeconds = 1.0)
    {
        //fill states lookup table
        for (int i = 0; i < StateIndexCount; ++i)
        {
            switch (i)
            {
            case StateIndex::unprepared:
                states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Unprepared([this](DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
                break;
            case StateIndex::idle:
                states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Idle([this](DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
                break;
            case StateIndex::delay:
                states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Delay([this](DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
                break;
            case StateIndex::attack:
                states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Attack([this](DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
                break;
            case StateIndex::hold:
                states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Hold([this](DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
                break;
            case StateIndex::decay:
                states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Decay([this](DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
                break;
            case StateIndex::sustain:
                states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Sustain([this](DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
                break;
            case StateIndex::release:
                states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Release([this](DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
                break;
            default:
                throw std::exception("unhandled state index. are any states not implemented above?");
            }
        }
        jassert(states.size() == StateIndexCount);

        //set states' parameters
        setDelayTime(delayTimeSeconds);
        setInitialLevel(initialLevel);
        setAttackTime(attackTimeSeconds);
        setPeakLevel(peakLevel);
        setHoldTime(holdTimeSeconds);
        setDecayTime(decayTimeSeconds);
        setSustainLevel(sustainLevel);
        setReleaseTime(releaseTimeSeconds);
    }

    ~DahdsrEnvelope()
    {
        states.clear();
    }

    void setSampleRate(double newSampleRate)
    {
        for (int i = 0; i < StateIndexCount; ++i)
        {
            if (i != currentStateIndex)
            {
                states[i]->sampleRateChanged(newSampleRate, false); //don't trigger transitions (back to idle) for transitions other than the current one
            }
        }
        states[currentStateIndex]->sampleRateChanged(newSampleRate, true); //for the current state, *do* trigger the transition to idle (if applicable)
    }

    void noteOn()
    {
        states[currentStateIndex]->noteOn(); //automatically transitions states where applicable
    }

    void noteOff() //triggers release; called in Voice::noteOff
    {
        states[currentStateIndex]->noteOff(); //automatically transitions states where applicable
    }

    double getNextEnvelopeSample() //gets the envelope value of the current position in the current state; called in Voice::renderNextBlock
    {
        return states[currentStateIndex]->getNextEnvelopeSample(); //automatically transitions states where applicable
    }

    //double getTailLength()
    //{
    //    return dynamic_cast<DahdsrEnvelopeState_Release*>(states[release])->getTimeInSeconds();
    //}

    enum StateIndex : int
    {
        unprepared = 0,
        idle,
        delay,
        attack,
        hold,
        decay,
        sustain,
        release,
        StateIndexCount
    };

    void transitionToState(DahdsrEnvelope::StateIndex stateToTransitionTo, double currentEnvelopeLevel)
    {
        bool nonInstantStateFound = false;

        while (!nonInstantStateFound) //finite states are skipped if their time is zero
        {
            //switch to the new state and pass the current envelope level (doesn't really matter for all states, but it's simply more consistent to do so for all of them)
            switch (stateToTransitionTo)
            {
            case StateIndex::unprepared:
                //auto* unpreparedState = dynamic_cast<DahdsrEnvelopeState_Unprepared*>(states[unprepared]);
                //unpreparedState->setStartingLevel(currentEnvelopeLevel); //has no effect
                nonInstantStateFound = true;
                break;

            case StateIndex::idle:
                //auto* idleState = dynamic_cast<DahdsrEnvelopeState_Idle*>(states[idle]);
                //idleState->setStartingLevel(currentEnvelopeLevel); //has no effect
                nonInstantStateFound = true;
                break;

            case StateIndex::delay:
                auto* delayState = dynamic_cast<DahdsrEnvelopeState_Delay*>(states[delay]);
                if (delayState->getTimeInSeconds() == 0.0)
                {
                    stateToTransitionTo = attack;
                    continue;
                }
                else
                {
                    //delayState->setStartingLevel(currentEnvelopeLevel); //has no effect
                    nonInstantStateFound = true;
                }
                break;

            case StateIndex::attack:
                auto* attackState = dynamic_cast<DahdsrEnvelopeState_Attack*>(states[attack]);
                if (attackState->getTimeInSeconds() == 0.0)
                {
                    stateToTransitionTo = hold;
                    continue;
                }
                else
                {
                    //attackState->setStartingLevel(currentEnvelopeLevel); //attack actually has a fixed starting value; don't overwrite that
                    nonInstantStateFound = true;
                }
                break;

            case StateIndex::hold:
                auto* holdState = dynamic_cast<DahdsrEnvelopeState_Hold*>(states[hold]);
                if (holdState->getTimeInSeconds() == 0.0)
                {
                    stateToTransitionTo = decay;
                    continue;
                }
                else
                {
                    //holdState->setStartingLevel(currentEnvelopeLevel); //has no effect
                    nonInstantStateFound = true;
                }
                break;

            case StateIndex::decay:
                auto* decayState = dynamic_cast<DahdsrEnvelopeState_Decay*>(states[decay]);
                if (decayState->getTimeInSeconds() == 0.0)
                {
                    stateToTransitionTo = sustain;
                    continue;
                }
                else
                {
                    //decayState->setStartingLevel(currentEnvelopeLevel); //fixed starting value; don't overwrite
                    nonInstantStateFound = true;
                }
                break;

            case StateIndex::sustain:
                //auto* sustainState = dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[sustain]);
                //sustainState->setStartingLevel(currentEnvelopeLevel); //has no effect
                nonInstantStateFound = true;
                break;

            case StateIndex::release:
                auto* releaseState = dynamic_cast<DahdsrEnvelopeState_Release*>(states[release]);
                if (releaseState->getTimeInSeconds() == 0.0)
                {
                    stateToTransitionTo = idle;
                    continue;
                }
                else
                {
                    releaseState->setStartingLevel(currentEnvelopeLevel); //starting value can be overwritten since any playing state can transition to release at any time
                    nonInstantStateFound = true;
                }
                break;

            default:
                throw std::exception("unhandled state index");
            }
        }

        //finally, update the current state index
        currentStateIndex = stateToTransitionTo;
    }

    void forceStop()
    {
        states[currentStateIndex]->forceStop(); //automatically transitions states where applicable
    }

    //================================================================

    void setDelayTime(double newTimeInSeconds)
    {
        dynamic_cast<DahdsrEnvelopeState_Delay*>(states[delay])->setTime(newTimeInSeconds);
    }
    double getDelayTime()
    {
        return dynamic_cast<DahdsrEnvelopeState_Delay*>(states[delay])->getTimeInSeconds();
    }

    void setInitialLevel(double newInitialLevel)
    {
        dynamic_cast<DahdsrEnvelopeState_Attack*>(states[attack])->setStartingLevel(newInitialLevel);
    }
    double getInitialLevel()
    {
        return dynamic_cast<DahdsrEnvelopeState_Attack*>(states[attack])->getStartingLevel();
    }

    void setAttackTime(double newTimeInSeconds)
    {
        dynamic_cast<DahdsrEnvelopeState_Attack*>(states[attack])->setTime(newTimeInSeconds);
    }
    double getAttackTime()
    {
        return dynamic_cast<DahdsrEnvelopeState_Attack*>(states[attack])->getTimeInSeconds();
    }

    void setPeakLevel(double newPeakLevel)
    {
        dynamic_cast<DahdsrEnvelopeState_Attack*>(states[attack])->setEndLevel(newPeakLevel);
        dynamic_cast<DahdsrEnvelopeState_Hold*>(states[hold])->setLevel(newPeakLevel);
        dynamic_cast<DahdsrEnvelopeState_Decay*>(states[decay])->setStartingLevel(newPeakLevel);
    }
    double getPeakLevel()
    {
        return dynamic_cast<DahdsrEnvelopeState_Hold*>(states[hold])->getLevel();
    }

    void setHoldTime(double newTimeInSeconds)
    {
        dynamic_cast<DahdsrEnvelopeState_Hold*>(states[hold])->setTime(newTimeInSeconds);
    }
    double getHoldTime()
    {
        return dynamic_cast<DahdsrEnvelopeState_Hold*>(states[hold])->getTimeInSeconds();
    }

    void setDecayTime(double newTimeInSeconds)
    {
        dynamic_cast<DahdsrEnvelopeState_Decay*>(states[decay])->setTime(newTimeInSeconds);
    }
    double getDecayTime()
    {
        return dynamic_cast<DahdsrEnvelopeState_Decay*>(states[decay])->getTimeInSeconds();
    }

    void setSustainLevel(double newSustainLevel)
    {
        dynamic_cast<DahdsrEnvelopeState_Decay*>(states[decay])->setEndLevel(newSustainLevel);
        dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[sustain])->setLevel(newSustainLevel);
        //dynamic_cast<DahdsrEnvelopeState_Release*>(states[release])->setStartingLevel(newSustainLevel); //doesn't matter thaaaat much if it isn't set here since this value is overwritten when any playing state transitions to release
    }
    double getSustainLevel()
    {
        return dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[sustain])->getLevel();
    }

    void setReleaseTime(double newTimeInSeconds)
    {
        dynamic_cast<DahdsrEnvelopeState_Release*>(states[release])->setTime(newTimeInSeconds);
    }
    double getReleaseTime()
    {
        return dynamic_cast<DahdsrEnvelopeState_Release*>(states[release])->getTimeInSeconds();
    }

private:
    juce::OwnedArray<DahdsrEnvelopeState> states;

    const StateIndex initialStateIndex = unprepared;
    StateIndex currentStateIndex = initialStateIndex;
};