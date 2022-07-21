/*
  ==============================================================================

    RegionLfoStateIndex.h
    Created: 14 Jul 2022 8:37:13am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum struct RegionLfoStateIndex : int
{
    unprepared,
    withoutWaveTable,
    withoutModulatedParameters,
    muted, //modulation depth is zero
    active,
    StateIndexCount
};