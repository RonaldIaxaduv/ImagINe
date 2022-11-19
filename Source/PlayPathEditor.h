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

    bool keyPressed(const juce::KeyPress& key) override;

private:
    PlayPath* associatedPath;

    juce::Label courierIntervalLabel;
    juce::Slider courierIntervalSlider;

    juce::TextButton randomiseButton;


    void copyPathParameters();

    void updateCourierInterval();
    void randomiseCourierInterval();

    void randomiseAllParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayPathEditor)
};