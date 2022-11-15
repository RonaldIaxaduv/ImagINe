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
    states[static_cast<int>(SegmentableImageStateIndex::drawingPlayPath)] = static_cast<SegmentableImageState*>(new SegmentableImageState_DrawingPlayPath(*this));
    states[static_cast<int>(SegmentableImageStateIndex::editingPlayPaths)] = static_cast<SegmentableImageState*>(new SegmentableImageState_EditingPlayPaths(*this));
    states[static_cast<int>(SegmentableImageStateIndex::playingPlayPaths)] = static_cast<SegmentableImageState*>(new SegmentableImageState_PlayingPlayPaths(*this));
    int initialisedStates = 8;
    jassert(initialisedStates == static_cast<int>(SegmentableImageStateIndex::StateIndexCount));
    
    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];

    //initialise remaining members
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
    delete states[static_cast<int>(SegmentableImageStateIndex::drawingPlayPath)];
    delete states[static_cast<int>(SegmentableImageStateIndex::editingPlayPaths)];
    delete states[static_cast<int>(SegmentableImageStateIndex::playingPlayPaths)];
    int deletedStates = 8;
    jassert(deletedStates == static_cast<int>(SegmentableImageStateIndex::StateIndexCount));

    resetPath();
    clearPlayPaths();
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
        //paint currently drawn path
        float width = static_cast<float>(getWidth());
        float height = static_cast<float>(getHeight());

        juce::Point<float> p1, p2;
        float dotRadius = 2.0f, lineThickness = 2.0f;

        p1 = currentPathPoints[0];
        p1.setXY(p1.getX() * width, p1.getY() * height); //convert relative coords to absolute coords
        g.setColour(juce::Colours::black);
        g.fillEllipse(p1.x - dotRadius, p1.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
        g.setColour(juce::Colours::gold);
        g.fillEllipse(p1.x - dotRadius * 0.75f, p1.y - dotRadius * 0.75f, dotRadius * 1.5f, dotRadius * 1.5f);

        for (int i = 1; i < currentPathPoints.size(); ++i)
        {
            p2 = currentPathPoints[i];
            p2.setXY(p2.getX() * width, p2.getY() * height); //convert relative coords to absolute coords
            g.setColour(juce::Colours::black);
            g.drawLine(juce::Line<float>(p1, p2), lineThickness);
            g.setColour(juce::Colours::gold);
            g.drawLine(juce::Line<float>(p1, p2), lineThickness * 0.5f);
            
            g.setColour(juce::Colours::black);
            g.fillEllipse(p2.x - dotRadius, p2.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
            g.setColour(juce::Colours::gold);
            g.fillEllipse(p2.x - dotRadius * 0.75f, p2.y - dotRadius * 0.75f, dotRadius * 1.5f, dotRadius * 1.5f);
            
            p1 = p2;
        }
    }

    //if (static_cast<int>(currentStateIndex) < static_cast<int>(SegmentableImageStateIndex::editingRegions))
    //{
    //    //repaintAllRegions();

    //    for (auto* itRegion = regions.begin(); itRegion != regions.end(); ++itRegion)
    //    {
    //        (*itRegion)->paintEntireComponent(g, false);
    //    }
    //}
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

    //resize play paths
    for (PlayPath* p : playPaths)
    {
        auto relativeBounds = p->relativeBounds;
        juce::Rectangle<float> newBounds(relativeBounds.getX() * curBounds.getWidth(),
            relativeBounds.getY() * curBounds.getHeight(),
            relativeBounds.getWidth() * curBounds.getWidth(),
            relativeBounds.getHeight() * curBounds.getHeight());
        p->setBounds(newBounds.toNearestInt());
    }

    //if (static_cast<int>(currentStateIndex) < static_cast<int>(SegmentableImageStateIndex::editingRegions))
    //{
    //    //repaintAllRegions();
    //}
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
            clearPlayPaths();
            clearRegions();
            nonInstantStateFound = true;
            DBG("segmentable image empty");
            break;

        case SegmentableImageStateIndex::withImage:
            if ((currentStateIndex == SegmentableImageStateIndex::drawingRegion || currentStateIndex == SegmentableImageStateIndex::drawingPlayPath) && currentPathPoints.size() > 0)
            {
                //currently drawing -> remain in that state
                return;
            }

            //make regions and play paths uninteractable
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->transitionToState(SegmentedRegionStateIndex::notInteractable);
            }
            for (auto it = playPaths.begin(); it != playPaths.end(); ++it)
            {
                (*it)->transitionToState(PlayPathStateIndex::notInteractable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image with image");
            break;




        case SegmentableImageStateIndex::drawingRegion:
            //make regions and play paths uninteractable, but bring regions to front
            for (auto it = playPaths.begin(); it != playPaths.end(); ++it)
            {
                (*it)->setAlwaysOnTop(false);
                (*it)->transitionToState(PlayPathStateIndex::notInteractable);
            }
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->setAlwaysOnTop(true);
                (*it)->transitionToState(SegmentedRegionStateIndex::notInteractable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image drawing region");
            break;

        case SegmentableImageStateIndex::editingRegions:
            //clear unfinished paths
            resetPath();

            //set regions to editable at front, play paths to not interactable at back.
            for (auto it = playPaths.begin(); it != playPaths.end(); ++it)
            {
                (*it)->setAlwaysOnTop(false);
                (*it)->transitionToState(PlayPathStateIndex::notInteractable, true);
            }
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->setAlwaysOnTop(true);
                (*it)->transitionToState(SegmentedRegionStateIndex::editable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image editing regions");
            break;

        case SegmentableImageStateIndex::playingRegions:
            //clear unfinished paths
            resetPath();

            //set regions to playable at front, play paths to not interactable at back
            for (auto it = playPaths.begin(); it != playPaths.end(); ++it)
            {
                (*it)->setAlwaysOnTop(false);
                (*it)->transitionToState(PlayPathStateIndex::notInteractable, true);
            }
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->setAlwaysOnTop(true);
                (*it)->transitionToState(SegmentedRegionStateIndex::playable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image playing regions");
            break;




        case SegmentableImageStateIndex::drawingPlayPath:
            //make regions and play paths uninteractable, but bring play paths to front
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->setAlwaysOnTop(false);
                (*it)->transitionToState(SegmentedRegionStateIndex::notInteractable);
            }
            for (auto it = playPaths.begin(); it != playPaths.end(); ++it)
            {
                (*it)->setAlwaysOnTop(true);
                (*it)->transitionToState(PlayPathStateIndex::notInteractable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image drawing play paths");
            break;

        case SegmentableImageStateIndex::editingPlayPaths:
            //clear unfinished paths
            resetPath();

            //set play paths to editable at front, regions to not interactable at back
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->setAlwaysOnTop(false);
                (*it)->transitionToState(SegmentedRegionStateIndex::notInteractable, true);
            }
            for (auto it = playPaths.begin(); it != playPaths.end(); ++it)
            {
                (*it)->setAlwaysOnTop(true);
                (*it)->transitionToState(PlayPathStateIndex::editable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image editing play paths");
            break;

        case SegmentableImageStateIndex::playingPlayPaths:
            //clear unfinished paths
            resetPath();

            //set play paths to playable at front, regions to not interactable at back
            for (auto it = regions.begin(); it != regions.end(); ++it)
            {
                (*it)->setAlwaysOnTop(false);
                (*it)->transitionToState(SegmentedRegionStateIndex::notInteractable, true);
            }
            for (auto it = playPaths.begin(); it != playPaths.end(); ++it)
            {
                (*it)->setAlwaysOnTop(true);
                (*it)->transitionToState(PlayPathStateIndex::playable);
            }
            nonInstantStateFound = true;
            DBG("segmentable image playing play paths");
            break;




        default:
            throw std::exception("unhandled state index");
        }
    }

    //finally, update the current state index
    currentStateIndex = stateToTransitionTo;
    currentState = states[static_cast<int>(currentStateIndex)];

    if (currentStateIndex == SegmentableImageStateIndex::empty)
    {
        setImage(juce::Image());
    }

    //repaint();
}
SegmentableImageStateIndex SegmentableImage::getCurrentStateIndex()
{
    return currentStateIndex;
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
    
    if (newImage.isValid() || currentStateIndex != SegmentableImageStateIndex::empty)
    {
        currentState->imageChanged(newImage);
    }
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

void SegmentableImage::startNewPath(juce::Point<float> newPt)
{
    if (getLocalBounds().contains(newPt.toInt()))
    {
        juce::Point<float> relativePt(newPt.getX() / static_cast<float>(getWidth()), newPt.getY() / static_cast<float>(getHeight()));

        currentPath.startNewSubPath(relativePt);
        currentPathPoints.add(relativePt); //convert absolute coords to relative coords
        repaint(getAbsolutePathBounds().expanded(3.0f, 3.0f).toNearestInt());
        //transitionToState(SegmentableImageStateIndex::drawingRegion);
    }
}
void SegmentableImage::addPointToPath(juce::Point<float> newPt)
{
    if (getLocalBounds().contains(newPt.toInt()))
    {
        juce::Point<float> relativePt(newPt.getX() / static_cast<float>(getWidth()), newPt.getY() / static_cast<float>(getHeight()));

        //currentPath.lineTo(newPt);
        currentPath.lineTo(relativePt);
        //currentPathPoints.add(newPt);
        currentPathPoints.add(juce::Point<float>(newPt.getX() / static_cast<float>(getWidth()), newPt.getY() / static_cast<float>(getHeight()))); //convert absolute coords to relative coords
        repaint(getAbsolutePathBounds().expanded(3.0f, 3.0f).toNearestInt());
    }
}

bool SegmentableImage::hasStartedDrawing()
{
    return currentPathPoints.size() > 0;
}
void SegmentableImage::resetPath()
{
    clearPath();
    currentPathPoints.clear();
}
void SegmentableImage::tryCompletePath_Region()
{
    //check whether there are enough points to form a 2D region
    if (currentPathPoints.size() < 3)
    {
        DBG("not enough points to form a region.");
        return;
    }

    //WIP: check whether all points jus form a line, or whether they actually define a 2D region
    DBG("checking whether points actually form a 2D region...");
    //...
    DBG("done.");

    //finish path
    currentPath.closeSubPath();
    bool previouslySuspended = audioEngine->isSuspended();
    audioEngine->suspendProcessing(true);

    //calculate bounds
    DBG("calculating region's bounds...");
    juce::Rectangle<float> origRegionBounds(getAbsolutePathBounds());
    juce::Rectangle<float> origParentBounds(getBounds().toFloat());
    juce::Rectangle<float> relativeBounds(origRegionBounds.getX() / origParentBounds.getWidth(),
        origRegionBounds.getY() / origParentBounds.getHeight(),
        origRegionBounds.getWidth() / origParentBounds.getWidth(),
        origRegionBounds.getHeight() / origParentBounds.getHeight());
    DBG("done.");

    //calculate colour
    DBG("calculating region's colour...");
    //juce::Random rng;
    //juce::Colour fillColour = juce::Colour::fromHSV(rng.nextFloat(), 0.6f + 0.4f * rng.nextFloat(), 0.6f + 0.4f * rng.nextFloat(), 1.0f); //random for now

    //calculate histogram of the pixels within the path (the commented out version's calculations are more exact (more colours aggregated), but a lot slower)
    juce::Image associatedImage = getImage();
    juce::HashMap<juce::String, int> colourHistogram;
    //float xMin = relativeBounds.getX() * associatedImage.getWidth();
    //float xMax = (relativeBounds.getX() + relativeBounds.getWidth()) * associatedImage.getWidth();
    //float yMin = relativeBounds.getY() * associatedImage.getHeight();
    //float yMax = (relativeBounds.getY() + relativeBounds.getHeight()) * associatedImage.getHeight();
    //float scaleX = origParentBounds.getWidth() / static_cast<float>(associatedImage.getWidth());
    //float scaleY = origParentBounds.getHeight() / static_cast<float>(associatedImage.getHeight());
    float xMin = origRegionBounds.getX();
    float xMax = origRegionBounds.getX() + origRegionBounds.getWidth();
    float yMin = origRegionBounds.getY();
    float yMax = origRegionBounds.getY() + origRegionBounds.getHeight();
    float scaleX = static_cast<float>(associatedImage.getWidth()) / origParentBounds.getWidth();
    float scaleY = static_cast<float>(associatedImage.getHeight()) / origParentBounds.getHeight();
    float widthDenom = 1.0f / static_cast<float>(getWidth());
    float heightDenom = 1.0f / static_cast<float>(getHeight());
    DBG("image search region: x[" + juce::String(xMin) + ", " + juce::String(xMax) + "], y[" + juce::String(yMin) + ", " + juce::String(yMax) + "]");
    DBG("path search region: x[" + juce::String(xMin * scaleX) + ", " + juce::String(xMax * scaleX) + "], y[" + juce::String(yMin * scaleY) + ", " + juce::String(yMax * scaleY) + "]");
    for (float x = xMin; x <= xMax; ++x)
    {
        for (float y = yMin; y <= yMax; ++y)
        {
            //check whether the point is actually contained within the path (origParentBounds is only the largest rectangle containing all points in the path!)
            if (currentPath.contains(x * widthDenom, y * heightDenom)) //(currentPath.contains(x * scaleX, y * scaleY))
            {
                //contained! -> add pixel to the histogram
                //juce::Colour nextColour = associatedImage.getPixelAt(static_cast<int>(x), static_cast<int>(y));
                juce::Colour nextColour = associatedImage.getPixelAt(static_cast<int>(x * scaleX), static_cast<int>(y * scaleY));
                if (!colourHistogram.contains(nextColour.toString()))
                {
                    //new colour -> add to histogram
                    colourHistogram.set(nextColour.toString(), 1);
                }
                else
                {
                    //known colour -> increment in histogram
                    colourHistogram.set(nextColour.toString(), colourHistogram[nextColour.toString()] + 1);
                }
            }
        }
    }
    DBG("histogram contains " + juce::String(colourHistogram.size()) + " entries.");

    //calculate (squared) distance of every contained colour to the average colour
    juce::Array<juce::Colour> coloursSorted;
    juce::Array<double> distancesSorted; //in ascending order
    for (juce::HashMap<juce::String, int>::Iterator it(colourHistogram); it.next();)
    {
        //calculate (squared) distance to the average colour
        juce::Colour c = juce::Colour::fromString(it.getKey());
        //double distanceSq = static_cast<double>(c.getRed() - averageColour.getRed()) * static_cast<double>(c.getRed() - averageColour.getRed()) +
        //                    static_cast<double>(c.getGreen() - averageColour.getGreen()) * static_cast<double>(c.getGreen() - averageColour.getGreen()) +
        //                    static_cast<double>(c.getBlue() - averageColour.getBlue()) * static_cast<double>(c.getBlue() - averageColour.getBlue());

        //calculate (squared) distance from (0,0,0)
        double distanceSq = static_cast<double>(c.getRed()) * static_cast<double>(c.getRed()) +
            static_cast<double>(c.getGreen()) * static_cast<double>(c.getGreen()) +
            static_cast<double>(c.getBlue()) * static_cast<double>(c.getBlue());

        //insert into the lists according to the distance
        int indexToInsertAt = 0;
        for (auto itDist = distancesSorted.begin(); itDist != distancesSorted.end(); itDist++)
        {
            if (distanceSq < *itDist)
            {
                //next index is larger -> insert here
                break;
            }
        }
        for (int i = 0; i < it.getValue(); ++i) //insert as many times as the pixel occurred in the image
        {
            coloursSorted.insert(indexToInsertAt, c);
            distancesSorted.insert(indexToInsertAt, distanceSq);
        }
    }
    juce::Colour medianColour = coloursSorted[coloursSorted.size() / 2]; //pick median entry -> colour that's actually contained within the region (in contrast to the average colour) -> should feel pretty close to the perceived average colour
    coloursSorted.clear();
    distancesSorted.clear();
    colourHistogram.clear();

    juce::Colour fillColour = medianColour;
    DBG("done. median colour is: " + juce::String(fillColour.getRed()) + ", " + juce::String(fillColour.getGreen()) + ", " + juce::String(fillColour.getBlue()));

    DBG("adding new region...");
    SegmentedRegion* newRegion = new SegmentedRegion(currentPath, relativeBounds, getBounds(), fillColour, audioEngine);
    newRegion->setBounds(getAbsolutePathBounds().toNearestInt());

    //calculate intersections with play paths
    for (auto itPath = playPaths.begin(); itPath != playPaths.end(); ++itPath)
    {
        if (newRegion->getBounds().intersects((*itPath)->getBounds()))
        {
            (*itPath)->addIntersectingRegion(newRegion);
        }
    }

    addRegion(newRegion);
    //newRegion->triggerButtonStateChanged(); //not necessary
    newRegion->triggerDrawableButtonStateChanged();

    resetPath();
    transitionToState(SegmentableImageStateIndex::drawingRegion);
    DBG("new region has been added successfully.");

    audioEngine->suspendProcessing(previouslySuspended);
}
void SegmentableImage::tryCompletePath_PlayPath()
{
    //check whether there are enough points to form a 2D region
    if (currentPathPoints.size() < 3)
    {
        DBG("not enough points to form a play path.");
        return;
    }

    //WIP: check whether all points jus form a line, or whether they actually define a 2D region
    DBG("checking whether points actually form a 2D area...");
    //...
    DBG("done.");

    //finish path
    currentPath.closeSubPath();
    bool previouslySuspended = audioEngine->isSuspended();
    audioEngine->suspendProcessing(true);

    //calculate bounds
    DBG("calculating play path's bounds...");
    juce::Rectangle<float> origRegionBounds(getAbsolutePathBounds());
    juce::Rectangle<float> origParentBounds(getBounds().toFloat());
    juce::Rectangle<float> relativeBounds(origRegionBounds.getX() / origParentBounds.getWidth(),
                                          origRegionBounds.getY() / origParentBounds.getHeight(),
                                          origRegionBounds.getWidth() / origParentBounds.getWidth(),
                                          origRegionBounds.getHeight() / origParentBounds.getHeight());
    DBG("done.");

    //calculate colour
    DBG("generating random path colour...");
    juce::Random rng;
    juce::Colour fillColour = juce::Colour::fromHSV(rng.nextFloat(), 0.6f + 0.4f * rng.nextFloat(), 0.6f + 0.4f * rng.nextFloat(), 1.0f); //random for now
    DBG("done. path colour is: " + juce::String(fillColour.getRed()) + ", " + juce::String(fillColour.getGreen()) + ", " + juce::String(fillColour.getBlue()));

    DBG("adding new play path...");
    PlayPath* newPlayPath = new PlayPath(getNextPlayPathID(), currentPath, relativeBounds, getBounds(), fillColour);
    newPlayPath->setBounds(getAbsolutePathBounds().toNearestInt());
    
    //calculate intersections with regions
    for (auto itRegion = regions.begin(); itRegion != regions.end(); ++itRegion)
    {
        if ((*itRegion)->getBounds().intersects(newPlayPath->getBounds()))
        {
            newPlayPath->addIntersectingRegion(*itRegion);
        }
    }

    addPlayPath(newPlayPath);
    //newPlayPath->triggerButtonStateChanged(); //not necessary
    newPlayPath->triggerDrawableButtonStateChanged();

    resetPath();
    transitionToState(SegmentableImageStateIndex::drawingPlayPath);
    DBG("new play path has been added successfully.");

    audioEngine->suspendProcessing(previouslySuspended);
}
void SegmentableImage::deleteLastNode()
{
    clearPath(); //erases the whole Path object (will be reconstructed later)
    currentPathPoints.removeLast();

    if (!currentPathPoints.isEmpty())
    {
        //reconstruct path object up to the previously second last point
        int i = 0;
        float width = static_cast<float>(getWidth());
        float height = static_cast<float>(getHeight());
        for (auto itPt = currentPathPoints.begin(); itPt != currentPathPoints.end(); itPt++, i++)
        {
            //juce::Point<float> absolutePt = juce::Point<float>(itPt->getX() * width, itPt->getY() * height); //convert back to absolute coords
            if (i == 0)
            {
                currentPath.startNewSubPath(*itPt);
                //currentPath.startNewSubPath(absolutePt);
            }
            else
            {
                currentPath.lineTo(*itPt);
                //currentPath.lineTo(absolutePt);
            }
        }
        repaint(getAbsolutePathBounds().expanded(3.0f, 3.0f).toNearestInt());
    }
    else //currentPathPoints.isEmpty()
    {
        //transitionToState(SegmentableImageStateIndex::withImage);
        //^- better stay in drawing mode
    }
}

void SegmentableImage::clearRegions()
{
    bool previouslySuspended = audioEngine->isSuspended();
    audioEngine->suspendProcessing(true);

    for (auto it = regions.begin(); it != regions.end(); ++it)
    {
        removeChildComponent(*it);
        for (auto itPath = playPaths.begin(); itPath != playPaths.end(); ++itPath)
        {
            (*itPath)->removeIntersectingRegion((*it)->getID()); //play paths must forget about this region as well, otherwise there will be dangling pointers!
        }
        audioEngine->removeRegion(*it);
    }
    regions.clear(true);
    
    audioEngine->resetRegionIDs();
    
    audioEngine->suspendProcessing(previouslySuspended);
}
void SegmentableImage::removeRegion(int regionID)
{
    int i = 0;
    bool regionRemoved = false;
    for (auto it = regions.begin(); it != regions.end(); ++it, ++i)
    {
        if ((*it)->getID() == regionID)
        {
            //found the region -> remove
            bool previouslySuspended = audioEngine->isSuspended();
            audioEngine->suspendProcessing(true);

            removeChildComponent(*it);
            for (auto itPath = playPaths.begin(); itPath != playPaths.end(); ++itPath)
            {
                (*itPath)->removeIntersectingRegion((*it)->getID()); //play paths must forget about this region as well, otherwise there will be dangling pointers!
            }
            audioEngine->removeRegion(*it);
            regions.remove(i, true); //automatically removes voices, LFOs etc.
            audioEngine->suspendProcessing(previouslySuspended);

            regionRemoved = true;
            break; //no two regions with the same ID
        }
    }

    if (!regionRemoved)
    {
        return;
    }

    //if a region has been deleted, tell all other regions to update their region editors, if they have that window currently open
    for (auto it = regions.begin(); it != regions.end(); ++it, ++i)
    {
        (*it)->refreshEditor();
    }

    if (regions.size() == 0)
    {
        //last region deleted -> reset engine's region counter
        audioEngine->resetRegionIDs();
    }
}

void SegmentableImage::clearPlayPaths()
{
    for (auto itPath = playPaths.begin(); itPath != playPaths.end(); ++itPath)
    {
        removeChildComponent(*itPath);
    }
    playPaths.clear(true);
    playPathIdCounter = -1;

    //remove any lingering play path couriers
    for (auto itRegion = regions.begin(); itRegion != regions.end(); ++itRegion)
    {
        (*itRegion)->resetCouriers();
    }
}
void SegmentableImage::removePlayPath(int pathID)
{
    int i = 0;
    for (auto itPath = playPaths.begin(); itPath != playPaths.end(); ++itPath, ++i)
    {
        if ((*itPath)->getID() == pathID)
        {
            //path found -> remove
            removeChildComponent(*itPath);
            playPaths.remove(i);
            break; //IDs are unique
        }
    }

    if (playPaths.size() == 0)
    {
        //reset ID counter
        playPathIdCounter = -1;
    }
}

bool SegmentableImage::serialise(juce::XmlElement* xmlParent, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("serialising SegmentableImage...");
    bool serialisationSuccessful = true;

    juce::XmlElement* xmlSegmentableImage = xmlParent->createNewChildElement("SegmentableImage");

    //define state that the image should start up with (that state will also determine the state that the editor will start up in)
    xmlSegmentableImage->setAttribute("targetStateIndex", static_cast<int>(getTargetStateIndex()));
    xmlSegmentableImage->setAttribute("playPathIdCounter", playPathIdCounter);

    if (getImage().isValid())
    {
        //serialise image
        juce::Image::BitmapData imageData (getImage(), juce::Image::BitmapData::ReadWriteMode::readOnly);
        juce::MemoryBlock imageMemory(imageData.size);
        juce::MemoryOutputStream imageStream (imageMemory, false); //by using this stream, the pixels can be written in a certain endian (unlike memBuffer.append()), ensuring portability. little endian will be used.

        //prepend width, height and format of the image so that they can be read correctly
        imageStream.writeInt(static_cast<int>(getImage().getFormat()));
        imageStream.writeInt(imageData.width);
        imageStream.writeInt(imageData.height);
        imageStream.flush();

        //copy the content of the image - unfortunately, the rows/columns can't be written as a whole, so it has to be done pixel-by-pixel...
        auto* pixels = imageData.data;
        int totalPixelBytes = imageData.width * imageData.height * imageData.pixelStride;
        for (int i = 0; i < totalPixelBytes; ++i)
        {
            imageStream.writeByte(pixels[i]);
        }
        imageStream.flush();

        attachedData->add(imageMemory);
        xmlSegmentableImage->setAttribute("imageMemory_index", attachedData->size() - 1);

        //serialise regions
        xmlSegmentableImage->setAttribute("regions_size", regions.size());
        int i = 0;
        for (auto* itRegion = regions.begin(); serialisationSuccessful && itRegion != regions.end(); ++itRegion, ++i)
        {
            juce::XmlElement* xmlRegion = xmlSegmentableImage->createNewChildElement("SegmentedRegion_" + juce::String(i));
            serialisationSuccessful = (*itRegion)->serialise(xmlRegion, attachedData);
        }

        if (serialisationSuccessful)
        {
            //serialise play paths
            xmlSegmentableImage->setAttribute("playPaths_size", playPaths.size());
            i = 0;
            for (auto* itPath = playPaths.begin(); serialisationSuccessful && itPath != playPaths.end(); ++itPath, ++i)
            {
                juce::XmlElement* xmlPlayPath = xmlSegmentableImage->createNewChildElement("PlayPath_" + juce::String(i));
                serialisationSuccessful = (*itPath)->serialise(xmlPlayPath);
            }
        }
    }
    else
    {
        //image is empty
        xmlSegmentableImage->setAttribute("imageMemory_index", -1); //nothing attached

        //no regions/playpaths to serialise on an empty image
        xmlSegmentableImage->setAttribute("regions_size", 0);
        xmlSegmentableImage->setAttribute("playPaths_size", 0);
    }

    DBG(juce::String(serialisationSuccessful ? "SegmentableImage has been serialised." : "SegmentableImage could not be serialised."));
    return serialisationSuccessful;
}
bool SegmentableImage::deserialise(juce::XmlElement* xmlParent, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("deserialising SegmentableImage...");
    bool deserialisationSuccessful = true;

    juce::XmlElement* xmlSegmentableImage = xmlParent->getChildByName("SegmentableImage");

    if (xmlSegmentableImage != nullptr)
    {
        int imageMemoryIndex = xmlSegmentableImage->getIntAttribute("imageMemory_index", -1);
        if (imageMemoryIndex >= 0 && imageMemoryIndex < attachedData->size())
        {
            //deserialise image
            juce::MemoryBlock imageMemory = (*attachedData)[imageMemoryIndex];
            juce::MemoryInputStream imageStream (imageMemory, false); //by using this stream, the pixels can be written in a certain endian (unlike memBuffer.append()), ensuring portability. little endian will be used.

            //initialise image. extract width, height and format so that the image can be reconstructed correctly
            juce::Image::PixelFormat format = static_cast<juce::Image::PixelFormat>(imageStream.readInt());
            int width = imageStream.readInt();
            int height = imageStream.readInt();
            juce::Image reconstructedImage = juce::Image(format, width, height, false);
            juce::Image::BitmapData imageData (reconstructedImage, juce::Image::BitmapData::ReadWriteMode::writeOnly);

            //copy the content of the image - unfortunately, the rows/columns can't be written as a whole, so it has to be done pixel-by-pixel...
            auto* pixels = imageData.data;
            int totalPixelBytes = imageData.width * imageData.height * imageData.pixelStride;
            for (int i = 0; i < totalPixelBytes; ++i)
            {
                pixels[i] = static_cast<juce::uint8>(imageStream.readByte());
            }

            //delete imageData; //apparently it's necessary to delete a BitmapData object before it updates the pixel data in the image
            setImage(reconstructedImage);

            //set component's size to correct aspect ratio (required for a few things, e.g. for the play paths to reconstruct missing ranges correctly - don't ask...)
            if (getParentComponent() != nullptr)
            {
                getParentComponent()->resized(); //adjust this components shape to that of the window (depends on the image's aspect ratio, which most likely changed). also updates all regions' and play paths' sizes to fit this component
            }
            else
            {
                setSize(500, static_cast<int>(500.0f * static_cast<float>(reconstructedImage.getPixelData()->height) / static_cast<float>(reconstructedImage.getPixelData()->width)));
            }

            //deserialise regions
            clearRegions();
            int size = xmlSegmentableImage->getIntAttribute("regions_size", 0);
            for (int i = 0; deserialisationSuccessful && i < size; ++i)
            {
                //generate new region
                SegmentedRegion* newRegion = new SegmentedRegion(juce::Path(), juce::Rectangle<float>(), getBounds(), juce::Colours::black, audioEngine);
                addRegion(newRegion);
                newRegion->triggerDrawableButtonStateChanged();

                juce::XmlElement* xmlRegion = xmlSegmentableImage->getChildByName("SegmentedRegion_" + juce::String(i));
                if (xmlRegion != nullptr)
                {
                    //load data to that region
                    deserialisationSuccessful = newRegion->deserialise(xmlRegion, attachedData);
                }
                else
                {
                    DBG("data of the region at the array index " + juce::String(i) + " could not be found.");
                    //deserialisationSuccessful = false; //not problematic
                }
            }

            if (deserialisationSuccessful)
            {
                resized(); //adjust regions to the segmentable image's bounds

                //deserialise play paths
                clearPlayPaths(); //caution: this also reset the playPathIdCounter!
                size = xmlSegmentableImage->getIntAttribute("playPaths_size", 0);
                for (int i = 0; deserialisationSuccessful && i < size; ++i)
                {
                    //generate new play path
                    PlayPath* newPlayPath = new PlayPath(-1, juce::Path(), juce::Rectangle<float>(), getBounds(), juce::Colours::black);
                    addPlayPath(newPlayPath);
                    newPlayPath->triggerDrawableButtonStateChanged();

                    juce::XmlElement* xmlPlayPath = xmlSegmentableImage->getChildByName("PlayPath_" + juce::String(i));
                    if (xmlPlayPath != nullptr)
                    {
                        //load data to that play path
                        deserialisationSuccessful = newPlayPath->deserialise(xmlPlayPath);
                    }
                    else
                    {
                        DBG("data of the play path at the array index " + juce::String(i) + " could not be found.");
                        //deserialisationSuccessful = false; //not problematic
                    }
                }

                playPathIdCounter = xmlSegmentableImage->getIntAttribute("playPathIdCounter", -1);
                if (playPathIdCounter < 0 && playPaths.size() > 0)
                {
                    //error in stored playPathIdCounter -> set to highest current path ID
                    int maxID = -1;
                    for (auto itPath = playPaths.begin(); itPath != playPaths.end(); ++itPath)
                    {
                        maxID = juce::jmax((*itPath)->getID(), maxID);
                    }
                    playPathIdCounter = maxID;
                }
            }

            resized(); //adjust play paths' bounds to the segmentable image's bounds
            juce::Timer::callAfterDelay(100, [this] { repaintAllRegions(); repaintAllPlayPaths(); }); //WIP: this is kinda messy, but at least it works. if called without a delay, the regions won't correctly repaint themselves, causing their lfoLines to be in the wrong position until one hovers over them or clicks them
        }
        else
        {
            //image data not contained in attachedData
            if (static_cast<SegmentableImageStateIndex>(xmlSegmentableImage->getIntAttribute("targetStateIndex", static_cast<int>(SegmentableImageStateIndex::StateIndexCount))) == SegmentableImageStateIndex::empty && xmlSegmentableImage->getIntAttribute("regions_size", 0) == 0)
            {
                //target state was empty and no regions have been stored -> seems like a normal empty image
                DBG("image not contained in attachedData. this is most likely because the image was originally empty.");
            }
            else
            {
                //no image data, but either the target state required an image, or region data has been stored (which would also have required an image)!
                DBG("image not contained in attachedData. it seems like this should *not* have been the case, though!");
                deserialisationSuccessful = false;
            }
        }
    }
    else
    {
        DBG("no SegmentableImage data found.");
        deserialisationSuccessful = false;
    }

    if (!deserialisationSuccessful)
    {
        transitionToState(SegmentableImageStateIndex::empty);
    }
    else
    {
        SegmentableImageStateIndex targetStateIndex = SegmentableImageStateIndex::StateIndexCount;
        if (xmlSegmentableImage->hasAttribute("targetStateIndex"))
        {
            //transition to the intended state of the image
            targetStateIndex = static_cast<SegmentableImageStateIndex>(xmlSegmentableImage->getIntAttribute("targetStateIndex", static_cast<int>(SegmentableImageStateIndex::empty)));
        }
        else
        {
            //targetStateIndex not contained -> not a problem, just automatically determine the best target state for the image
            targetStateIndex = getTargetStateIndex();
        }
        transitionToState(targetStateIndex);
    }

    DBG(juce::String(deserialisationSuccessful ? "SegmentableImage has been deserialised." : "SegmentableImage could not be deserialised."));
    return deserialisationSuccessful;
}




bool SegmentableImage::hasAtLeastOneRegion()
{
    return regions.size() > 0;
}
bool SegmentableImage::hasAtLeastOneRegionWithAudio()
{
    bool regionWithAudioFound = false;

    for (auto* itRegion = regions.begin(); itRegion != regions.end(); ++itRegion)
    {
        if ((*itRegion)->getFileName() != "")
        {
            regionWithAudioFound = true;
            break;
        }
    }

    return regionWithAudioFound;
}

int SegmentableImage::getNextPlayPathID()
{
    return ++playPathIdCounter;
}
int SegmentableImage::getLastPlayPathID()
{
    return playPathIdCounter;
}
bool SegmentableImage::hasAtLeastOnePlayPath()
{
    return playPaths.size() > 0;
}




SegmentableImageStateIndex SegmentableImage::getTargetStateIndex()
{
    SegmentableImageStateIndex targetStateIndex = SegmentableImageStateIndex::StateIndexCount;
    if (getImage().isNull())
    {
        targetStateIndex = SegmentableImageStateIndex::empty;
    }
    else
    {
        //valid image has been set
        if (!hasAtLeastOneRegion())
        {
            targetStateIndex = SegmentableImageStateIndex::drawingRegion; //will set the editor to its Drawing state
        }
        else
        {
            //at least one region has been set
            if (!hasAtLeastOneRegionWithAudio())
            {
                targetStateIndex = SegmentableImageStateIndex::editingRegions;
            }
            else
            {
                //at least one region has audio
                if (!hasAtLeastOnePlayPath())
                {
                    targetStateIndex = SegmentableImageStateIndex::playingRegions;
                }
                else
                {
                    //at least one play path has been defined
                    targetStateIndex = SegmentableImageStateIndex::playingPlayPaths;
                }
            }
        }
    }

    return targetStateIndex;
}

juce::Rectangle<float> SegmentableImage::getAbsolutePathBounds()
{
    juce::Rectangle<float> relativeBounds = currentPath.getBounds();

    return juce::Rectangle<float>()
        .withX(relativeBounds.getX() * static_cast<float>(getWidth()))
        .withY(relativeBounds.getY() * static_cast<float>(getHeight()))
        .withWidth(relativeBounds.getWidth() * static_cast<float>(getWidth()))
        .withHeight(relativeBounds.getHeight() * static_cast<float>(getHeight()));;
}

void SegmentableImage::addRegion(SegmentedRegion* newRegion)
{
    regions.add(newRegion);

    newRegion->setAlwaysOnTop(true);
    switch (currentStateIndex)
    {
    case SegmentableImageStateIndex::editingRegions:
        newRegion->transitionToState(SegmentedRegionStateIndex::editable);
        break;

    case SegmentableImageStateIndex::playingRegions:
        newRegion->transitionToState(SegmentedRegionStateIndex::playable);
        break;

    default:
        newRegion->transitionToState(SegmentedRegionStateIndex::notInteractable);
        break;
    }

    addAndMakeVisible(newRegion);
}
void SegmentableImage::repaintAllRegions()
{
    for (auto* itRegion = regions.begin(); itRegion != regions.end(); ++itRegion)
    {
        (*itRegion)->triggerDrawableButtonStateChanged(); //this makes the button redraw its background
        (*itRegion)->repaint();
    }
}

void SegmentableImage::addPlayPath(PlayPath* newPlayPath)
{
    playPaths.add(newPlayPath);

    newPlayPath->setAlwaysOnTop(true);
    switch (currentStateIndex)
    {
    case SegmentableImageStateIndex::editingPlayPaths:
        newPlayPath->transitionToState(PlayPathStateIndex::editable);
        break;

    case SegmentableImageStateIndex::playingRegions:
        newPlayPath->transitionToState(PlayPathStateIndex::playable);
        break;

    default:
        newPlayPath->transitionToState(PlayPathStateIndex::notInteractable);
        break;
    }

    addAndMakeVisible(newPlayPath);
}
void SegmentableImage::repaintAllPlayPaths()
{
    for (auto* itPath = playPaths.begin(); itPath != playPaths.end(); ++itPath)
    {
        (*itPath)->triggerDrawableButtonStateChanged(); //this makes the button redraw its background
        (*itPath)->repaint();
    }
}

void SegmentableImage::clearPath()
{
    auto formerArea = getAbsolutePathBounds().expanded(3.0f, 3.0f).toNearestInt();
    currentPath.clear();
    //currentPathPoints remain! if you want to remove them, use resetPath() instead.
    repaint(formerArea);
}
