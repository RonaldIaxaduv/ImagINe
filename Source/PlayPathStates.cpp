/*
  ==============================================================================

    PlayPathStates.cpp
    Created: 7 Nov 2022 9:05:52am
    Author:  Aaron

  ==============================================================================
*/

#include "PlayPathStates.h"


PlayPathState::PlayPathState(PlayPath& path) :
    path(path)
{ }




#pragma region PlayPathState_NotInteractable

PlayPathState_NotInteractable::PlayPathState_NotInteractable(PlayPath& path) :
    PlayPathState::PlayPathState(path)
{
    implementedStateIndex = PlayPathStateIndex::notInteractable;
}

bool PlayPathState_NotInteractable::hitTest(int x, int y)
{
    //return false; //no interaction
    return path.hitTest_Interactable(x, y); //this is fine (and beneficial to redrawing) as long as clicked and buttonStateChanged remain empty or the path is disabled
}

void PlayPathState_NotInteractable::clicked(const juce::ModifierKeys& modifiers)
{
    //no interaction
}
void PlayPathState_NotInteractable::buttonStateChanged()
{
    path.triggerDrawableButtonStateChanged(); //this is fine because to disable clicking, the path can just be disabled.
}

#pragma endregion PlayPathState_NotInteractable




#pragma region PlayPathState_Editable

PlayPathState_Editable::PlayPathState_Editable(PlayPath& path) :
    PlayPathState::PlayPathState(path)
{
    implementedStateIndex = PlayPathStateIndex::editable;
}

bool PlayPathState_Editable::hitTest(int x, int y)
{
    return path.hitTest_Interactable(x, y); //normal interaction
}

void PlayPathState_Editable::clicked(const juce::ModifierKeys& modifiers)
{
    path.triggerButtonStateChanged();

    if (!path.isEditorOpen())
    {
        path.openEditor();
    }
    else
    {
        path.sendEditorToFront();
    }
}
void PlayPathState_Editable::buttonStateChanged()
{
    path.triggerDrawableButtonStateChanged();
}

#pragma endregion PlayPathState_Editable




#pragma region PlayPathState_Playable

PlayPathState_Playable::PlayPathState_Playable(PlayPath& path) :
    PlayPathState::PlayPathState(path)
{
    implementedStateIndex = PlayPathStateIndex::playable;
}

bool PlayPathState_Playable::hitTest(int x, int y)
{
    return path.hitTest_Interactable(x, y); //normal interaction
}

void PlayPathState_Playable::clicked(const juce::ModifierKeys& modifiers)
{
    path.triggerButtonStateChanged();
}
void PlayPathState_Playable::buttonStateChanged()
{
    path.triggerDrawableButtonStateChanged();

    switch (path.getState())
    {
    case juce::Button::ButtonState::buttonDown:
        //toggle path off or on
        if (path.getToggleState() == true)
        {
            path.stopPlaying();
        }
        else
        {
            path.startPlaying();
        }
        break;

    default:
        break;
    }
}

#pragma endregion PlayPathState_Playable
