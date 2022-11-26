/*
  ==============================================================================

    Voice.h
    Created: 9 Jun 2022 9:58:36am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VoiceStates.h"
class VoiceState;
class VoiceState_Unprepared;
class VoiceState_NoWavefile_NoLfo;
class VoiceState_NoWavefile_Lfo;
class VoiceState_Stopped_NoLfo;
class VoiceState_Stopped_Lfo;
class VoiceState_Playable_NoLfo;
class VoiceState_Playable_Lfo;
#include "VoiceStateIndex.h"

#include "SamplerOscillator.h"
#include "DahdsrEnvelope.h"
#include "ModulatableParameter.h"
#include "RegionLfo.h"
#include "PitchQuantisationMethod.h"


//==============================================================================
/*
*/
struct Voice : public juce::SynthesiserVoice
{
public:
    Voice();
    Voice(int regionID);
    Voice(juce::AudioSampleBuffer buffer, int origSampleRate, int regionID);

    ~Voice() override;

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec);

    void setOsc(juce::AudioSampleBuffer buffer, int origSampleRate);

    bool canPlaySound(juce::SynthesiserSound* sound) override;

    //==============================================================================
    /*void noteStarted() override;*/
    void startNote(int midiNoteNumber, float velocity,
        juce::SynthesiserSound* sound, int /*currentPitchWheelPosition*/) override;

    void stopNote(float /*velocity*/, bool allowTailOff) override;

    //==============================================================================
    /*void notePressureChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;*/

    void pitchWheelMoved(int) override;
    void controllerMoved(int, int) override;

    //==============================================================================
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override;

    void renderNextBlock_empty();
    void renderNextBlock_onlyLfo();
    void renderNextBlock_wave(juce::AudioSampleBuffer& outputBuffer, int sampleIndex);
    void renderNextBlock_waveAndLfo(juce::AudioSampleBuffer& outputBuffer, int sampleIndex);

    //==============================================================================
    void transitionToState(VoiceStateIndex stateToTransitionTo);
    bool isPlaying();

    //==============================================================================
    ModulatableMultiplicativeParameter<double>* getLevelParameter();
    void setBaseLevel(double newLevel);
    double getBaseLevel();

    ModulatableAdditiveParameter<double>* getPitchShiftParameter();
    void setBasePitchShift(double newPitchShift);
    double getBasePitchShift();

    ModulatableAdditiveParameter<double>* getPlaybackPositionStartParameter();
    void setBasePlaybackPositionStart(double newPlaybackPositionStart);
    double getBasePlaybackPositionStart();

    ModulatableMultiplicativeParameterLowerCap<double>* getPlaybackPositionIntervalParameter();
    void setBasePlaybackPositionInterval(double newPlaybackPositionInterval);
    double getBasePlaybackPositionInterval();

    ModulatableAdditiveParameter<double>* getPlaybackPositionCurrentParameter();
    void setBasePlaybackPositionCurrent(double newPlaybackPositionCurrent);
    double getBasePlaybackPositionCurrent();

    void updateBufferPosDelta();
    void updateBufferPosDelta_NotPlayable();
    void updateBufferPosDelta_Playable();

    void evaluateBufferPosModulation();

    void setPitchQuantisationMethod(PitchQuantisationMethod newPitchQuantisationMethod);
    PitchQuantisationMethod getPitchQuantisationMethod();

    double getQuantisedPitch_continuous();
    double getQuantisedPitch_semitones();
    double getQuantisedPitch_scale();

    void setPitchQuantisationScale_major();
    void setPitchQuantisationScale_minor();
    void setPitchQuantisationScale_semitonesReversed();
    void setPitchQuantisationScale_majorReversed();
    void setPitchQuantisationScale_minorReversed();
    void setPitchQuantisationScale_hijazKar();
    void setPitchQuantisationScale_hungarianMinor();
    void setPitchQuantisationScale_persian();
    void setPitchQuantisationScale_bluesHexa();
    void setPitchQuantisationScale_bluesHepta();
    void setPitchQuantisationScale_bluesNona();
    void setPitchQuantisationScale_ritsu();
    void setPitchQuantisationScale_ryo();
    void setPitchQuantisationScale_minyo();
    void setPitchQuantisationScale_miyakoBushi();
    void setPitchQuantisationScale_wholeTone();
    void setPitchQuantisationScale_pentatonicMajor();
    void setPitchQuantisationScale_pentatonicMinor();

    void setPitchQuantisationScale_minorSecondDown();
    void setPitchQuantisationScale_minorSecondUp();
    void setPitchQuantisationScale_majorSecondDown();
    void setPitchQuantisationScale_majorSecondUp();
    void setPitchQuantisationScale_minorThirdDown();
    void setPitchQuantisationScale_minorThirdUp();
    void setPitchQuantisationScale_majorThirdDown();
    void setPitchQuantisationScale_majorThirdUp();
    void setPitchQuantisationScale_perfectFourthDown();
    void setPitchQuantisationScale_perfectFourthUp();
    void setPitchQuantisationScale_diminishedFifthDown();
    void setPitchQuantisationScale_diminishedFifthUp();
    void setPitchQuantisationScale_perfectFifthDown();
    void setPitchQuantisationScale_perfectFifthUp();
    void setPitchQuantisationScale_minorSixthDown();
    void setPitchQuantisationScale_minorSixthUp();
    void setPitchQuantisationScale_majorSixthDown();
    void setPitchQuantisationScale_majorSixthUp();
    void setPitchQuantisationScale_minorSeventhDown();
    void setPitchQuantisationScale_minorSeventhUp();
    void setPitchQuantisationScale_majorSeventhDown();
    void setPitchQuantisationScale_majorSeventhUp();
    void setPitchQuantisationScale_perfectOctave();

    void setLfo(RegionLfo* newAssociatedLfo);

    int getID();
    void setID(int newID);

    DahdsrEnvelope* getEnvelope();

    bool serialise(juce::XmlElement* xmlVoice);
    bool deserialise(juce::XmlElement* xmlVoice);

private:
    //states
    VoiceState* states[static_cast<int>(VoiceStateIndex::StateIndexCount)]; //fixed size -> more efficient access
    
    static const VoiceStateIndex initialStateIndex = VoiceStateIndex::unprepared;
    VoiceStateIndex currentStateIndex;
    VoiceState* currentState = nullptr; //provides at least one less array lookup during each access (i.e. every sample)

    //other
    int ID = -1;

    double currentBufferPos = 0.0, bufferPosDelta = 0.0;

    ModulatableMultiplicativeParameter<double> levelParameter;

    ModulatableAdditiveParameter<double> pitchShiftParameter;
    juce::dsp::LookupTableTransform<double> playbackMultApprox; //calculating the resulting playback speed from the semitones of pitch shift needs the power function (2^(semis/12)) which is expensive. this pre-calculates values within a certain range (-60...+60)

    PitchQuantisationMethod currentPitchQuantisationMethod = PitchQuantisationMethod::continuous;
    double (Voice::* pitchQuantisationFuncPt)() = nullptr;
    int pitchQuantisationScale[12]; //for each semitone in an octave (-> input index), maps to a note on a scale

    ModulatableAdditiveParameter<double> playbackPositionStartParameter;
    ModulatableMultiplicativeParameterLowerCap<double> playbackPositionIntervalParameter;
    ModulatableAdditiveParameter<double> playbackPositionCurrentParameter;

    SamplerOscillator* osc;

    RegionLfo* associatedLfo = nullptr;

    DahdsrEnvelope envelope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Voice)
};