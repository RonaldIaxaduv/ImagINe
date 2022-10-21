/*
  ==============================================================================

    SegmentableImageStates.cpp
    Created: 7 Oct 2022 3:28:15pm
    Author:  Aaron

  ==============================================================================
*/

#include "SegmentableImageStates.h"


SegmentableImageState::SegmentableImageState(SegmentableImage& image) :
    image(image)
{ }




#pragma region SegmentableImageState_Empty

SegmentableImageState_Empty::SegmentableImageState_Empty(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::empty;
}

void SegmentableImageState_Empty::imageChanged(const juce::Image& newImage)
{
    if (!newImage.isNull())
    {
        DBG("valid image loaded.");
        image.transitionToState(SegmentableImageStateIndex::withImage);
    }
    else
    {
        DBG("invalid image passed.");
    }
}

void SegmentableImageState_Empty::mouseDown(const juce::MouseEvent& event)
{
    //nothing to interact with clicks
}

bool SegmentableImageState_Empty::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    if (key.getTextCharacter() == 'o')
    {
        //WIP: open file dialogue
    }

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_Empty




#pragma region SegmentableImageState_WithImage

SegmentableImageState_WithImage::SegmentableImageState_WithImage(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::withImage;
}

void SegmentableImageState_WithImage::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
    }
}

void SegmentableImageState_WithImage::mouseDown(const juce::MouseEvent& event)
{
    //start drawing a region
    image.startNewRegion(event.position);
}

bool SegmentableImageState_WithImage::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());
    
    //no key bindings for this state atm

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_WithImage




#pragma region SegmentableImageState_DrawingRegion

SegmentableImageState_DrawingRegion::SegmentableImageState_DrawingRegion(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::drawingRegion;
}

void SegmentableImageState_DrawingRegion::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
        image.transitionToState(SegmentableImageStateIndex::withImage);
    }
}

void SegmentableImageState_DrawingRegion::mouseDown(const juce::MouseEvent& event)
{
    //add point to region path
    image.addPointToPath(event.position);
}

bool SegmentableImageState_DrawingRegion::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    if (key == juce::KeyPress::backspaceKey)
    {
        image.deleteLastNode();
        return true;
    }
    else if (key == juce::KeyPress::escapeKey)
    {
        image.resetPath();
        return true;
    }
    else if (key.getTextCharacter() == 'o')
    {
        image.tryCompletePath();
        return true;
    }

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_DrawingRegion




#pragma region SegmentableImageState_EditingRegions

SegmentableImageState_EditingRegions::SegmentableImageState_EditingRegions(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::editingRegions;
}

void SegmentableImageState_EditingRegions::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
    }
}

void SegmentableImageState_EditingRegions::mouseDown(const juce::MouseEvent& event)
{
    //nothing to interact with clicks (regions will intercept clicks)
}

bool SegmentableImageState_EditingRegions::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    //currently no key bindings for this state

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_EditingRegions




#pragma region SegmentableImageState_PlayingRegions

SegmentableImageState_PlayingRegions::SegmentableImageState_PlayingRegions(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::playingRegions;
}

void SegmentableImageState_PlayingRegions::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions(); //WIP: this might cause memory access exceptions in this state
    }
}

void SegmentableImageState_PlayingRegions::mouseDown(const juce::MouseEvent& event)
{
    //nothing to interact with clicks (active regions will intercept clicks)
}

bool SegmentableImageState_PlayingRegions::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    //currently no key bindings for this state

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_PlayingRegions
