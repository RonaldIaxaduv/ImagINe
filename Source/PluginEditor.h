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
        Null = 0,
        Init,
        Drawing,
        Editing,
        Playing
    };

    void transitionToState(PluginEditorStateIndex stateToTransitionTo);

    bool serialise(juce::XmlElement* xmlProcessor, juce::Array<juce::MemoryBlock>* attachedData);
    bool deserialise(juce::XmlElement* xmlProcessor, juce::Array<juce::MemoryBlock>* attachedData);

private:
    //==============================================================================
    void updateState();

    void showLoadImageDialogue();

    //==============================================================================
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ImageINeDemoAudioProcessor& audioProcessor;

    juce::TextButton loadImageButton;

    PluginEditorStateIndex currentStateIndex = PluginEditorStateIndex::Null;

    juce::Label modeLabel;
    juce::ComboBox modeBox;

    SegmentableImage image;

    std::unique_ptr<juce::FileChooser> fc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageINeDemoAudioProcessorEditor)
};
