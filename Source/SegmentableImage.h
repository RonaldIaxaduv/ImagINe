/*
  ==============================================================================

    SegmentableImage.h
    Created: 9 Jun 2022 9:02:42am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SegmentableImageState.h"
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

    void setImage(const juce::Image& newImage);

    void setState(SegmentableImageState newState);

    //to add point to path
    void mouseDown(const juce::MouseEvent& event) override;

    //to finish/restart path
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;


    //================================================================

    /// <summary>
    /// 2D Array. Entry [x][y] states whether the pixel at position [x][y] of the image has been segmented yet
    /// </summary>
    //juce::uint8** segmentedPixels; //WIP: "Always prefer a juce::HeapBlock or some other container class." -> change it to that

    juce::OwnedArray<SegmentedRegion> regions;

private:
    SegmentableImageState currentState;

    bool hasImage;

    bool drawsPath;
    juce::Path currentPath;
    juce::Array<juce::Point<float>> currentPathPoints; //WIP: normalise these to [0...1] so that, when resizing, the path can be redrawn

    AudioEngine* audioEngine;

    void resetPath();

    void tryCompletePath();

    void tryDeleteLastNode();

    void addRegion(SegmentedRegion* newRegion);
    void clearRegions();

    void clearPath();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentableImage)
};
