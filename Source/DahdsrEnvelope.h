/*
  ==============================================================================

    DahdsrEnvelope.h
    Created: 6 Jul 2022 9:46:48am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DahdsrEnvelopeStateIndex.h"

//forward references to the envelope states (required to enable circular dependencies - see https://stackoverflow.com/questions/994253/two-classes-that-refer-to-each-other )
#include "DahdsrEnvelopeStates.h"
class DahdsrEnvelopeState;
class DahdsrEnvelopeState_Unprepared;
class DahdsrEnvelopeState_Idle;
class DahdsrEnvelopeState_Delay;
class DahdsrEnvelopeState_Attack;
class DahdsrEnvelopeState_Hold;
class DahdsrEnvelopeState_Decay;
class DahdsrEnvelopeState_Sustain;
class DahdsrEnvelopeState_Release;




/// <summary>
/// Implements a DAHDSR (delay, attack, hold, decay, sustain, release) envelope curve
/// </summary>
class DahdsrEnvelope
{
public:
    DahdsrEnvelope(double delayTimeSeconds = defaultDelayTimeSeconds,
        double attackTimeSeconds = defaultAttackTimeSeconds, double initialLevel = defaultInitialLevel,
        double holdTimeSeconds = defaultHoldTimeSeconds, double peakLevel = defaultPeakLevel,
        double decayTimeSeconds = defaultDecayTimeSeconds, double sustainLevel = defaultSustainLevel,
        double releaseTimeSeconds = defaultReleaseTimeSeconds);

    ~DahdsrEnvelope();

    void setSampleRate(double newSampleRate);

    void noteOn(bool includeDelay = true, bool ignoreDuringRelease = false);

    void noteOff();

    double getNextEnvelopeSample();

    void transitionToState(DahdsrEnvelopeStateIndex stateToTransitionTo, double currentEnvelopeLevel);

    void forceStop();

    bool isReleasing();

    bool isIdle();

    //================================================================

    void setDelayTime(double newTimeInSeconds);
    double getDelayTime();

    void setInitialLevel(double newInitialLevel);
    double getInitialLevel();

    void setAttackTime(double newTimeInSeconds);
    double getAttackTime();

    void setPeakLevel(double newPeakLevel);
    double getPeakLevel();

    void setHoldTime(double newTimeInSeconds);
    double getHoldTime();

    void setDecayTime(double newTimeInSeconds);
    double getDecayTime();

    void setSustainLevel(double newSustainLevel);
    double getSustainLevel();

    void setReleaseTime(double newTimeInSeconds);
    double getReleaseTime();

    bool serialise(juce::XmlElement* xmlParent);
    bool deserialise(juce::XmlElement* xmlParent);

private:
    DahdsrEnvelopeState* states[static_cast<int>(DahdsrEnvelopeStateIndex::StateIndexCount)]; //fixed size -> more efficient access

    static const DahdsrEnvelopeStateIndex initialStateIndex = DahdsrEnvelopeStateIndex::unprepared;
    DahdsrEnvelopeStateIndex currentStateIndex;
    DahdsrEnvelopeState* currentState = nullptr; //provides one less array lookup during each access (i.e. every sample)

    static const double defaultDelayTimeSeconds;
    static const double defaultAttackTimeSeconds;
    static const double defaultInitialLevel;
    static const double defaultHoldTimeSeconds;
    static const double defaultPeakLevel;
    static const double defaultDecayTimeSeconds;
    static const double defaultSustainLevel;
    static const double defaultReleaseTimeSeconds;
};