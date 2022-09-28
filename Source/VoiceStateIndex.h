/*
  ==============================================================================

    VoiceStateIndex.h
    Created: 28 Sep 2022 9:24:57am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum class VoiceStateIndex : int
{
    unprepared = 0,
    noWavefile_noLfo, //osc = nullptr || osc->fileBuffer.getNumSamples() == 0, also associatedLfo = nullptr
    noWavefile_Lfo, //osc = nullptr || osc->fileBuffer.getNumSamples() == 0, but associatedLfo != nullptr
    stopped_noLfo, //bufferPosDelta = 0.0, also associatedLfo = nullptr
    stopped_Lfo, //bufferPosDelta = 0.0, but associatedLfo != nullptr
    playable_noLfo, //osc->fileBuffer.getNumSamples() > 0 && bufferPosDelta > 0.0 && associatedLfo = nullptr
    playable_Lfo, //osc->fileBuffer.getNumSamples() > 0 && bufferPosDelta > 0.0 && associatedLfo != nullptr
    StateIndexCount
};
