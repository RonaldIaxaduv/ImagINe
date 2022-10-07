/*
  ==============================================================================

    SegmentedRegionStateIndex.h
    Created: 7 Oct 2022 3:26:53pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum struct SegmentedRegionStateIndex : int
{
    notInteractable = 0, //shape cannot be clicked yet and doesn't react to hovering
    editable, //clicking the region opens its RegionEditorWindow. toggling always disabled
    playable, //clicking the region plays its Voice(s) and LFO. if there are no voices, only the LFO will play. toggling can be enabled independently.
    StateIndexCount
};
