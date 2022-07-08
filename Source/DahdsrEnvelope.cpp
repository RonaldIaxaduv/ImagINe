/*
  ==============================================================================

    DahdsrEnvelope.cpp
    Created: 7 Jul 2022 7:14:56pm
    Author:  Aaron

  ==============================================================================
*/

#include "DahdsrEnvelope.h"

DahdsrEnvelope::DahdsrEnvelope(double delayTimeSeconds,
    double attackTimeSeconds, double initialLevel,
    double holdTimeSeconds, double peakLevel,
    double decayTimeSeconds, double sustainLevel,
    double releaseTimeSeconds)
{
    //initialStateIndex = DahdsrEnvelopeStateIndex::unprepared;
    currentStateIndex = initialStateIndex;

    //fill states lookup table
    for (int i = 0; i < DahdsrEnvelopeStateIndex::StateIndexCount; ++i) //WIP: need to static_cast all of them, i think...
    {
        switch (i)
        {
        case DahdsrEnvelopeStateIndex::unprepared:
            states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Unprepared([this](DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
            break;
        case DahdsrEnvelopeStateIndex::idle:
            states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Idle([this](DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
            break;
        case DahdsrEnvelopeStateIndex::delay:
            states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Delay([this](DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
            break;
        case DahdsrEnvelopeStateIndex::attack:
            states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Attack([this](DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
            break;
        case DahdsrEnvelopeStateIndex::hold:
            states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Hold([this](DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
            break;
        case DahdsrEnvelopeStateIndex::decay:
            states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Decay([this](DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
            break;
        case DahdsrEnvelopeStateIndex::sustain:
            states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Sustain([this](DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
            break;
        case DahdsrEnvelopeStateIndex::release:
            states.add(static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Release([this](DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel) { transitionToState(stateToTransitionTo, currentEnvelopeLevel); })));
            break;
        default:
            throw std::exception("unhandled state index. are any states not implemented above?");
        }
    }
    jassert(states.size() == DahdsrEnvelopeStateIndex::StateIndexCount);

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

DahdsrEnvelope::~DahdsrEnvelope()
{
    states.clear();
}

void DahdsrEnvelope::setSampleRate(double newSampleRate)
{
    for (int i = 0; i < DahdsrEnvelopeStateIndex::StateIndexCount; ++i)
    {
        if (i != currentStateIndex)
        {
            states[i]->sampleRateChanged(newSampleRate, false); //don't trigger transitions (back to idle) for transitions other than the current one
        }
    }
    states[currentStateIndex]->sampleRateChanged(newSampleRate, true); //for the current state, *do* trigger the transition to idle (if applicable)
}

void DahdsrEnvelope::noteOn()
{
    states[currentStateIndex]->noteOn(); //automatically transitions states where applicable
}

void DahdsrEnvelope::noteOff() //triggers release; called in Voice::noteOff
{
    states[currentStateIndex]->noteOff(); //automatically transitions states where applicable
}

double DahdsrEnvelope::getNextEnvelopeSample() //gets the envelope value of the current position in the current state; called in Voice::renderNextBlock
{
    return states[currentStateIndex]->getNextEnvelopeSample(); //automatically transitions states where applicable
}

//double DahdsrEnvelope::getTailLength()
//{
//    return dynamic_cast<DahdsrEnvelopeState_Release*>(states[release])->getTimeInSeconds();
//}

void DahdsrEnvelope::transitionToState(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel)
{
    bool nonInstantStateFound = false;

    while (!nonInstantStateFound) //finite states are skipped if their time is zero
    {
        //switch to the new state and pass the current envelope level (doesn't really matter for all states, but it's simply more consistent to do so for all of them)
        switch (stateToTransitionTo)
        {
        case DahdsrEnvelopeStateIndex::unprepared:
            //auto* unpreparedState = dynamic_cast<DahdsrEnvelopeState_Unprepared*>(states[DahdsrEnvelopeStateIndex::unprepared]);
            //unpreparedState->setStartingLevel(currentEnvelopeLevel); //has no effect
            nonInstantStateFound = true;
            break;

        case DahdsrEnvelopeStateIndex::idle:
            //auto* idleState = dynamic_cast<DahdsrEnvelopeState_Idle*>(states[DahdsrEnvelopeStateIndex::idle]);
            //idleState->setStartingLevel(currentEnvelopeLevel); //has no effect
            nonInstantStateFound = true;
            break;

        case DahdsrEnvelopeStateIndex::delay:
            //auto* delayState = dynamic_cast<DahdsrEnvelopeState_Delay*>(states[DahdsrEnvelopeStateIndex::delay]);
            if (dynamic_cast<DahdsrEnvelopeState_Delay*>(states[DahdsrEnvelopeStateIndex::delay])->getTimeInSeconds() == 0.0)
            {
                stateToTransitionTo = DahdsrEnvelopeStateIndex::attack;
                continue;
            }
            else
            {
                //delayState->setStartingLevel(currentEnvelopeLevel); //has no effect
                nonInstantStateFound = true;
            }
            break;

        case DahdsrEnvelopeStateIndex::attack:
            //auto* attackState = dynamic_cast<DahdsrEnvelopeState_Attack*>(states[DahdsrEnvelopeStateIndex::attack]);
            if (dynamic_cast<DahdsrEnvelopeState_Attack*>(states[DahdsrEnvelopeStateIndex::attack])->getTimeInSeconds() == 0.0)
            {
                stateToTransitionTo = DahdsrEnvelopeStateIndex::hold;
                continue;
            }
            else
            {
                //attackState->setStartingLevel(currentEnvelopeLevel); //attack actually has a fixed starting value; don't overwrite that
                nonInstantStateFound = true;
            }
            break;

        case DahdsrEnvelopeStateIndex::hold:
            //auto* holdState = dynamic_cast<DahdsrEnvelopeState_Hold*>(states[DahdsrEnvelopeStateIndex::hold]);
            if (dynamic_cast<DahdsrEnvelopeState_Hold*>(states[DahdsrEnvelopeStateIndex::hold])->getTimeInSeconds() == 0.0)
            {
                stateToTransitionTo = DahdsrEnvelopeStateIndex::decay;
                continue;
            }
            else
            {
                //holdState->setStartingLevel(currentEnvelopeLevel); //has no effect
                nonInstantStateFound = true;
            }
            break;

        case DahdsrEnvelopeStateIndex::decay:
            //auto* decayState = dynamic_cast<DahdsrEnvelopeState_Decay*>(states[DahdsrEnvelopeStateIndex::decay]);
            if (dynamic_cast<DahdsrEnvelopeState_Decay*>(states[DahdsrEnvelopeStateIndex::decay])->getTimeInSeconds() == 0.0)
            {
                stateToTransitionTo = DahdsrEnvelopeStateIndex::sustain;
                continue;
            }
            else
            {
                //decayState->setStartingLevel(currentEnvelopeLevel); //fixed starting value; don't overwrite
                nonInstantStateFound = true;
            }
            break;

        case DahdsrEnvelopeStateIndex::sustain:
            //auto* sustainState = dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[DahdsrEnvelopeStateIndex::sustain]);
            //sustainState->setStartingLevel(currentEnvelopeLevel); //has no effect
            nonInstantStateFound = true;
            break;

        case DahdsrEnvelopeStateIndex::release:
            //auto* releaseState = dynamic_cast<DahdsrEnvelopeState_Release*>(states[DahdsrEnvelopeStateIndex::release]);
            if (dynamic_cast<DahdsrEnvelopeState_Release*>(states[DahdsrEnvelopeStateIndex::release])->getTimeInSeconds() == 0.0)
            {
                stateToTransitionTo = DahdsrEnvelopeStateIndex::idle;
                continue;
            }
            else
            {
                dynamic_cast<DahdsrEnvelopeState_Release*>(states[DahdsrEnvelopeStateIndex::release])->setStartingLevel(currentEnvelopeLevel); //starting value can be overwritten since any playing state can transition to release at any time
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

void DahdsrEnvelope::forceStop()
{
    states[currentStateIndex]->forceStop(); //automatically transitions states where applicable
}

bool DahdsrEnvelope::isReleasing()
{
    return currentStateIndex == DahdsrEnvelopeStateIndex::release;
}

bool DahdsrEnvelope::isIdle()
{
    return currentStateIndex == DahdsrEnvelopeStateIndex::idle;
}

//================================================================

void DahdsrEnvelope::setDelayTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Delay*>(states[DahdsrEnvelopeStateIndex::delay])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getDelayTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Delay*>(states[DahdsrEnvelopeStateIndex::delay])->getTimeInSeconds();
}

void DahdsrEnvelope::setInitialLevel(double newInitialLevel)
{
    dynamic_cast<DahdsrEnvelopeState_Attack*>(states[DahdsrEnvelopeStateIndex::attack])->setStartingLevel(newInitialLevel);
}
double DahdsrEnvelope::getInitialLevel()
{
    return dynamic_cast<DahdsrEnvelopeState_Attack*>(states[DahdsrEnvelopeStateIndex::attack])->getStartingLevel();
}

void DahdsrEnvelope::setAttackTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Attack*>(states[DahdsrEnvelopeStateIndex::attack])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getAttackTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Attack*>(states[DahdsrEnvelopeStateIndex::attack])->getTimeInSeconds();
}

void DahdsrEnvelope::setPeakLevel(double newPeakLevel)
{
    dynamic_cast<DahdsrEnvelopeState_Attack*>(states[DahdsrEnvelopeStateIndex::attack])->setEndLevel(newPeakLevel);
    dynamic_cast<DahdsrEnvelopeState_Hold*>(states[DahdsrEnvelopeStateIndex::hold])->setLevel(newPeakLevel);
    dynamic_cast<DahdsrEnvelopeState_Decay*>(states[DahdsrEnvelopeStateIndex::decay])->setStartingLevel(newPeakLevel);
}
double DahdsrEnvelope::getPeakLevel()
{
    return dynamic_cast<DahdsrEnvelopeState_Hold*>(states[DahdsrEnvelopeStateIndex::hold])->getLevel();
}

void DahdsrEnvelope::setHoldTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Hold*>(states[DahdsrEnvelopeStateIndex::hold])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getHoldTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Hold*>(states[DahdsrEnvelopeStateIndex::hold])->getTimeInSeconds();
}

void DahdsrEnvelope::setDecayTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Decay*>(states[DahdsrEnvelopeStateIndex::decay])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getDecayTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Decay*>(states[DahdsrEnvelopeStateIndex::decay])->getTimeInSeconds();
}

void DahdsrEnvelope::setSustainLevel(double newSustainLevel)
{
    dynamic_cast<DahdsrEnvelopeState_Decay*>(states[DahdsrEnvelopeStateIndex::decay])->setEndLevel(newSustainLevel);
    dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[DahdsrEnvelopeStateIndex::sustain])->setLevel(newSustainLevel);
    //dynamic_cast<DahdsrEnvelopeState_Release*>(states[release])->setStartingLevel(newSustainLevel); //doesn't matter thaaaat much if it isn't set here since this value is overwritten when any playing state transitions to release
}
double DahdsrEnvelope::getSustainLevel()
{
    return dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[DahdsrEnvelopeStateIndex::sustain])->getLevel();
}

void DahdsrEnvelope::setReleaseTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Release*>(states[DahdsrEnvelopeStateIndex::release])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getReleaseTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Release*>(states[DahdsrEnvelopeStateIndex::release])->getTimeInSeconds();
}
