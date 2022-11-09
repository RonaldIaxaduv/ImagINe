/*
  ==============================================================================

    PlayPathEditor.h
    Created: 7 Nov 2022 9:06:47am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include "PlayPath.h"
class PlayPath;

class PlayPathEditor final : public juce::Component
{
public:
    PlayPathEditor(PlayPath* path);
    ~PlayPathEditor();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    PlayPath* associatedPath;

    void copyPathParameters();

    void updateCourierInterval();

    juce::Label courierIntervalLabel;
    juce::Slider courierIntervalSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayPathEditor)
};