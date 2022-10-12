/*
  ==============================================================================

    SegmentedRegionStates.h
    Created: 7 Oct 2022 3:27:04pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SegmentedRegionStateIndex.h"
#include "SegmentedRegion.h"
class SegmentedRegion;


class SegmentedRegionState
{
public:
    SegmentedRegionState(SegmentedRegion& region);

    virtual bool hitTest(int x, int y) = 0;

    virtual void clicked(const juce::ModifierKeys& modifiers) = 0;
    virtual void buttonStateChanged() = 0;

protected:
    SegmentedRegionStateIndex implementedState = SegmentedRegionStateIndex::StateIndexCount;
    SegmentedRegion& region;
};




class SegmentedRegionState_NotInteractable final : public SegmentedRegionState
{
public:
    SegmentedRegionState_NotInteractable(SegmentedRegion& region);

    bool hitTest(int x, int y) override;

    void clicked(const juce::ModifierKeys& modifiers) override;
    void buttonStateChanged() override;
};




class SegmentedRegionState_Editable final : public SegmentedRegionState
{
public:
    SegmentedRegionState_Editable(SegmentedRegion& region);

    bool hitTest(int x, int y) override;

    void clicked(const juce::ModifierKeys& modifiers) override;
    void buttonStateChanged() override;
};




class SegmentedRegionState_Playable final : public SegmentedRegionState
{
public:
    SegmentedRegionState_Playable(SegmentedRegion& region);

    bool hitTest(int x, int y) override;

    void clicked(const juce::ModifierKeys& modifiers) override;
    void buttonStateChanged() override;
};
