/*
  ==============================================================================

    PlayPathStateIndex.h
    Created: 7 Nov 2022 9:06:13am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum class PlayPathStateIndex : int
{
    notInteractable = 0, //path cannot be clicked and doesn't react to hovering
    editable, //clicking the path opens its PlayPathEditorWindow
    playable, //path (automatically?) sends out a courier to run alongside it and play regions
    StateIndexCount
};