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
    this->audioEngine = audioEngine;
    this->audioEngine->registerImage(this);

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
    if (getLocalBounds().contains(newPt.toInt()))
    {
        juce::Point<float> relativePt(newPt.getX() / static_cast<float>(getWidth()), newPt.getY() / static_cast<float>(getHeight()));

        //currentPath.startNewSubPath(newPt);
        currentPath.startNewSubPath(relativePt);
        //currentPathPoints.add(newPt);
        currentPathPoints.add(relativePt); //convert absolute coords to relative coords
        repaint(getAbsolutePathBounds().expanded(3.0f, 3.0f).toNearestInt());
        transitionToState(SegmentableImageStateIndex::drawingRegion);
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
    DBG("checking whether points actually form a 2D region...");
    //...
    DBG("done.");

    //finish path
    currentPath.closeSubPath();
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
    SegmentedRegion* newRegion = new SegmentedRegion(currentPath, relativeBounds, fillColour, audioEngine);
    newRegion->setBounds(getAbsolutePathBounds().toNearestInt());
    addRegion(newRegion);
    //newRegion->triggerButtonStateChanged(); //not necessary
    newRegion->triggerDrawableButtonStateChanged();
    resetPath();
    transitionToState(SegmentableImageStateIndex::withImage);
    DBG("new region has been added successfully.");

    audioEngine->suspendProcessing(false);
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
        transitionToState(SegmentableImageStateIndex::withImage);
    }
}

void SegmentableImage::clearRegions()
{
    for (auto it = regions.begin(); it != regions.end(); ++it)
    {
        removeChildComponent(*it);
    }
    audioEngine->suspendProcessing(true);
    regions.clear(true);
    audioEngine->resetRegionIDs();
    audioEngine->suspendProcessing(false);
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
            removeChildComponent(*it);
            audioEngine->suspendProcessing(true);
            regions.remove(i, true); //automatically removes voices, LFOs etc.
            audioEngine->suspendProcessing(false);

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

void SegmentableImage::serialise(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("serialising SegmentableImage...");

    juce::XmlElement* xmlSegmentableImage = xmlAudioEngine->createNewChildElement("SegmentableImage");

    if (getImage().isValid())
    {
        //serialise image
        juce::Image::BitmapData imageData = juce::Image::BitmapData(getImage(), juce::Image::BitmapData::ReadWriteMode::readOnly);
        juce::MemoryBlock imageMemory(imageData.size);
        juce::MemoryOutputStream imageStream = juce::MemoryOutputStream(imageMemory, false); //by using this stream, the pixels can be written in a certain endian (unlike memBuffer.append()), ensuring portability. little endian will be used.

        //prepend width, height and format of the image so that they can be read correctly
        imageStream.writeInt(static_cast<int>(imageData.pixelFormat));
        imageStream.writeInt(imageData.width);
        imageStream.writeInt(imageData.height);

        //copy the content of the image - unfortunately, the rows/columns can't be written as a whole, so it has to be done pixel-by-pixel...
        auto* pixels = imageData.data;
        for (int i = 0; i < imageData.width * imageData.height; ++i)
        {
            imageStream.writeByte(pixels[i]);
        }

        attachedData->add(imageMemory);
        xmlSegmentableImage->setAttribute("imageMemory_index", attachedData->size() - 1);
    }
    else
    {
        //image is empty
        xmlSegmentableImage->setAttribute("imageMemory_index", -1); //nothing attached
    }



    //serialise regions
    xmlSegmentableImage->setAttribute("regions_size", regions.size());
    int i = 0;
    for (auto* itRegion = regions.begin(); itRegion != regions.end(); ++itRegion, ++i)
    {
        juce::XmlElement* xmlRegion = xmlSegmentableImage->createNewChildElement("SegmentedRegion_" + juce::String(i));
        (*itRegion)->serialise(xmlRegion, attachedData);
    }

    DBG("SegmentableImage has been serialised.");
}
void SegmentableImage::deserialise(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("deserialising SegmentableImage...");

    juce::XmlElement* xmlSegmentableImage = xmlAudioEngine->getChildByName("SegmentableImage");

    int imageMemoryIndex = xmlSegmentableImage->getIntAttribute("imageMemory_index", -1);
    if (imageMemoryIndex >= 0 && imageMemoryIndex < attachedData->size())
    {
        //deserialise image
        juce::MemoryBlock imageMemory = (*attachedData)[imageMemoryIndex];
        juce::MemoryInputStream imageStream = juce::MemoryInputStream(imageMemory, false); //by using this stream, the pixels can be written in a certain endian (unlike memBuffer.append()), ensuring portability. little endian will be used.

        //initialise image. extract width, height and format so that the image can be reconstructed correctly
        juce::Image reconstructedImage = juce::Image(static_cast<juce::Image::PixelFormat>(imageStream.readInt()),
                                      imageStream.readInt(),
                                      imageStream.readInt(),
                                      false
                                     );
        juce::Image::BitmapData imageData = juce::Image::BitmapData(reconstructedImage, juce::Image::BitmapData::ReadWriteMode::writeOnly);

        //copy the content of the image - unfortunately, the rows/columns can't be written as a whole, so it has to be done pixel-by-pixel...
        auto* pixels = imageData.data;
        for (int i = 0; i < imageData.width * imageData.height; ++i)
        {
            pixels[i] = imageStream.readByte();
        }

        setImage(reconstructedImage);
    }
    else
    {
        //image data not contained in attachedData (image was probably empty)
    }

    //deserialise regions
    int size = xmlSegmentableImage->getIntAttribute("regions_size", 0);
    for (int i = 0; i < size; ++i)
    {
        juce::XmlElement* xmlRegion = xmlSegmentableImage->getChildByName("SegmentedRegion_" + juce::String(i));

        //generate new region
        SegmentedRegion* newRegion = new SegmentedRegion(juce::Path(), juce::Rectangle<float>(), juce::Colours::black, audioEngine);
        addRegion(newRegion);
        newRegion->triggerDrawableButtonStateChanged();

        //load data to that region
        newRegion->deserialise(xmlRegion, attachedData);
    }
    resized(); //updates all regions' sizes to fit this component

    DBG("SegmentableImage has been deserialised.");
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
    addAndMakeVisible(newRegion);
}

void SegmentableImage::clearPath()
{
    auto formerArea = getAbsolutePathBounds().expanded(3.0f, 3.0f).toNearestInt();
    currentPath.clear();
    //currentPathPoints remain! if you want to remove them, use resetPath() instead.
    repaint(formerArea);
}
