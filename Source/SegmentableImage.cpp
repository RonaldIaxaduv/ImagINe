/*
  ==============================================================================

    SegmentableImage.cpp
    Created: 8 Jul 2022 10:50:34am
    Author:  Aaron

  ==============================================================================
*/

#include "SegmentableImage.h"


//public

SegmentableImage::SegmentableImage(AudioEngine* audioEngine) : juce::ImageComponent()
{
    //initialise states
    states[static_cast<int>(SegmentableImageStateIndex::empty)] = static_cast<SegmentableImageState*>(new SegmentableImageState_Empty(*this));
    states[static_cast<int>(SegmentableImageStateIndex::withImage)] = static_cast<SegmentableImageState*>(new SegmentableImageState_WithImage(*this));
    states[static_cast<int>(SegmentableImageStateIndex::drawingRegion)] = static_cast<SegmentableImageState*>(new SegmentableImageState_DrawingRegion(*this));
    states[static_cast<int>(SegmentableImageStateIndex::editingRegions)] = static_cast<SegmentableImageState*>(new SegmentableImageState_EditingRegions(*this));
    states[static_cast<int>(SegmentableImageStateIndex::playingRegions)] = static_cast<SegmentableImageState*>(new SegmentableImageState_PlayingRegions(*this));
    int initialisedStates = 5;
    jassert(initialisedStates == static_cast<int>(SegmentableImageStateIndex::StateIndexCount));
    
    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];

    //initialise remaining members
    //currentPathPoints = { };
    //regions = { };
    this->audioEngine = audioEngine;

    setSize(500, 500);
}

SegmentableImage::~SegmentableImage()
{
    DBG("destroying SegmentableImage...");

    //release states
    currentState = nullptr;
    delete states[static_cast<int>(SegmentableImageStateIndex::empty)];
    delete states[static_cast<int>(SegmentableImageStateIndex::withImage)];
    delete states[static_cast<int>(SegmentableImageStateIndex::drawingRegion)];
    delete states[static_cast<int>(SegmentableImageStateIndex::editingRegions)];
    delete states[static_cast<int>(SegmentableImageStateIndex::playingRegions)];
    int deletedStates = 5;
    jassert(deletedStates == static_cast<int>(SegmentableImageStateIndex::StateIndexCount));

    resetPath();
    clearRegions();
    /*for (int x = 0; x < getImage().getWidth(); ++x)
    {
        segmentedPixels[x] = nullptr;
    }
    segmentedPixels = nullptr;*/
    audioEngine = nullptr;

    DBG("SegmentableImage destroyed.");
}

void SegmentableImage::paint(juce::Graphics& g)
{
    juce::ImageComponent::paint(g);

    if (currentPathPoints.size() > 0)
    {
        g.setColour(juce::Colours::gold);
        //g.strokePath(currentPath, juce::PathStrokeType(5.0f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
        //g.fillPath(currentPath);

        juce::Point<float> p1, p2;
        float dotRadius = 2.0f, lineThickness = 1.5f;

        p1 = currentPathPoints[0];
        g.fillEllipse(p1.x - dotRadius, p1.y - dotRadius, dotRadius * 2, dotRadius * 2);

        for (int i = 1; i < currentPathPoints.size(); ++i)
        {
            p2 = currentPathPoints[i];
            g.drawLine(juce::Line<float>(p1, p2), lineThickness);
            g.fillEllipse(p2.x - dotRadius, p2.y - dotRadius, dotRadius * 2, dotRadius * 2);
            p1 = p2;
        }
    }
}

void SegmentableImage::resized()
{
    juce::ImageComponent::resized();

    //resize regions
    juce::Rectangle<float> curBounds = getBounds().toFloat();

    for (SegmentedRegion* r : regions)
    {
        auto relativeBounds = r->relativeBounds;
        juce::Rectangle<float> newBounds(relativeBounds.getX() * curBounds.getWidth(),
            relativeBounds.getY() * curBounds.getHeight(),
            relativeBounds.getWidth() * curBounds.getWidth(),
            relativeBounds.getHeight() * curBounds.getHeight());
        r->setBounds(newBounds.toNearestInt());
    }
}

void SegmentableImage::transitionToState(SegmentableImageStateIndex stateToTransitionTo)
{
    bool nonInstantStateFound = false;

    while (!nonInstantStateFound) //check if any states can be skipped (might be the case depending on what has been prepared already)
    {
        switch (stateToTransitionTo)
        {
        case SegmentableImageStateIndex::empty:
            resetPath();
            clearRegions();
            nonInstantStateFound = true;
            DBG("segmentable image empty");
            break;

        case SegmentableImageStateIndex::withImage:
            if (currentStateIndex == SegmentableImageStateIndex::drawingRegion && currentPathPoints.size() > 0)
            {
                //currently drawing -> remain in that state
                return;
            }

            //make regions uninteractable
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->transitionToState(SegmentedRegionStateIndex::notInteractable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image with image");
            break;

        case SegmentableImageStateIndex::drawingRegion:
            //make regions uninteractable
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->transitionToState(SegmentedRegionStateIndex::notInteractable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image drawing region");
            break;

        case SegmentableImageStateIndex::editingRegions:
            //clear unfinished paths
            resetPath();

            //set regions to editable
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->transitionToState(SegmentedRegionStateIndex::editable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image editing regions");
            break;

        case SegmentableImageStateIndex::playingRegions:
            //clear unfinished paths
            resetPath();

            //set regions to playable
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->transitionToState(SegmentedRegionStateIndex::playable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image playing regions");
            break;

        default:
            throw std::exception("unhandled state index");
        }
    }

    //finally, update the current state index
    currentStateIndex = stateToTransitionTo;
    currentState = states[static_cast<int>(currentStateIndex)];

    //repaint();
}

void SegmentableImage::setImage(const juce::Image& newImage)
{
    //segmentedPixels = new juce::uint8* [newImage.getWidth()];
    //for (int x = 0; x < newImage.getWidth(); ++x)
    //{
    //    //segmentedPixels[x] = new bool[newImage.getHeight()];
    //    segmentedPixels[x] = new juce::uint8[newImage.getHeight()];
    //    for (int y = 0; y < newImage.getHeight(); ++y)
    //    {
    //        //segmentedPixels[x][y] = false;
    //        segmentedPixels[x][y] = 0; //set to false
    //    }
    //}

    juce::ImageComponent::setImage(newImage); //also automatically repaints the component
    currentState->imageChanged(newImage);
}

//to add point to path
void SegmentableImage::mouseDown(const juce::MouseEvent& event)
{
    //juce::ImageComponent::mouseDown(event);
    currentState->mouseDown(event);
}

//to finish/restart path
bool SegmentableImage::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    return currentState->keyPressed(key, originatingComponent);
}

void SegmentableImage::startNewRegion(juce::Point<float> newPt)
{
    if (getBounds().contains(newPt.toInt()))
    {
        DBG("new path: " + newPt.toString());
        currentPath.startNewSubPath(newPt);
        currentPathPoints.add(newPt);
        repaint(currentPath.getBounds().expanded(3.0f, 3.0f).toNearestInt());
        transitionToState(SegmentableImageStateIndex::drawingRegion);
    }
}
void SegmentableImage::addPointToPath(juce::Point<float> newPt)
{
    if (getBounds().contains(newPt.toInt()))
    {
        DBG("next point: " + newPt.toString());
        currentPath.lineTo(newPt);
        currentPathPoints.add(newPt);
        repaint(currentPath.getBounds().expanded(3.0f, 3.0f).toNearestInt());
    }
}





void SegmentableImage::resetPath()
{
    clearPath();
    currentPathPoints.clear();
}

void SegmentableImage::tryCompletePath()
{
    //check whether there are enough points to form a 2D region
    if (currentPathPoints.size() < 3)
    {
        DBG("not enough points to form a region.");
        return;
    }

    //WIP: check whether all points jus form a line, or whether they actually define a 2D region
    //...

    currentPath.closeSubPath();

    juce::Rectangle<float> origRegionBounds(currentPath.getBounds().toFloat());
    juce::Rectangle<float> origParentBounds(getBounds().toFloat());
    juce::Rectangle<float> relativeBounds(origRegionBounds.getX() / origParentBounds.getWidth(),
        origRegionBounds.getY() / origParentBounds.getHeight(),
        origRegionBounds.getWidth() / origParentBounds.getWidth(),
        origRegionBounds.getHeight() / origParentBounds.getHeight());

    juce::Random rng;
    juce::Colour fillColour = juce::Colour::fromHSV(rng.nextFloat(), 0.6f + 0.4f * rng.nextFloat(), 0.6f + 0.4f * rng.nextFloat(), 1.0f); //random for now

    SegmentedRegion* newRegion = new SegmentedRegion(currentPath, relativeBounds, fillColour, audioEngine);
    newRegion->setBounds(currentPath.getBounds().toNearestInt());
    addRegion(newRegion);
    //newRegion->triggerButtonStateChanged();
    newRegion->triggerDrawableButtonStateChanged();
    DBG("tryCompletePath -- new region generated.");

    resetPath();
    DBG("tryCompletePath -- path reset.");

    transitionToState(SegmentableImageStateIndex::withImage);
    DBG("tryCompletePath -- state transitioned.");
}

void SegmentableImage::deleteLastNode()
{
    clearPath(); //erases the whole Path object (will be reconstructed later)
    currentPathPoints.removeLast();

    if (!currentPathPoints.isEmpty())
    {
        //reconstruct path object up to the previously second last point
        int i = 0;
        for (auto itPt = currentPathPoints.begin(); itPt != currentPathPoints.end(); itPt++, i++)
        {
            if (i == 0)
            {
                currentPath.startNewSubPath(*itPt);
            }
            else
            {
                currentPath.lineTo(*itPt);
            }
        }
        repaint(currentPath.getBounds().expanded(3.0f, 3.0f).toNearestInt());
    }
    else //currentPathPoints.isEmpty()
    {
        transitionToState(SegmentableImageStateIndex::withImage);
    }
}

void SegmentableImage::addRegion(SegmentedRegion* newRegion)
{
    regions.add(newRegion);
    newRegion->setAlwaysOnTop(true);
    //newRegion->onStateChange += [newRegion] { newRegion->handleButtonStateChanged(); };
    addAndMakeVisible(newRegion);
}
void SegmentableImage::clearRegions()
{
    DBG("clearing regions...");
    for (auto it = regions.begin(); it != regions.end(); ++it)
    {
        removeChildComponent(*it);
    }
    audioEngine->suspendProcessing(true);
    regions.clear(true);
    audioEngine->suspendProcessing(false);
    DBG("regions have been cleared.");
}

void SegmentableImage::clearPath()
{
    auto formerArea = currentPath.getBounds().expanded(3.0f, 3.0f).toNearestInt();
    currentPath.clear();
    //currentPathPoints remain! if you want to remove them, use resetPath() instead.
    repaint(formerArea);
}
