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

class PlayPathEditor final : public juce::Component, juce::ChangeListener
{
public:
    PlayPathEditor(PlayPath* path);
    ~PlayPathEditor();

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;

    void changeListenerCallback(juce:: ChangeBroadcaster* source) override;

private:
    PlayPath* associatedPath;

    juce::Label courierIntervalLabel;
    juce::Slider courierIntervalSlider;

    juce::Label upColourLabel;
    juce::ColourSelector upColourSelector;

    juce::Label downColourLabel;
    juce::ColourSelector downColourSelector;

    juce::Label midiChannelLabel;
    juce::ComboBox midiChannelChoice;
    juce::Label midiNoteLabel;
    juce::ComboBox midiNoteChoice;

    juce::TextButton randomiseButton;


    void copyPathParameters();

    void updateCourierInterval();
    void randomiseCourierInterval();

    void updateUpColour();
    void randomiseUpColour();

    void updateDownColour();
    void randomiseDownColour();

    void randomiseAllParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayPathEditor)
};