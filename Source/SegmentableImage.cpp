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
    hasImage = false;
    drawsPath = false;
    currentPathPoints = { };

    this->audioEngine = audioEngine;

    setSize(500, 500);

    regions = { };
}

SegmentableImage::~SegmentableImage()
{
    resetPath();
    clearRegions();
    /*for (int x = 0; x < getImage().getWidth(); ++x)
    {
        segmentedPixels[x] = nullptr;
    }
    segmentedPixels = nullptr;*/
    audioEngine = nullptr;
}

void SegmentableImage::paint(juce::Graphics& g)
{
    juce::ImageComponent::paint(g);

    if (drawsPath)
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

void SegmentableImage::setImage(const juce::Image& newImage)
{
    hasImage = !newImage.isNull();

    if (newImage.isNull())
        DBG("new image was null.");

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

    clearRegions();
    clearPath();
    currentPathPoints = { };

    juce::ImageComponent::setImage(newImage); //also automatically repaints the component
}

void SegmentableImage::setState(SegmentableImageState newState)
{
    if (currentState != newState)
    {
        currentState = newState;
        DBG("set SegmentableImage's current state to " + juce::String(static_cast<int>(currentState)));

        switch (currentState)
        {
        case SegmentableImageState::Init:
            break;

        case SegmentableImageState::Drawing:
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                //(*it)->setState(SegmentedRegionState::Standby);
                (*it)->transitionToState(SegmentedRegionStateIndex::notInteractable);
            }
            break;

        case SegmentableImageState::Editing:
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                //(*it)->setState(SegmentedRegionState::Editing);
                (*it)->transitionToState(SegmentedRegionStateIndex::editable);
            }
            break;

        case SegmentableImageState::Playing:
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                //(*it)->setState(SegmentedRegionState::Playable);
                (*it)->transitionToState(SegmentedRegionStateIndex::playable);
            }
            break;

        default:
            break;
        }

        resized();
    }
}

//to add point to path
void SegmentableImage::mouseDown(const juce::MouseEvent& event)
{
    juce::ImageComponent::mouseDown(event);
    DBG("Click!");
    juce::Point<float> clickPosition = event.position;

    if (currentState == SegmentableImageState::Drawing && hasImage)
    {
        if (getBounds().contains(clickPosition.toInt()))
        {
            if (!drawsPath)
            {
                DBG("new path: " + clickPosition.toString());
                currentPath.startNewSubPath(clickPosition);
                drawsPath = true;
            }
            else //append next point
            {
                DBG("next point: " + clickPosition.toString());
                currentPath.lineTo(clickPosition);
            }
            currentPathPoints.add(clickPosition);
            repaint(currentPath.getBounds().expanded(3.0f, 3.0f).toNearestInt());
        }
    }
}

//to finish/restart path
bool SegmentableImage::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    DBG("key pressed");
    if (currentState == SegmentableImageState::Drawing && hasImage)
    {
        if (key == juce::KeyPress::backspaceKey)
        {
            DBG("backspace");
            tryDeleteLastNode();
            return true;
        }
        else if (key.getTextCharacter() == 'o')
        {
            DBG("o");
            tryCompletePath();
            return true;
        }
        else if (key.getTextCharacter() == 'r')
        {
            DBG("r");
            resetPath();
            return true;
        }
        //else if (key.getTextCharacter() == 'f')
        //{
        //    DBG("f");
        //    //WIP: fill rest as background region(s)? -> actually, why fill the background? not really needed, nor very artistic imo

        //}
        else
        {
            DBG("unhandled key");
            return false;
        }
    }
}




//private

void SegmentableImage::resetPath()
{
    clearPath();
    currentPathPoints.clear();
}

void SegmentableImage::tryCompletePath()
{
    if (drawsPath)
    {
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

        resetPath();
    }
}

void SegmentableImage::tryDeleteLastNode()
{
    if (drawsPath)
    {
        clearPath();
        currentPathPoints.removeLast();

        if (!currentPathPoints.isEmpty())
        {
            for (auto itPt = currentPathPoints.begin(); itPt != currentPathPoints.end(); itPt++)
            {
                if (!drawsPath)
                {
                    currentPath.startNewSubPath(*itPt);
                    DBG("path empty");
                    drawsPath = true;
                }
                else
                {
                    currentPath.lineTo(*itPt);
                }
            }
            repaint(currentPath.getBounds().expanded(3.0f, 3.0f).toNearestInt());
        }
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
    for (auto it = regions.begin(); it != regions.end(); ++it)
    {
        removeChildComponent(*it);
    }
    regions.clear();
}

void SegmentableImage::clearPath()
{
    auto formerArea = currentPath.getBounds().expanded(3.0f, 3.0f).toNearestInt();
    currentPath.clear();
    //currentPathPoints remain! if you want to remove them, use resetPath() instead.
    drawsPath = false;
    repaint(formerArea);
}
