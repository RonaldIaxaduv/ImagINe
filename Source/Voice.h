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


//==============================================================================
/*
*/
struct Voice : public juce::SynthesiserVoice
{
public:
    Voice();
    Voice(juce::AudioSampleBuffer buffer, int origSampleRate, int regionID);

    ~Voice() override;

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec);

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

    ModulatableMultiplicativeParameter<double>* getPlaybackPositionParameter();
    void setBasePlaybackPosition(double newPlaybackPosition);
    double getBasePlaybackPosition();

    void updateBufferPosDelta();
    void updateBufferPosDelta_NotPlayable();
    void updateBufferPosDelta_Playable();

    void setLfo(RegionLfo* newAssociatedLfo);

    int getID();

    DahdsrEnvelope* getEnvelope();

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

    ModulatableMultiplicativeParameter<double> playbackPositionParameter;

    SamplerOscillator* osc;

    RegionLfo* associatedLfo = nullptr;

    DahdsrEnvelope envelope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Voice)
};