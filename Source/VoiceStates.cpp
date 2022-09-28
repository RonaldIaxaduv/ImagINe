/*
  ==============================================================================

    VoiceStates.cpp
    Created: 28 Sep 2022 9:24:33am
    Author:  Aaron

  ==============================================================================
*/

#include "VoiceStates.h"




VoiceState::VoiceState(Voice& voice) :
    voice(voice)
{ }




#pragma region VoiceState_Unprepared

VoiceState_Unprepared::VoiceState_Unprepared(Voice& voice) :
    VoiceState::VoiceState(voice)
{
    implementedVoiceStateIndex = VoiceStateIndex::unprepared;
}

void VoiceState_Unprepared::prepared(double newSampleRate)
{
    if (newSampleRate > 0.0)
    {
        voice.transitionToState(VoiceStateIndex::noWavefile_noLfo);
    }
}
void VoiceState_Unprepared::wavefileChanged(int newSampleCount)
{
    //not prepared yet -> do nothing
}
void VoiceState_Unprepared::playableChanged(bool playable)
{
    //not prepared yet -> do nothing
}
void VoiceState_Unprepared::associatedLfoChanged(RegionLfo* newAssociatedLfo)
{
    //not prepared yet -> do nothing
}

void VoiceState_Unprepared::renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex)
{
    voice.renderNextBlock_empty(); //no sound, no LFO
}

void VoiceState_Unprepared::updateBufferPosDelta()
{
    voice.updateBufferPosDelta_NotPlayable(); //no sound yet
}

#pragma endregion VoiceState_Unprepared




#pragma region VoiceState_NoWavefile_NoLfo

VoiceState_NoWavefile_NoLfo::VoiceState_NoWavefile_NoLfo(Voice& voice) :
    VoiceState::VoiceState(voice)
{
    implementedVoiceStateIndex = VoiceStateIndex::noWavefile_noLfo;
}

void VoiceState_NoWavefile_NoLfo::prepared(double newSampleRate)
{
    if (newSampleRate == 0.0)
    {
        voice.transitionToState(VoiceStateIndex::unprepared);
    }
}
void VoiceState_NoWavefile_NoLfo::wavefileChanged(int newSampleCount)
{
    if (newSampleCount > 0)
    {
        voice.transitionToState(VoiceStateIndex::stopped_noLfo);
    }
}
void VoiceState_NoWavefile_NoLfo::playableChanged(bool playable)
{
    //has no wavefile yet -> do nothing
}
void VoiceState_NoWavefile_NoLfo::associatedLfoChanged(RegionLfo* newAssociatedLfo)
{
    if (newAssociatedLfo != nullptr)
    {
        voice.transitionToState(VoiceStateIndex::noWavefile_Lfo);
    }
}

void VoiceState_NoWavefile_NoLfo::renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex)
{
    voice.renderNextBlock_empty(); //no sound, no LFO
}

void VoiceState_NoWavefile_NoLfo::updateBufferPosDelta()
{
    voice.updateBufferPosDelta_NotPlayable(); //no sound yet
}

#pragma endregion VoiceState_NoWavefile_NoLfo




#pragma region VoiceState_NoWavefile_Lfo

VoiceState_NoWavefile_Lfo::VoiceState_NoWavefile_Lfo(Voice& voice) :
    VoiceState::VoiceState(voice)
{
    implementedVoiceStateIndex = VoiceStateIndex::noWavefile_Lfo;
}

void VoiceState_NoWavefile_Lfo::prepared(double newSampleRate)
{
    if (newSampleRate == 0.0)
    {
        voice.transitionToState(VoiceStateIndex::unprepared);
    }
}
void VoiceState_NoWavefile_Lfo::wavefileChanged(int newSampleCount)
{
    if (newSampleCount > 0)
    {
        voice.transitionToState(VoiceStateIndex::stopped_Lfo);
    }
}
void VoiceState_NoWavefile_Lfo::playableChanged(bool playable)
{
    //has no wavefile yet -> do nothing
}
void VoiceState_NoWavefile_Lfo::associatedLfoChanged(RegionLfo* newAssociatedLfo)
{
    if (newAssociatedLfo == nullptr)
    {
        voice.transitionToState(VoiceStateIndex::noWavefile_noLfo);
    }
}

void VoiceState_NoWavefile_Lfo::renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex)
{
    voice.renderNextBlock_onlyLfo(); //no sound, only LFO
}

void VoiceState_NoWavefile_Lfo::updateBufferPosDelta()
{
    voice.updateBufferPosDelta_NotPlayable(); //no sound yet
}

#pragma endregion VoiceState_NoWavefile_Lfo




#pragma region VoiceState_Stopped_NoLfo

VoiceState_Stopped_NoLfo::VoiceState_Stopped_NoLfo(Voice& voice) :
    VoiceState::VoiceState(voice)
{
    implementedVoiceStateIndex = VoiceStateIndex::stopped_noLfo;
}

void VoiceState_Stopped_NoLfo::prepared(double newSampleRate)
{
    if (newSampleRate == 0.0)
    {
        voice.transitionToState(VoiceStateIndex::unprepared);
    }
}
void VoiceState_Stopped_NoLfo::wavefileChanged(int newSampleCount)
{
    if (newSampleCount == 0)
    {
        voice.transitionToState(VoiceStateIndex::noWavefile_noLfo);
    }
}
void VoiceState_Stopped_NoLfo::playableChanged(bool playable)
{
    if (playable)
    {
        voice.transitionToState(VoiceStateIndex::playable_noLfo);
    }
}
void VoiceState_Stopped_NoLfo::associatedLfoChanged(RegionLfo* newAssociatedLfo)
{
    if (newAssociatedLfo != nullptr)
    {
        voice.transitionToState(VoiceStateIndex::stopped_Lfo);
    }
}

void VoiceState_Stopped_NoLfo::renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex)
{
    voice.renderNextBlock_empty(); //no sound, no LFO
}

void VoiceState_Stopped_NoLfo::updateBufferPosDelta()
{
    voice.updateBufferPosDelta_NotPlayable(); //not affected (only plays when un-stopped)
}

#pragma endregion VoiceState_Stopped_NoLfo




#pragma region VoiceState_Stopped_Lfo

VoiceState_Stopped_Lfo::VoiceState_Stopped_Lfo(Voice& voice) :
    VoiceState::VoiceState(voice)
{
    implementedVoiceStateIndex = VoiceStateIndex::stopped_Lfo;
}

void VoiceState_Stopped_Lfo::prepared(double newSampleRate)
{
    if (newSampleRate == 0.0)
    {
        voice.transitionToState(VoiceStateIndex::unprepared);
    }
}
void VoiceState_Stopped_Lfo::wavefileChanged(int newSampleCount)
{
    if (newSampleCount == 0)
    {
        voice.transitionToState(VoiceStateIndex::noWavefile_Lfo);
    }
}
void VoiceState_Stopped_Lfo::playableChanged(bool playable)
{
    if (playable)
    {
        voice.transitionToState(VoiceStateIndex::playable_Lfo);
    }
}
void VoiceState_Stopped_Lfo::associatedLfoChanged(RegionLfo* newAssociatedLfo)
{
    if (newAssociatedLfo == nullptr)
    {
        voice.transitionToState(VoiceStateIndex::stopped_noLfo);
    }
}

void VoiceState_Stopped_Lfo::renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex)
{
    voice.renderNextBlock_empty(); //no sound, no LFO (the LFO may be set, but the voice has stopped, so it shouldn't advance!)
}

void VoiceState_Stopped_Lfo::updateBufferPosDelta()
{
    voice.updateBufferPosDelta_NotPlayable(); //not affected (only plays when un-stopped)
}

#pragma endregion VoiceState_Stopped_Lfo




#pragma region VoiceState_Playable_NoLfo

VoiceState_Playable_NoLfo::VoiceState_Playable_NoLfo(Voice& voice) :
    VoiceState::VoiceState(voice)
{
    implementedVoiceStateIndex = VoiceStateIndex::playable_noLfo;
}

void VoiceState_Playable_NoLfo::prepared(double newSampleRate)
{
    if (newSampleRate == 0.0)
    {
        voice.transitionToState(VoiceStateIndex::unprepared);
    }
}
void VoiceState_Playable_NoLfo::wavefileChanged(int newSampleCount)
{
    if (newSampleCount == 0)
    {
        voice.transitionToState(VoiceStateIndex::noWavefile_noLfo);
    }
}
void VoiceState_Playable_NoLfo::playableChanged(bool playable)
{
    if (!playable)
    {
        voice.transitionToState(VoiceStateIndex::stopped_noLfo);
    }
}
void VoiceState_Playable_NoLfo::associatedLfoChanged(RegionLfo* newAssociatedLfo)
{
    if (newAssociatedLfo != nullptr)
    {
        voice.transitionToState(VoiceStateIndex::playable_Lfo);
    }
}

void VoiceState_Playable_NoLfo::renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex)
{
    voice.renderNextBlock_wave(outputBuffer, sampleIndex); //sound, no LFO
}

void VoiceState_Playable_NoLfo::updateBufferPosDelta()
{
    voice.updateBufferPosDelta_Playable(); //execute as usual
}

#pragma endregion VoiceState_Playable_NoLfo




#pragma region VoiceState_Playable_Lfo

VoiceState_Playable_Lfo::VoiceState_Playable_Lfo(Voice& voice) :
    VoiceState::VoiceState(voice)
{
    implementedVoiceStateIndex = VoiceStateIndex::playable_Lfo;
}

void VoiceState_Playable_Lfo::prepared(double newSampleRate)
{
    if (newSampleRate == 0.0)
    {
        voice.transitionToState(VoiceStateIndex::unprepared);
    }
}
void VoiceState_Playable_Lfo::wavefileChanged(int newSampleCount)
{
    if (newSampleCount == 0)
    {
        voice.transitionToState(VoiceStateIndex::noWavefile_Lfo);
    }
}
void VoiceState_Playable_Lfo::playableChanged(bool playable)
{
    if (!playable)
    {
        voice.transitionToState(VoiceStateIndex::stopped_Lfo);
    }
}
void VoiceState_Playable_Lfo::associatedLfoChanged(RegionLfo* newAssociatedLfo)
{
    if (newAssociatedLfo == nullptr)
    {
        voice.transitionToState(VoiceStateIndex::playable_noLfo);
    }
}

void VoiceState_Playable_Lfo::renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int sampleIndex)
{
    voice.renderNextBlock_waveAndLfo(outputBuffer, sampleIndex); //sound + LFO
}

void VoiceState_Playable_Lfo::updateBufferPosDelta()
{
    voice.updateBufferPosDelta_Playable(); //execute as usual
}

#pragma endregion VoiceState_Playable_Lfo
