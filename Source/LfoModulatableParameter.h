/*
  ==============================================================================

    LfoModulatableParameter.h
    Created: 8 Jul 2022 10:50:01am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum class LfoModulatableParameter : int
{
    none = 0,

    volume = 1,
    volume_inverted,
    pitch,
    pitch_inverted,
    //panning,
    //panning_inverted,
    playbackPosition,
    playbackPosition_inverted,

    lfoRate,
    lfoRate_inverted,
    //lfoDepth,
    //lfoDepth_inverted,
    lfoPhase,
    lfoPhase_inverted,
    lfoUpdateInterval,
    lfoUpdateInterval_inverted
};