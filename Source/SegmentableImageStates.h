/*
  ==============================================================================

    SegmentableImageStates.h
    Created: 7 Oct 2022 3:28:15pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include "SegmentableImageStateIndex.h"
#include "SegmentableImage.h"
class SegmentableImage;


class SegmentableImageState
{
public:
    SegmentableImageState(SegmentableImage& image);

    virtual void imageChanged(const juce::Image& newImage) = 0;

    virtual void mouseDown(const juce::MouseEvent& event) = 0;
    virtual bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) = 0;

protected:
    SegmentableImageStateIndex implementedStateIndex;
    SegmentableImage& image;
};


class SegmentableImageState_Empty final : public SegmentableImageState
{
public:
    SegmentableImageState_Empty(SegmentableImage& image);

    void imageChanged(const juce::Image& newImage) override;

    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
};


class SegmentableImageState_WithImage final : public SegmentableImageState
{
public:
    SegmentableImageState_WithImage(SegmentableImage& image);

    void imageChanged(const juce::Image& newImage) override;

    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
};


class SegmentableImageState_DrawingRegion final : public SegmentableImageState
{
public:
    SegmentableImageState_DrawingRegion(SegmentableImage& image);

    void imageChanged(const juce::Image& newImage) override;

    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
};


class SegmentableImageState_EditingRegions final : public SegmentableImageState
{
public:
    SegmentableImageState_EditingRegions(SegmentableImage& image);

    void imageChanged(const juce::Image& newImage) override;

    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
};


class SegmentableImageState_PlayingRegions final : public SegmentableImageState
{
public:
    SegmentableImageState_PlayingRegions(SegmentableImage& image);

    void imageChanged(const juce::Image& newImage) override;

    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
};


class SegmentableImageState_DrawingPlayPath final : public SegmentableImageState
{
public:
    SegmentableImageState_DrawingPlayPath(SegmentableImage& image);

    void imageChanged(const juce::Image& newImage) override;

    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
};


class SegmentableImageState_EditingPlayPaths final : public SegmentableImageState
{
public:
    SegmentableImageState_EditingPlayPaths(SegmentableImage& image);

    void imageChanged(const juce::Image& newImage) override;

    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
};


class SegmentableImageState_PlayingPlayPaths final : public SegmentableImageState
{
public:
    SegmentableImageState_PlayingPlayPaths(SegmentableImage& image);

    void imageChanged(const juce::Image& newImage) override;

    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
};
