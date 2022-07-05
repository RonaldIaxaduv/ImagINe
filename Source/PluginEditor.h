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

private:
    //==============================================================================
    void updateState();

    void showLoadImageDialogue();

    //==============================================================================
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ImageINeDemoAudioProcessor& audioProcessor;

    juce::TextButton loadImageButton;

    enum State
    {
        Null = 0, //only takes on this value before the MainComponent constructor has finished
        Init = 1, //note: cannot set the first ID to 0 (reserved for when no ID is selected in a ComboBox - and since these values will be used in a ComboBox, 0 is skipped here)
        Drawing = 2,
        Editing = 3,
        Playing = 4
    };
    State currentState = State::Null;

    juce::Label modeLabel;
    juce::ComboBox modeBox;

    SegmentableImage image;

    std::unique_ptr<juce::FileChooser> fc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageINeDemoAudioProcessorEditor)
};
