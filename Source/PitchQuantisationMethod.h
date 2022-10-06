/*
  ==============================================================================

    PitchQuantisationMethod.h
    Created: 6 Oct 2022 1:22:50pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum class PitchQuantisationMethod : int
{
    continuous = 0,
    semitones,
    scale_major,
    scale_minor,
    scale_octaves,
    MethodCount
};