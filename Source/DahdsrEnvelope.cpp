/*
  ==============================================================================

    DahdsrEnvelope.cpp
    Created: 7 Jul 2022 7:14:56pm
    Author:  Aaron

  ==============================================================================
*/

#include "DahdsrEnvelope.h"

const double DahdsrEnvelope::defaultDelayTimeSeconds = 0.0;
const double DahdsrEnvelope::defaultAttackTimeSeconds = 0.1;
const double DahdsrEnvelope::defaultInitialLevel = 0.0;
const double DahdsrEnvelope::defaultHoldTimeSeconds = 0.0;
const double DahdsrEnvelope::defaultPeakLevel = 1.0;
const double DahdsrEnvelope::defaultDecayTimeSeconds = 0.5;
const double DahdsrEnvelope::defaultSustainLevel = 1.0;
const double DahdsrEnvelope::defaultReleaseTimeSeconds = 0.1;

DahdsrEnvelope::DahdsrEnvelope(double delayTimeSeconds,
    double attackTimeSeconds, double initialLevel,
    double holdTimeSeconds, double peakLevel,
    double decayTimeSeconds, double sustainLevel,
    double releaseTimeSeconds)
{
    //define states
    states[static_cast<int>(DahdsrEnvelopeStateIndex::unprepared)] = static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Unprepared(*this));
    states[static_cast<int>(DahdsrEnvelopeStateIndex::idle)] = static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Idle(*this));
    states[static_cast<int>(DahdsrEnvelopeStateIndex::delay)] = static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Delay(*this));
    states[static_cast<int>(DahdsrEnvelopeStateIndex::attack)] = static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Attack(*this));
    states[static_cast<int>(DahdsrEnvelopeStateIndex::hold)] = static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Hold(*this));
    states[static_cast<int>(DahdsrEnvelopeStateIndex::decay)] = static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Decay(*this));
    states[static_cast<int>(DahdsrEnvelopeStateIndex::sustain)] = static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Sustain(*this));
    states[static_cast<int>(DahdsrEnvelopeStateIndex::release)] = static_cast<DahdsrEnvelopeState*>(new DahdsrEnvelopeState_Release(*this));
    int initialisedStates = 8;
    jassert(initialisedStates == static_cast<int>(DahdsrEnvelopeStateIndex::StateIndexCount));

    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];

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
    DBG("destroying DahdsrEnvelope...");

    //release states
    currentState = nullptr;
    delete states[static_cast<int>(DahdsrEnvelopeStateIndex::unprepared)];
    delete states[static_cast<int>(DahdsrEnvelopeStateIndex::idle)];
    delete states[static_cast<int>(DahdsrEnvelopeStateIndex::delay)];
    delete states[static_cast<int>(DahdsrEnvelopeStateIndex::attack)];
    delete states[static_cast<int>(DahdsrEnvelopeStateIndex::hold)];
    delete states[static_cast<int>(DahdsrEnvelopeStateIndex::decay)];
    delete states[static_cast<int>(DahdsrEnvelopeStateIndex::sustain)];
    delete states[static_cast<int>(DahdsrEnvelopeStateIndex::release)];
    int deletedStates = 8;
    jassert(deletedStates == static_cast<int>(DahdsrEnvelopeStateIndex::StateIndexCount));

    DBG("DahdsrEnvelope destroyed.");
}

void DahdsrEnvelope::setSampleRate(double newSampleRate)
{
    for (int i = 0; i < static_cast<int>(DahdsrEnvelopeStateIndex::StateIndexCount); ++i)
    {
        if (i != static_cast<int>(currentStateIndex))
        {
            states[i]->sampleRateChanged(newSampleRate, false); //don't trigger transitions (back to idle) for transitions other than the current one
        }
    }
    currentState->sampleRateChanged(newSampleRate, true); //for the current state, *do* trigger the transition to idle (if applicable)
}

void DahdsrEnvelope::noteOn()
{
    currentState->noteOn(); //automatically transitions states where applicable
}

void DahdsrEnvelope::noteOff() //triggers release; called in Voice::noteOff
{
    currentState->noteOff(); //automatically transitions states where applicable
}

double DahdsrEnvelope::getNextEnvelopeSample() //gets the envelope value of the current position in the current state; called in Voice::renderNextBlock
{
    return currentState->getNextEnvelopeSample(); //automatically changes states where applicable
}

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
            if (dynamic_cast<DahdsrEnvelopeState_Delay*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::delay)])->getTimeInSeconds() == 0.0)
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
            if (dynamic_cast<DahdsrEnvelopeState_Attack*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::attack)])->getTimeInSeconds() == 0.0)
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
            if (dynamic_cast<DahdsrEnvelopeState_Hold*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::hold)])->getTimeInSeconds() == 0.0)
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
            if (dynamic_cast<DahdsrEnvelopeState_Decay*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::decay)])->getTimeInSeconds() == 0.0)
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
            //auto* sustainState = dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::sustain)]);
            if (dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::sustain)])->getLevel() == 0.0) //if sustain is inaudible, release would also be inaudible -> note has basically already ended -> skip to idle
            {
                stateToTransitionTo = DahdsrEnvelopeStateIndex::idle;
                continue;
            }
            else
            {
                //dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::sustain)])->setStartingLevel(currentEnvelopeLevel); //has no effect
                nonInstantStateFound = true;
            }
            break;

        case DahdsrEnvelopeStateIndex::release:
            //auto* releaseState = dynamic_cast<DahdsrEnvelopeState_Release*>(states[DahdsrEnvelopeStateIndex::release]);
            if (dynamic_cast<DahdsrEnvelopeState_Release*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::release)])->getTimeInSeconds() == 0.0)
            {
                stateToTransitionTo = DahdsrEnvelopeStateIndex::idle;
                continue;
            }
            else
            {
                dynamic_cast<DahdsrEnvelopeState_Release*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::release)])->setStartingLevel(currentEnvelopeLevel); //starting value can be overwritten since any playing state can transition to release at any time
                nonInstantStateFound = true;
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

void DahdsrEnvelope::forceStop()
{
    currentState->forceStop(); //automatically transitions states where applicable
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

inline void DahdsrEnvelope::setDelayTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Delay*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::delay)])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getDelayTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Delay*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::delay)])->getTimeInSeconds();
}

inline void DahdsrEnvelope::setInitialLevel(double newInitialLevel)
{
    dynamic_cast<DahdsrEnvelopeState_Attack*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::attack)])->setStartingLevel(newInitialLevel);
}
double DahdsrEnvelope::getInitialLevel()
{
    return dynamic_cast<DahdsrEnvelopeState_Attack*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::attack)])->getStartingLevel();
}

inline void DahdsrEnvelope::setAttackTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Attack*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::attack)])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getAttackTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Attack*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::attack)])->getTimeInSeconds();
}

inline void DahdsrEnvelope::setPeakLevel(double newPeakLevel)
{
    dynamic_cast<DahdsrEnvelopeState_Attack*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::attack)])->setEndLevel(newPeakLevel);
    dynamic_cast<DahdsrEnvelopeState_Hold*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::hold)])->setLevel(newPeakLevel);
    dynamic_cast<DahdsrEnvelopeState_Decay*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::decay)])->setStartingLevel(newPeakLevel);
}
double DahdsrEnvelope::getPeakLevel()
{
    return dynamic_cast<DahdsrEnvelopeState_Hold*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::hold)])->getLevel();
}

inline void DahdsrEnvelope::setHoldTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Hold*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::hold)])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getHoldTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Hold*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::hold)])->getTimeInSeconds();
}

inline void DahdsrEnvelope::setDecayTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Decay*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::decay)])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getDecayTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Decay*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::decay)])->getTimeInSeconds();
}

inline void DahdsrEnvelope::setSustainLevel(double newSustainLevel)
{
    dynamic_cast<DahdsrEnvelopeState_Decay*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::decay)])->setEndLevel(newSustainLevel);
    dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::sustain)])->setLevel(newSustainLevel);
    //dynamic_cast<DahdsrEnvelopeState_Release*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::release)])->setStartingLevel(newSustainLevel); //doesn't matter thaaaat much if it isn't set here since this value is overwritten when any playing state transitions to release
}
double DahdsrEnvelope::getSustainLevel()
{
    return dynamic_cast<DahdsrEnvelopeState_Sustain*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::sustain)])->getLevel();
}

inline void DahdsrEnvelope::setReleaseTime(double newTimeInSeconds)
{
    dynamic_cast<DahdsrEnvelopeState_Release*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::release)])->setTime(newTimeInSeconds);
}
double DahdsrEnvelope::getReleaseTime()
{
    return dynamic_cast<DahdsrEnvelopeState_Release*>(states[static_cast<int>(DahdsrEnvelopeStateIndex::release)])->getTimeInSeconds();
}




bool DahdsrEnvelope::serialise(juce::XmlElement* xmlParent)
{
    DBG("serialising DAHDSR envelope...");
    bool serialisationSuccessful = true;

    juce::XmlElement* xmlEnvelope = xmlParent->createNewChildElement("DahdsrEnvelope");

    xmlEnvelope->setAttribute("delayTime", getDelayTime());
    xmlEnvelope->setAttribute("initialLevel", getInitialLevel());
    xmlEnvelope->setAttribute("attackTime", getAttackTime());
    xmlEnvelope->setAttribute("peakLevel", getPeakLevel());
    xmlEnvelope->setAttribute("holdTime", getHoldTime());
    xmlEnvelope->setAttribute("decayTime", getDecayTime());
    xmlEnvelope->setAttribute("sustainLevel", getSustainLevel());
    xmlEnvelope->setAttribute("releaseTime", getReleaseTime());

    DBG(juce::String(serialisationSuccessful ? "DAHDSR envelope has been serialised." : "DAHDSR envelope could not be serialised."));
    return serialisationSuccessful;
}
bool DahdsrEnvelope::deserialise(juce::XmlElement* xmlParent)
{
    DBG("deserialising DAHDSR envelope...");
    bool deserialisationSuccessful = true;

    juce::XmlElement* xmlEnvelope = xmlParent->getChildByName("DahdsrEnvelope");

    if (xmlEnvelope != nullptr)
    {
        setDelayTime(xmlEnvelope->getDoubleAttribute("delayTime", defaultDelayTimeSeconds));
        setInitialLevel(xmlEnvelope->getDoubleAttribute("initialLevel", defaultInitialLevel));
        setAttackTime(xmlEnvelope->getDoubleAttribute("attackTime", defaultAttackTimeSeconds));
        setPeakLevel(xmlEnvelope->getDoubleAttribute("peakLevel", defaultPeakLevel));
        setHoldTime(xmlEnvelope->getDoubleAttribute("holdTime", defaultHoldTimeSeconds));
        setDecayTime(xmlEnvelope->getDoubleAttribute("decayTime", defaultDecayTimeSeconds));
        setSustainLevel(xmlEnvelope->getDoubleAttribute("sustainLevel", defaultSustainLevel));
        setReleaseTime(xmlEnvelope->getDoubleAttribute("releaseTime", defaultReleaseTimeSeconds));
    }
    else
    {
        DBG("no DAHDSR data found.");
        deserialisationSuccessful = false;
    }

    DBG(juce::String(deserialisationSuccessful ? "DAHDSR envelope has been deserialised." : "DAHDSR envelope could not be deserialised."));
    return deserialisationSuccessful;
}
