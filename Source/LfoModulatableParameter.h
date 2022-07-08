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
    pitch,
    //Panning,

    lfoRate,
    //lfoDepth

    playbackPosition
};