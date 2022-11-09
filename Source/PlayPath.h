/*
  ==============================================================================

    PlayPath.h
    Created: 7 Nov 2022 8:54:08am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PlayPathStateIndex.h"
class PlayPathState;

#include "PlayPathCourier.h"
#include "PlayPathEditorWindow.h"
class PlayPathEditorWindow;

#include "SegmentedRegion.h"

class PlayPath final : public juce::DrawableButton
{
public:
    PlayPath(int ID, const juce::Path& path, const juce::Rectangle<float>& relativeBounds, juce::Colour fillColour);
    ~PlayPath();

    void transitionToState(PlayPathStateIndex stateToTransitionTo);

    void resized() override;
    bool hitTest(int x, int y) override;
    bool hitTest_Interactable(int x, int y);

    juce::Point<float> getPointAlongPath(float distanceFromStart);
    float getPathLength();

    void clicked(const juce::ModifierKeys& modifiers) override;

    bool isEditorOpen();
    void sendEditorToFront();
    void openEditor();

    void triggerButtonStateChanged();
    void triggerDrawableButtonStateChanged();

    void startPlaying();
    void stopPlaying();

    int getID();

    juce::Colour getFillColour();
    void setFillColour(juce::Colour newFillColour);

    float getCourierInterval_seconds();
    void setCourierInterval_seconds(float newCourierIntervalSeconds);

    void addIntersectingRegion(SegmentedRegion* region);
    void recalculateAllIntersectingRegions();
    void removeIntersectingRegion(int regionID);
    void evaluateCourierPosition(juce::Range<float> previousPosition, juce::Range<float> newPosition);

    bool serialise(juce::XmlElement* xmlPlayPath);
    bool deserialise(juce::XmlElement* xmlPlayPath);

    juce::Rectangle<float> relativeBounds;

private:
    PlayPathState* states[static_cast<int>(PlayPathStateIndex::StateIndexCount)];
    PlayPathStateIndex initialStateIndex = PlayPathStateIndex::notInteractable;
    PlayPathStateIndex currentStateIndex;
    PlayPathState* currentState = nullptr;

    int ID;
    juce::Path underlyingPath;
    float courierIntervalSeconds = 5.0f;

    juce::Colour fillColour;
    static const float outlineThickness;
    juce::DrawablePath normalImage; //normal image when not toggleable or toggled off
    juce::DrawablePath overImage; //image when hovering over the button when not toggleable or toggled off
    juce::DrawablePath downImage; //image when clicking the button when not toggleable or toggled off
    juce::DrawablePath disabledImage; //image when disabled when not toggleable or toggled off
    juce::DrawablePath normalImageOn; //normal image when toggleable and toggled on
    juce::DrawablePath overImageOn; //image when hovering over the button when toggleable and toggled on
    juce::DrawablePath downImageOn; //image when clicking the button when toggleable and toggled on
    juce::DrawablePath disabledImageOn; //image when disabled when toggleable and toggled on
    void initialiseImages();

    bool isPlaying = false;

    juce::OwnedArray<PlayPathCourier> couriers;

    //keep a double array to remember at which distance from the start of the path a region is entered (much more efficient than searching for components using hitTest since it can be pre-computed)
    juce::Array<juce::Range<float>> regionsByRange_range;
    juce::Array<SegmentedRegion*> regionsByRange_region;
    void insertIntoRegionsLists(juce::Range<float> regionRange, SegmentedRegion* region);

    juce::Component::SafePointer<PlayPathEditorWindow> pathEditorWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayPath)
};