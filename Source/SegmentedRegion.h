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
class SegmentedRegion final : public juce::DrawableButton, public juce::Timer, public juce::MidiKeyboardState::Listener
{
public:
    SegmentedRegion(const juce::Path& outline, const juce::Rectangle<float>& relativeBounds, const juce::Rectangle<int>& parentBounds, juce::Colour fillColour, AudioEngine* audioEngine)/* :
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

    void transitionToState(SegmentedRegionStateIndex stateToTransitionTo, bool keepPlayingAndEditing = false);

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

    void setIsPlaying_Click(bool shouldBePlaying);
    bool shouldBePlaying();
    void startPlaying(bool toggleButtonState = true);
    void stopPlaying(bool toggleButtonState = true);

    int getMidiChannel();
    void setMidiChannel(int newMidiChannel);
    int getMidiNote();
    void setMidiNote(int newNoteNumber);
    void handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;

    void signalCourierEntered();
    void signalCourierLeft();
    void resetCouriers();
    int getNumCouriers();

    RegionLfo* getAssociatedLfo();
    AudioEngine* getAudioEngine();
    juce::Array<Voice*> getAssociatedVoices();
    juce::String getFileName();

    bool serialise(juce::XmlElement* xmlRegion, juce::Array<juce::MemoryBlock>* attachedData);
    bool deserialise(juce::XmlElement* xmlRegion, juce::Array<juce::MemoryBlock>* attachedData);

    juce::Rectangle<float> relativeBounds;

protected:
    void buttonStateChanged() override;

    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    int ID;

    SegmentedRegionState* states[static_cast<int>(SegmentedRegionStateIndex::StateIndexCount)];
    static const SegmentedRegionStateIndex initialStateIndex = SegmentedRegionStateIndex::notInteractable;
    SegmentedRegionStateIndex currentStateIndex;
    SegmentedRegionState* currentState = nullptr;

    bool shouldBeToggleable = false;

    bool isPlaying = false; //states whether the region is playing. equals (isPlaying_click || isPlaying_courier || isPlaying_midi).
    bool isPlaying_click = false; //states whether the user has attempted to play the region by clicking it.
    bool isPlaying_courier = false; //states whether one or more couriers are within a region, causing it to play.
    bool isPlaying_midi = false; //states whether the MIDI note associated with this region is pressed, causing the region to play.
    int currentVoiceIndex = 0;
    int currentCourierCount = 0; //used to determine whether a region should start/stop playing when a courier enters/exits it (depending on whether there are already couriers contained at that time)

    int midiChannel = -1; //-1 = none, 0 = any, 1...16 = [channel]
    int noteNumber = -1; //-1 = none, 0...127 = [note]

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
    static const float disabledTransparencyOutline;
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
