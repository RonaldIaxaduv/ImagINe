/*
  ==============================================================================

    RegionEditor.h
    Created: 8 Jul 2022 10:00:28am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DahdsrEnvelopeEditor.h"
#include "LfoEditor.h"
#include "PitchQuantisationMethod.h"

//forward referencing in the headers to allow the classes to reference one another
#include "SegmentedRegion.h"
class SegmentedRegion;


class RegionEditor final : public juce::Component//, public juce::SettableTooltipClient
{
public:
    RegionEditor(SegmentedRegion* region);
    ~RegionEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key/*, Component* originatingComponent*/) override;

    void refreshParameters();

    SegmentedRegion* getAssociatedRegion();

private:
    void setChildVisibility(bool shouldBeVisible);

    void copyRegionParameters();

    void selectFile();

    void updateFocusPosition();
    void randomiseFocusPosition();

    void renderLfoWaveform();

    void updateToggleable();

    void updateAllVoiceSettings();
  
    void updateVolume();
    void randomiseVolume();
    
    void updatePitch();
    void randomisePitch();
    void randomisePitchQuantisation();

    void updatePlaybackPositionStart();
    void randomisePlaybackPositionStart();

    void updatePlaybackPositionInterval();
    void randomisePlaybackPositionInterval();

    void updateFilterType();
    void randomiseFilterType();

    void updateFilterPosition();
    void randomiseFilterPosition();

    void randomiseAllParameters();

    //================================================================
    SegmentedRegion* associatedRegion;

    juce::Label selectedFileLabel;
    juce::TextButton selectFileButton;

    juce::Label focusPositionLabel;
    juce::Slider focusPositionX; //inc/dec slider 
    juce::Slider focusPositionY; //inc/dec slider
    juce::Label lfoDepth; //adjustable using the focus position. hence, it's listed here

    juce::ToggleButton toggleModeButton;

    DahdsrEnvelopeEditor dahdsrEditor;

    juce::Label volumeLabel;
    juce::Slider volumeSlider;

    juce::Label pitchLabel;
    juce::Slider pitchSlider;
    juce::Label pitchQuantisationLabel;
    juce::ComboBox pitchQuantisationChoice;

    juce::Label playbackPositionStartLabel;
    juce::Slider playbackPositionStartSlider;

    juce::Label playbackPositionIntervalLabel;
    juce::Slider playbackPositionIntervalSlider;

    juce::Label filterTypeLabel;
    juce::ComboBox filterTypeChoice;
    juce::Label filterPositionLabel;
    juce::Slider filterPositionSlider;

    juce::Label midiChannelLabel;
    juce::ComboBox midiChannelChoice;
    juce::Label midiNoteLabel;
    juce::ComboBox midiNoteChoice;

    LfoEditor lfoEditor;

    juce::TextButton randomiseButton;

    std::unique_ptr<juce::FileChooser> fc;
    juce::AudioFormatManager formatManager;
    juce::AudioSampleBuffer tempBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegionEditor)
};
