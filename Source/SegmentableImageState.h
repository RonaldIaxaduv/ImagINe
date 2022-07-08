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
    Drawing, //image has been assigned. regions can be drawn.
    Editing, //regions can no longer be drawn. clicking on them will update the editor in the MainComponent
    Playing //regions can no longer be drawn or edited. instead, they can be played
};