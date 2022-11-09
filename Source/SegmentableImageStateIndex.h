/*
  ==============================================================================

    SegmentableImageStateIndex.h
    Created: 7 Oct 2022 3:28:00pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum struct SegmentableImageStateIndex : int
{
    empty = 0, //no image loaded yet
    withImage, //image loaded, not drawing any regions
    drawingRegion, //currently drawing a region
    editingRegions, //currently editing regions
    playingRegions, //currently playing regions
    drawingPlayPath, //currently drawing a play path
    editingPlayPaths, //currently editing play paths
    playingPlayPaths, //currently playing play paths
    StateIndexCount
};