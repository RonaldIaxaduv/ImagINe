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
    void resized() override;

    enum class PluginEditorStateIndex : int
    {
        null = 0,
        init,
        drawingRegion,
        editingRegions,
        playingRegions,
        drawingPlayPath,
        editingPlayPaths,
        playingPlayPaths
    };

    void transitionToState(PluginEditorStateIndex stateToTransitionTo);

private:
    //==============================================================================
    void updateState();

    void showLoadImageDialogue();

    //==============================================================================
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ImageINeDemoAudioProcessor& audioProcessor;

    juce::TextButton loadImageButton;

    PluginEditorStateIndex currentStateIndex = PluginEditorStateIndex::null;
    void setStateAccordingToImage();

    juce::Label modeLabel;
    juce::ComboBox modeBox;

    SegmentableImage& image;

    std::unique_ptr<juce::FileChooser> fc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageINeDemoAudioProcessorEditor)
};
