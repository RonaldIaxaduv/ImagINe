/*
  ==============================================================================

    SegmentedImageState.h
    Created: 8 Jul 2022 11:03:23am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum struct SegmentableImageState : int
{
    Null, //components have not been initialised yet
    Init, //components have been initialised, but no image has been assigned yet. cannot draw regions in this state.
    
    Draw_Regions, //image has been assigned. regions can be drawn.
    Edit_Regions, //regions can no longer be drawn. clicking on them will open/update the region's editor
    Play_Regions, //regions can no longer be drawn or their editors opened. instead, they can be played
    
    Draw_PlayPath, //image has been assigned. play path can be drawn.
    Edit_PlayPath, //play paths can no longer be drawn. clicking on them will open/update the play path's editor
    Play_PlayPath //play paths can no longer be drawn or their editors opened. instead, they can be played
};