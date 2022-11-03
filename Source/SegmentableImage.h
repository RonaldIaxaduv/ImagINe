/*
  ==============================================================================

    SegmentableImage.h
    Created: 9 Jun 2022 9:02:42am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SegmentableImageStateIndex.h"
#include "SegmentableImageStates.h"
class SegmentableImageState;
class SegmentableImageState_Empty;
class SegmentableImageState_WithImage;
class SegmentableImageState_DrawingRegion;
class SegmentableImageState_EditingRegions;
class SegmentableImageState_PlayingRegions;

#include "SegmentedRegion.h"


//==============================================================================
/*
*/
class SegmentableImage : public juce::ImageComponent, public juce::KeyListener
{
public:
    SegmentableImage(AudioEngine* audioEngine);
    ~SegmentableImage() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void transitionToState(SegmentableImageStateIndex stateToTransitionTo);

    void setImage(const juce::Image& newImage);

    //to add point to path
    void mouseDown(const juce::MouseEvent& event) override;

    //to finish/restart path
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;


    //================================================================
    
    void startNewRegion(juce::Point<float> newPt);
    void addPointToPath(juce::Point<float> newPt);

    void resetPath();
    void clearRegions();
    void removeRegion(int regionID);

    void tryCompletePath();

    void deleteLastNode();

    bool serialise(juce::XmlElement* xmlEditor, juce::Array<juce::MemoryBlock>* attachedData);
    bool deserialise(juce::XmlElement* xmlEditor, juce::Array<juce::MemoryBlock>* attachedData);

    //================================================================

    /// <summary>
    /// 2D Array. Entry [x][y] states whether the pixel at position [x][y] of the image has been segmented yet
    /// </summary>
    //juce::uint8** segmentedPixels; //WIP: "Always prefer a juce::HeapBlock or some other container class." -> change it to that

    juce::OwnedArray<SegmentedRegion> regions;
    bool hasAtLeastOneRegion();
    bool hasAtLeastOneRegionWithAudio();

private:
    SegmentableImageState* states[static_cast<int>(SegmentableImageStateIndex::StateIndexCount)];
    static const SegmentableImageStateIndex initialStateIndex = SegmentableImageStateIndex::empty;
    SegmentableImageStateIndex currentStateIndex;
    SegmentableImageState* currentState = nullptr;

    juce::Path currentPath; //allows to easily reduce the area that has to be redrawn when adding/removing drawing points
    juce::Array<juce::Point<float>> currentPathPoints; //WIP: normalise these to [0...1] so that, when resizing, the path can be redrawn
    juce::Rectangle<float> getAbsolutePathBounds();

    AudioEngine* audioEngine;

    void addRegion(SegmentedRegion* newRegion);
    void repaintAllRegions();

    void clearPath();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentableImage)
};
