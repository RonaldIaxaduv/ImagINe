/*
  ==============================================================================

    Voice.h
    Created: 9 Jun 2022 9:58:36am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SamplerOscillator.h"
#include "DahdsrEnvelope.h"
#include "ModulatableParameter.h"

#include "RegionLfo.h" //uses pointer + needs access to methods -> full build required -> no forwarding(?)
//class RegionLfo;


//==============================================================================
/*
*/
struct Voice : public juce::SynthesiserVoice
{
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

    ModulatableMultiplicativeParameter<double>* getLevelParameter();
    void setBaseLevel(double newLevel);
    double getBaseLevel();
    //void modulateLevel(double newMultiplier);

    ModulatableAdditiveParameter<double>* getPitchShiftParameter();
    void setBasePitchShift(double newPitchShift);
    double getBasePitchShift();
    //void modulatePitchShift(double semitonesToAdd);
    void updateBufferPosDelta();

    //void setLfoAdvancer(std::function<void()> newAdvanceLfoFunction);
    void setLfo(RegionLfo* newAssociatedLfo);

    int getID();

    DahdsrEnvelope* getEnvelope();

private:
    int ID = -1;

    double currentBufferPos = 0.0, bufferPosDelta = 0.0;

    //double level = 0.25;
    //double totalLevelMultiplier = 1.0;
    ModulatableMultiplicativeParameter<double> levelParameter;

    //double pitchShiftBase = 0.0; //in semitones difference to the normal bufferPosDelta
    //double totalPitchShiftModulation = 0.0; //in semitones (can also be negative)
    ModulatableAdditiveParameter<double> pitchShiftParameter;
    juce::dsp::LookupTableTransform<double> playbackMultApprox; //calculating the resulting playback speed from the semitones of pitch shift needs the power function (2^(semis/12)) which is expensive. this pre-calculates values within a certain range (-60...+60)

    SamplerOscillator* osc;

    RegionLfo* associatedLfo = nullptr;
    //int associatedLfoIndex = -1; //must be handled like this and NOT via a RegionLfo pointer, because otherwise the RegionLfo class and Voice class would cross-reference each other -> not compilable
    //std::function<void()> advanceLfoFunction; //nvm, referencing AudioEngine would cause the same problem. guess it's gotta be a function then. noooot messy at all xD

    DahdsrEnvelope envelope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Voice)
};