/*
  ==============================================================================

    SegmentableImage.h
    Created: 9 Jun 2022 9:02:42am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SegmentedRegion.h"



//==============================================================================
/*
*/
class SegmentableImage : public juce::ImageComponent, public juce::KeyListener
{
public:
    SegmentableImage(AudioEngine* audioEngine) : juce::ImageComponent()
    {
        hasImage = false;
        drawsPath = false;
        currentPathPoints = { };

        this->audioEngine = audioEngine;

        setSize(500, 500);

        regions = { };
    }

    ~SegmentableImage() override
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

    void paint(juce::Graphics& g) override
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

    void resized() override
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

    void setImage(const juce::Image& newImage)
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

    enum State
    {
        Null, //components have not been initialised yet
        Init, //components have been initialised, but no image has been assigned yet. cannot draw regions in this state.
        Drawing, //image has been assigned. regions can be drawn.
        Editing, //regions can no longer be drawn. clicking on them will update the editor in the MainComponent
        Playing //regions can no longer be drawn or edited. instead, they can be played
    };
    void setState(State newState)
    {
        if (currentState != newState)
        {
            currentState = newState;
            DBG("set SegmentableImage's current state to " + juce::String(currentState));

            switch (currentState)
            {
            case State::Init:
                break;

            case State::Drawing:
                for (auto it = regions.begin(); it != regions.end(); ++it)
                {
                    (*it)->setState(SegmentedRegion::State::Standby);
                }
                break;

            case State::Editing:
                for (auto it = regions.begin(); it != regions.end(); ++it)
                {
                    (*it)->setState(SegmentedRegion::State::Editing);
                }
                break;

            case State::Playing:
                for (auto it = regions.begin(); it != regions.end(); ++it)
                {
                    (*it)->setState(SegmentedRegion::State::Playable);
                }
                break;

            default:
                break;
            }

            resized();
        }
    }

    //to add point to path
    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::ImageComponent::mouseDown(event);
        DBG("Click!");
        juce::Point<float> clickPosition = event.position;

        if (currentState == State::Drawing && hasImage)
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
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override
    {
        DBG("key pressed");
        if (currentState == State::Drawing && hasImage)
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


    //================================================================

    /// <summary>
    /// 2D Array. Entry [x][y] states whether the pixel at position [x][y] of the image has been segmented yet
    /// </summary>
    //juce::uint8** segmentedPixels; //WIP: "Always prefer a juce::HeapBlock or some other container class." -> change it to that

    juce::OwnedArray<SegmentedRegion> regions;

private:
    State currentState;

    bool hasImage;

    bool drawsPath;
    juce::Path currentPath;
    juce::Array<juce::Point<float>> currentPathPoints; //WIP: normalise these to [0...1] so that, when resizing, the path can be redrawn

    AudioEngine* audioEngine;

    void resetPath()
    {
        clearPath();
        currentPathPoints.clear();
    }

    void tryCompletePath()
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

            SegmentedRegion* newRegion = new SegmentedRegion(currentPath, relativeBounds, audioEngine);
            newRegion->setBounds(currentPath.getBounds().toNearestInt());
            addRegion(newRegion);

            resetPath();
        }
    }

    void tryDeleteLastNode()
    {
        if (drawsPath)
        {
            clearPath();
            currentPathPoints.removeLast();

            if (!currentPathPoints.isEmpty())
            {
                for (auto pt : currentPathPoints)
                {
                    if (!drawsPath)
                    {
                        currentPath.startNewSubPath(pt);
                        drawsPath = true;
                    }
                    else
                        currentPath.lineTo(pt);
                }
                repaint(currentPath.getBounds().expanded(3.0f, 3.0f).toNearestInt());
            }
        }
    }

    void addRegion(SegmentedRegion* newRegion)
    {
        regions.add(newRegion);
        newRegion->setAlwaysOnTop(true);
        //newRegion->onStateChange += [newRegion] { newRegion->handleButtonStateChanged(); };
        addAndMakeVisible(newRegion);
    }
    void clearRegions()
    {
        for (auto it = regions.begin(); it != regions.end(); ++it)
        {
            removeChildComponent(*it);
        }
        regions.clear();
    }

    void clearPath()
    {
        auto formerArea = currentPath.getBounds().expanded(3.0f, 3.0f).toNearestInt();
        currentPath.clear();
        //currentPathPoints remain! if you want to remove them, use resetPath() instead.
        drawsPath = false;
        repaint(formerArea);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentableImage)
};
