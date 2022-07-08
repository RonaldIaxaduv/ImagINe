/*
  ==============================================================================

    DahdsrEnvelopeStateIndex.h
    Created: 7 Jul 2022 9:50:32pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum DahdsrEnvelopeStateIndex : int //not an enum class since using it in for-loops and as array indices is the main use case
{
    unprepared = 0,
    idle,
    delay,
    attack,
    hold,
    decay,
    sustain,
    release,
    StateIndexCount
};
