/*
  ==============================================================================

    VoiceStates.h
    Created: 28 Sep 2022 9:24:33am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VoiceStateIndex.h"
#include "Voice.h"
struct Voice;
#include "RegionLfo.h"

class VoiceState
{
public:
    VoiceState(Voice& voice);

    virtual void prepared(double newSampleRate) = 0;
    virtual void wavefileChanged(int newSampleCount) = 0;
    virtual void playableChanged(bool playable) = 0; //true if bufferPosDelta > 0.0, otherwise false
    virtual void associatedLfoChanged(RegionLfo* newAssociatedLfo) = 0;

    virtual void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex) = 0; //chooses one of the simplified (normally unsafe) renderNextBlock methods in voice that should be called. saves several if cases each sample

    virtual void updateBufferPosDelta() = 0; //chooses whether bufferPosDelta is set to 0.0 (when not playable) or calculated normally. saves 1 if case per sample

protected:
    VoiceStateIndex implementedVoiceStateIndex = VoiceStateIndex::StateIndexCount;
    Voice& voice;
};

class VoiceState_Unprepared final : public VoiceState
{
public:
    VoiceState_Unprepared(Voice& voice);

    void prepared(double newSampleRate) override;
    void wavefileChanged(int newSampleCount) override;
    void playableChanged(bool playable) override; //true if bufferPosDelta > 0.0, otherwise false
    void associatedLfoChanged(RegionLfo* newAssociatedLfo) override;

    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex) override; //chooses one of the simplified (normally unsafe) renderNextBlock methods in voice that should be called

    void updateBufferPosDelta() override;
};

class VoiceState_NoWavefile_NoLfo final : public VoiceState
{
public:
    VoiceState_NoWavefile_NoLfo(Voice& voice);

    void prepared(double newSampleRate) override;
    void wavefileChanged(int newSampleCount) override;
    void playableChanged(bool playable) override; //true if bufferPosDelta > 0.0, otherwise false
    void associatedLfoChanged(RegionLfo* newAssociatedLfo) override;

    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex) override; //chooses one of the simplified (normally unsafe) renderNextBlock methods in voice that should be called

    void updateBufferPosDelta() override;
};

class VoiceState_NoWavefile_Lfo final : public VoiceState
{
public:
    VoiceState_NoWavefile_Lfo(Voice& voice);

    void prepared(double newSampleRate) override;
    void wavefileChanged(int newSampleCount) override;
    void playableChanged(bool playable) override; //true if bufferPosDelta > 0.0, otherwise false
    void associatedLfoChanged(RegionLfo* newAssociatedLfo) override;

    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex) override; //chooses one of the simplified (normally unsafe) renderNextBlock methods in voice that should be called

    void updateBufferPosDelta() override;
};

class VoiceState_Stopped_NoLfo final : public VoiceState
{
public:
    VoiceState_Stopped_NoLfo(Voice& voice);

    void prepared(double newSampleRate) override;
    void wavefileChanged(int newSampleCount) override;
    void playableChanged(bool playable) override; //true if bufferPosDelta > 0.0, otherwise false
    void associatedLfoChanged(RegionLfo* newAssociatedLfo) override;

    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex) override; //chooses one of the simplified (normally unsafe) renderNextBlock methods in voice that should be called

    void updateBufferPosDelta() override;
};

class VoiceState_Stopped_Lfo final : public VoiceState
{
public:
    VoiceState_Stopped_Lfo(Voice& voice);

    void prepared(double newSampleRate) override;
    void wavefileChanged(int newSampleCount) override;
    void playableChanged(bool playable) override; //true if bufferPosDelta > 0.0, otherwise false
    void associatedLfoChanged(RegionLfo* newAssociatedLfo) override;

    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex) override; //chooses one of the simplified (normally unsafe) renderNextBlock methods in voice that should be called

    void updateBufferPosDelta() override;
};

class VoiceState_Playable_NoLfo final : public VoiceState
{
public:
    VoiceState_Playable_NoLfo(Voice& voice);

    void prepared(double newSampleRate) override;
    void wavefileChanged(int newSampleCount) override;
    void playableChanged(bool playable) override; //true if bufferPosDelta > 0.0, otherwise false
    void associatedLfoChanged(RegionLfo* newAssociatedLfo) override;

    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex) override; //chooses one of the simplified (normally unsafe) renderNextBlock methods in voice that should be called

    void updateBufferPosDelta() override;
};

class VoiceState_Playable_Lfo final : public VoiceState
{
public:
    VoiceState_Playable_Lfo(Voice& voice);

    void prepared(double newSampleRate) override;
    void wavefileChanged(int newSampleCount) override;
    void playableChanged(bool playable) override; //true if bufferPosDelta > 0.0, otherwise false
    void associatedLfoChanged(RegionLfo* newAssociatedLfo) override;

    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex) override; //chooses one of the simplified (normally unsafe) renderNextBlock methods in voice that should be called

    void updateBufferPosDelta() override;
};
