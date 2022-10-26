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


class RegionEditor : public juce::Component
{
public:
    RegionEditor(SegmentedRegion* region);
    ~RegionEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshParameters();

    SegmentedRegion* getAssociatedRegion();

private:
    void setChildVisibility(bool shouldBeVisible);

    void copyRegionParameters();

    void selectFile();

    void updateFocusPosition();

    void renderLfoWaveform();

    void updateToggleable();

    void updateAllVoiceSettings();
    void updateVolume();
    void updatePitch();
    void updatePlaybackPosition();

    //================================================================
    SegmentedRegion* associatedRegion;

    juce::Label selectedFileLabel;
    juce::TextButton selectFileButton;

    juce::Label colourPickerWIP; //WIP: later, this will be a button that opens a separate window containing a juce::ColourSelector (the GUI for these is rather large, see https://www.ccoderun.ca/juce/api/ColourSelector.png )
    //^- when changing region colours, also call associatedRegion->audioEngine->changeRegionColour

    juce::Label focusPositionLabel;
    juce::Slider focusPositionX; //inc/dec slider 
    juce::Slider focusPositionY; //inc/dec slider

    juce::ToggleButton toggleModeButton;

    DahdsrEnvelopeEditor dahdsrEditor;

    juce::Label volumeLabel;
    juce::Slider volumeSlider;

    juce::Label pitchLabel;
    juce::Slider pitchSlider;
    juce::Label pitchQuantisationLabel;
    juce::ComboBox pitchQuantisationChoice;

    LfoEditor lfoEditor;

    std::unique_ptr<juce::FileChooser> fc;
    juce::AudioFormatManager formatManager;
    juce::AudioSampleBuffer tempBuffer;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegionEditor)
};
