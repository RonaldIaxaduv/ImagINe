/*
  ==============================================================================

    SegmentedRegionState.h
    Created: 8 Jul 2022 11:03:06am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum struct SegmentedRegionState : int
{
    Null,
    Init, //all components have been initialised
    Standby, //shape cannot be clicked yet
    Editing, //region is ready to be edited
    Playable
};
