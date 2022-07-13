/*
  ==============================================================================

    LfoEditor.h
    Created: 8 Jul 2022 10:00:12am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "LfoModulatableParameter.h"
#include "CheckBoxList.h"
#include "AudioEngine.h"
#include "RegionLfo.h"

class LfoEditor : public juce::Component, public juce::ChangeListener
{
public:
    LfoEditor(AudioEngine* audioEngine, RegionLfo* associatedLfo);
    ~LfoEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void copyParameters();

    void updateAvailableVoices();

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    AudioEngine* audioEngine;
    RegionLfo* associatedLfo;

    juce::Label lfoRateLabel;
    juce::Slider lfoRateSlider;

    //juce::Label lfoParameterLabel;
    //juce::ComboBox lfoParameterChoice;

    CheckBoxList lfoRegionsList;

    void updateLfoRate();

    void updateLfoParameter(int targetRegionID, bool shouldBeModulated, LfoModulatableParameter modulatedParameter);

    //void updateLfoVoices(juce::Array<int> voiceIndices);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoEditor)
};
