/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SegmentableImage.h"

//==============================================================================
/**
*/
class ImageINeDemoAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ImageINeDemoAudioProcessorEditor (ImageINeDemoAudioProcessor&);
    ~ImageINeDemoAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;

    enum class PluginEditorStateIndex : int
    {
        null = 0,
        init,
        drawingRegion,
        editingRegions,
        playingRegions,
        drawingPlayPath,
        editingPlayPaths,
        playingPlayPaths,

        StateIndexCount
    };

    void transitionToState(PluginEditorStateIndex stateToTransitionTo);
    void setStateAccordingToImage();

    void restorePreviousModeBoxSelection();

private:
    //==============================================================================
    void updateState();

    void panic();

    void showOpenImageDialogue();

    void showOpenPresetDialogue();
    void showSavePresetDialogue();

    void setMidiInput(int index);

    void displayModeInformation();

    //==============================================================================
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ImageINeDemoAudioProcessor& audioProcessor;

    juce::TextButton openPresetButton;
    juce::TextButton savePresetButton;

    juce::ComboBox midiInputList;
    juce::Label midiInputListLabel;
    int lastInputIndex = 0;

    juce::TextButton informationButton;

    PluginEditorStateIndex currentStateIndex = PluginEditorStateIndex::null;

    juce::Label modeLabel;
    juce::ComboBox modeBox;

    juce::TextButton panicButton;

    juce::TextButton openImageButton;

    SegmentableImage& image;

    std::unique_ptr<juce::FileChooser> fc;

    juce::SharedResourcePointer<juce::TooltipWindow> tooltipWindow; //used for displaying tooltips. automatically initialised

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageINeDemoAudioProcessorEditor)
};
