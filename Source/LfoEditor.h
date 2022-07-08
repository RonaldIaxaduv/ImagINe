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

//forward reference to be able to use SegmentedRegion here
//#include "SegmentedRegion.h"
//class SegmentedRegion;

#include "AudioEngine.h"
#include "RegionLfo.h"

class LfoEditor : public juce::Component
{
public:
    LfoEditor(AudioEngine* audioEngine, RegionLfo* associatedLfo /*SegmentedRegion* associatedRegion*/);
    ~LfoEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void copyParameters();

    void updateAvailableVoices();

private:
    //SegmentedRegion* associatedRegion;
    AudioEngine* audioEngine;
    RegionLfo* associatedLfo;

    juce::Label lfoRateLabel;
    juce::Slider lfoRateSlider;

    juce::Label lfoParameterLabel;
    juce::ComboBox lfoParameterChoice;

    CheckBoxList lfoRegionsList;

    void updateLfoRate();

    void updateLfoParameter();

    void updateLfoVoices(juce::Array<int> voiceIndices);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoEditor)
};
