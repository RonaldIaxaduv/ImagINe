/*
  ==============================================================================

    SegmentedRegion.h
    Created: 9 Jun 2022 9:03:01am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
//#include "SegmentedRegionState.h"
#include "SegmentedRegionStateIndex.h"
#include "SegmentedRegionStates.h"
class SegmentedRegionState;
class SegmentedRegionState_NotInteractable;
class SegmentedRegionState_Editable;
class SegmentedRegionState_Playable;

#include "AudioEngine.h"
class AudioEngine;

#include "Voice.h"
struct Voice;

#include "RegionEditorWindow.h"
class RegionEditorWindow;

#include "RegionLfo.h"

//==============================================================================
/*
*/
class SegmentedRegion : public juce::DrawableButton, juce::Timer
{
public:
    SegmentedRegion(const juce::Path& outline, const juce::Rectangle<float>& relativeBounds, AudioEngine* audioEngine)/* :
        juce::DrawableButton("", ButtonStyle::ImageStretched),
        p(outline),
        relativeBounds(relativeBounds)*/;
    ~SegmentedRegion() override;

    void initialiseImages();

    void timerCallback() override;
    void setTimerInterval(int newIntervalMs);

    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    bool hitTest(int x, int y) override;

    //void setState(SegmentedRegionState newState);
    void transitionToState(SegmentedRegionStateIndex stateToTransitionTo);

    void triggerButtonStateChanged();
    void triggerDrawableButtonStateChanged();

    void clicked(const juce::ModifierKeys& modifiers) override;

    void setBuffer(juce::AudioSampleBuffer newBuffer, juce::String fileName, double origSampleRate);

    void renderLfoWaveform();

    int getID();

    bool isEditorOpen();
    void sendEditorToFront();
    void openEditor();

    void startPlaying();
    void stopPlaying();

    RegionLfo* getAssociatedLfo();
    AudioEngine* getAudioEngine();
    Voice* getAssociatedVoice();
    juce::String getFileName();

    juce::Point<float> focus;
    juce::Rectangle<float> relativeBounds;

protected:
    void buttonStateChanged() override;

private:
    int ID;

    //SegmentedRegionState currentState;

    SegmentedRegionState* states[static_cast<int>(SegmentedRegionStateIndex::StateIndexCount)];
    static const SegmentedRegionStateIndex initialStateIndex = SegmentedRegionStateIndex::notInteractable;
    SegmentedRegionStateIndex currentStateIndex;
    SegmentedRegionState* currentState = nullptr;

    bool isPlaying = false;
    int timerIntervalMs = 50; //-> 20.0f Hz

    juce::Path p; //also acts as a hitbox
    juce::Colour fillColour;
    juce::DrawablePath normalImage; //normal image when not toggleable or toggled off
    juce::DrawablePath overImage; //image when hovering over the button when not toggleable or toggled off
    juce::DrawablePath downImage; //image when clicking the button when not toggleable or toggled off
    juce::DrawablePath disabledImage; //image when disabled when not toggleable or toggled off
    juce::DrawablePath normalImageOn; //normal image when toggleable and toggled on
    juce::DrawablePath overImageOn; //image when hovering over the button when toggleable and toggled on
    juce::DrawablePath downImageOn; //image when clicking the button when toggleable and toggled on
    juce::DrawablePath disabledImageOn; //image when disabled when toggleable and toggled on

    AudioEngine* audioEngine;
    juce::AudioSampleBuffer buffer;
    juce::String audioFileName = "";
    Voice* associatedVoice = nullptr;

    RegionLfo* associatedLfo = nullptr;

    juce::Component::SafePointer<RegionEditorWindow> regionEditorWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentedRegion)
};
