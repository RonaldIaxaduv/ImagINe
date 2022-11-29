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
    playbackPositionInterval,
    playbackPositionInterval_inverted,

    lfoRate,
    lfoRate_inverted,
    //lfoDepth,
    //lfoDepth_inverted,
    lfoPhaseInterval,
    lfoPhaseInterval_inverted,
    lfoUpdateInterval,
    lfoUpdateInterval_inverted,



    //after here, the order of the parameters will be a little more chaotic.
    //this is because, to ensure backwards compatibility with older save states, the numbering of the previous states must remain the same!
    //though it would also be possible to keep a cleaner ordering by manually assigning a number to each state, doing it that way it more prone to errors.
    //hence, keeping a cleaner order will be restricted to the combo boxes shown in the program.
    playbackPositionStart,
    playbackPositionStart_inverted,
    playbackPositionCurrent,
    playbackPositionCurrent_inverted,

    lfoStartingPhase,
    lfoStartingPhase_inverted,
    lfoCurrentPhase,
    lfoCurrentPhase_inverted,

    filterPosition,
    filterPosition_inverted,

    ModulatableParameterCount
};