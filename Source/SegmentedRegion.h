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
class SegmentedRegion final : public juce::DrawableButton, public juce::Timer
{
public:
    SegmentedRegion(const juce::Path& outline, const juce::Rectangle<float>& relativeBounds, juce::Colour fillColour, AudioEngine* audioEngine)/* :
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
    bool hitTest_Interactable(int x, int y);

    void forceRepaint();

    //void setState(SegmentedRegionState newState);
    void transitionToState(SegmentedRegionStateIndex stateToTransitionTo);

    void setShouldBeToggleable(bool newShouldBeToggleable);
    bool getShouldBeToggleable();

    void triggerButtonStateChanged();
    void triggerDrawableButtonStateChanged();

    void clicked(const juce::ModifierKeys& modifiers) override;

    void setBuffer(juce::AudioSampleBuffer newBuffer, juce::String fileName, double origSampleRate);

    void renderLfoWaveform();

    int getID();

    juce::Point<float> getFocusPoint();
    void setFocusPoint(juce::Point<float> newFocusPoint);

    bool isEditorOpen();
    void sendEditorToFront();
    void openEditor();
    void refreshEditor();

    void startPlaying();
    void stopPlaying();

    RegionLfo* getAssociatedLfo();
    AudioEngine* getAudioEngine();
    juce::Array<Voice*> getAssociatedVoices();
    juce::String getFileName();

    juce::Rectangle<float> relativeBounds;

    bool serialise(juce::XmlElement* xmlRegion, juce::Array<juce::MemoryBlock>* attachedData);
    bool deserialise(juce::XmlElement* xmlRegion, juce::Array<juce::MemoryBlock>* attachedData);

protected:
    void buttonStateChanged() override;

private:
    int ID;

    //SegmentedRegionState currentState;

    SegmentedRegionState* states[static_cast<int>(SegmentedRegionStateIndex::StateIndexCount)];
    static const SegmentedRegionStateIndex initialStateIndex = SegmentedRegionStateIndex::notInteractable;
    SegmentedRegionStateIndex currentStateIndex;
    SegmentedRegionState* currentState = nullptr;

    bool shouldBeToggleable = false;

    bool isPlaying = false;
    int currentVoiceIndex = 0;
    int timerIntervalMs = 50; //-> 20.0f Hz
    juce::Line<float> currentLfoLine;
    static const float lfoLineThickness;

    juce::Path p; //also acts as a hitbox

    static const float focusRadius;
    juce::Point<float> focus;
    juce::Point<float> focusAbs; //focus, but applied to the current (actual) bounds of the region

    juce::Colour fillColour;
    juce::Colour lfoLineColour;
    juce::Colour focusPointColour;

    static const float outlineThickness;
    static const float inherentTransparency;
    static const float disabledTransparency;
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
    double origSampleRate = 0.0;

    juce::Array<Voice*> associatedVoices;
    RegionLfo* associatedLfo = nullptr;

    juce::Component::SafePointer<RegionEditorWindow> regionEditorWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentedRegion)
};
