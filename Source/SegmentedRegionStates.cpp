/*
  ==============================================================================

    SegmentedRegionStates.cpp
    Created: 7 Oct 2022 3:27:04pm
    Author:  Aaron

  ==============================================================================
*/

#include "SegmentedRegionStates.h"


SegmentedRegionState::SegmentedRegionState(SegmentedRegion& region) :
    region(region)
{ }




SegmentedRegionState_NotInteractable::SegmentedRegionState_NotInteractable(SegmentedRegion& region) :
    SegmentedRegionState::SegmentedRegionState(region)
{
    implementedState = SegmentedRegionStateIndex::notInteractable;
}

bool SegmentedRegionState_NotInteractable::hitTest(int x, int y)
{
    return false; //no interaction
}

void SegmentedRegionState_NotInteractable::clicked(const juce::ModifierKeys& modifiers)
{
    //no interaction
}
void SegmentedRegionState_NotInteractable::buttonStateChanged()
{
    //no interaction
}




SegmentedRegionState_Editable::SegmentedRegionState_Editable(SegmentedRegion& region) :
    SegmentedRegionState::SegmentedRegionState(region)
{
    implementedState = SegmentedRegionStateIndex::editable;
}

bool SegmentedRegionState_Editable::hitTest(int x, int y)
{
    return region.hitTest(x, y); //normal interaction
}

void SegmentedRegionState_Editable::clicked(const juce::ModifierKeys& modifiers)
{
    region.triggerButtonStateChanged();

    if (region.isEditorOpen())
    {
        region.sendEditorToFront();
    }
    else
    {
        region.openEditor();
    }
}
void SegmentedRegionState_Editable::buttonStateChanged()
{
    region.triggerDrawableButtonStateChanged();
}




SegmentedRegionState_Playable::SegmentedRegionState_Playable(SegmentedRegion& region) :
    SegmentedRegionState::SegmentedRegionState(region)
{
    implementedState = SegmentedRegionStateIndex::playable;
}

bool SegmentedRegionState_Playable::hitTest(int x, int y)
{
    return region.hitTest(x, y); //normal interaction
}

void SegmentedRegionState_Playable::clicked(const juce::ModifierKeys& modifiers)
{
    region.triggerButtonStateChanged(); //normal clicking (enables changing the button's toggle state)
}
void SegmentedRegionState_Playable::buttonStateChanged()
{
    region.triggerDrawableButtonStateChanged();

    switch (region.getState())
    {
    case juce::Button::ButtonState::buttonDown:
        //if toggleable, toggle music off or on
        if (region.isToggleable() && region.getToggleState() == true)
            region.stopPlaying();
        else //not in toggle mode or toggling on
            region.startPlaying();
        break;

    default:
        if (!region.isToggleable())
            region.stopPlaying();
        break;
    }
}