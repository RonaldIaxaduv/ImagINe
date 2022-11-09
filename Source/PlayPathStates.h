/*
  ==============================================================================

    PlayPathStates.h
    Created: 7 Nov 2022 9:05:52am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PlayPath.h"
#include "PlayPathStateIndex.h"


class PlayPathState
{
public:
    PlayPathState(PlayPath& path);

    virtual bool hitTest(int x, int y) = 0;

    virtual void clicked(const juce::ModifierKeys& modifiers) = 0;
    virtual void buttonStateChanged() = 0;

protected:
    PlayPathStateIndex implementedStateIndex = PlayPathStateIndex::StateIndexCount;
    PlayPath& path;
};




class PlayPathState_NotInteractable final : public PlayPathState
{
public:
    PlayPathState_NotInteractable(PlayPath& path);

    bool hitTest(int x, int y) override;

    void clicked(const juce::ModifierKeys& modifiers) override;
    void buttonStateChanged() override;
};




class PlayPathState_Editable final : public PlayPathState
{
public:
    PlayPathState_Editable(PlayPath& path);

    bool hitTest(int x, int y) override;

    void clicked(const juce::ModifierKeys& modifiers) override;
    void buttonStateChanged() override;
};




class PlayPathState_Playable final : public PlayPathState
{
public:
    PlayPathState_Playable(PlayPath& path);

    bool hitTest(int x, int y) override;

    void clicked(const juce::ModifierKeys& modifiers) override;
    void buttonStateChanged() override;
};
