/*
  ==============================================================================

    LfoEditor.h
    Created: 8 Jul 2022 10:00:12am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CheckBoxList.h"
#include "AudioEngine.h"
#include "RegionLfo.h"
#include "LfoModulatableParameter.h"

class LfoEditor : public juce::Component, public juce::ChangeListener, public juce::SettableTooltipClient
{
public:
    LfoEditor(AudioEngine* audioEngine, RegionLfo* associatedLfo);
    ~LfoEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;

    void copyParameters();

    void updateAvailableVoices();

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void randomiseAllParameters();

private:
    AudioEngine* audioEngine;
    RegionLfo* associatedLfo;

    juce::Label lfoRateLabel;
    juce::Slider lfoRateSlider;

    juce::Label lfoStartingPhaseLabel;
    juce::Slider lfoStartingPhaseSlider;

    juce::Label lfoPhaseIntervalLabel;
    juce::Slider lfoPhaseIntervalSlider;

    juce::Label lfoUpdateIntervalLabel;
    juce::Slider lfoUpdateIntervalSlider;
    juce::Label lfoUpdateQuantisationLabel;
    juce::ComboBox lfoUpdateQuantisationChoice;

    CheckBoxList lfoRegionsList;

    void updateLfoRate();
    void randomiseLfoRate();

    void updateLfoStartingPhase();
    void randomiseLfoStartingPhase();

    void updateLfoPhaseInterval();
    void randomiseLfoPhaseInterval();

    void updateLfoUpdateInterval();
    void randomiseLfoUpdateInterval();

    void updateLfoUpdateQuantisation();
    void randomiseLfoUpdateQuantisation();

    void updateLfoParameter(int targetRegionID, bool shouldBeModulated, LfoModulatableParameter modulatedParameter);



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoEditor)
};
