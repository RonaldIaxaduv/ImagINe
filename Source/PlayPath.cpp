/*
  ==============================================================================

    PlayPath.cpp
    Created: 7 Nov 2022 8:54:08am
    Author:  Aaron

  ==============================================================================
*/

#include "PlayPath.h"
#include "PlayPathStates.h"

#include "SegmentableImage.h"

//constants
const float PlayPath::outlineThickness = 3.5f;




PlayPath::PlayPath(int ID, const juce::Path& path, const juce::Rectangle<float>& relativeBounds, const juce::Rectangle<int>& parentBounds, juce::Colour fillColour) :
    DrawableButton::DrawableButton("", juce::DrawableButton::ButtonStyle::ImageStretched),
    underlyingPath(path),
    relativeBounds(relativeBounds),
    fillColour(fillColour)
{
    this->ID = ID;

    //initialiseStates
    states[static_cast<int>(PlayPathStateIndex::notInteractable)] = static_cast<PlayPathState*>(new PlayPathState_NotInteractable(*this));
    states[static_cast<int>(PlayPathStateIndex::editable)] = static_cast<PlayPathState*>(new PlayPathState_Editable(*this));
    states[static_cast<int>(PlayPathStateIndex::playable)] = static_cast<PlayPathState*>(new PlayPathState_Playable(*this));
    int initialisedStates = 3;
    jassert(initialisedStates == static_cast<int>(PlayPathStateIndex::StateIndexCount));

    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];

    //initialise images
    fillColourOn = fillColour.darker(0.2f); //default value for the colour when playing: just a little darker
    initialiseImages();

    //initialise the first courier(s)
    isPlaying = true; //needs to be set for the line below to work
    stopPlaying(); //this also prepares the couriers that will be played next in advance

    //rest
    setToggleable(true);
    setBufferedToImage(true);
    setMouseClickGrabsKeyboardFocus(false);
    setSize(parentBounds.getWidth(), parentBounds.getHeight());
}
PlayPath::~PlayPath()
{
    if (pathEditorWindow != nullptr) //is editor currently opened?
    {
        pathEditorWindow.deleteAndZero(); //close and release
    }

    stopPlaying();

    //release couriers
    for (auto* itCourier = couriers.begin(); itCourier != couriers.end(); ++itCourier)
    {
        removeChildComponent(*itCourier);
    }
    couriers.clear(true);

    regionsByRange_region.clear();
    regionsByRange_range.clear();

    //release states
    states[static_cast<int>(PlayPathStateIndex::notInteractable)] = static_cast<PlayPathState*>(new PlayPathState_NotInteractable(*this));
    states[static_cast<int>(PlayPathStateIndex::editable)] = static_cast<PlayPathState*>(new PlayPathState_Editable(*this));
    states[static_cast<int>(PlayPathStateIndex::playable)] = static_cast<PlayPathState*>(new PlayPathState_Playable(*this));
    int deletedStates = 3;
    jassert(deletedStates == static_cast<int>(PlayPathStateIndex::StateIndexCount));
}

void PlayPath::initialiseImages()
{
    if (getWidth() == 0 || getHeight() == 0)
    {
        DBG("no play path size set yet. assigning temporary values.");
        setSize(static_cast<int>(500.0f * relativeBounds.getWidth()),
            static_cast<int>(500.0f * relativeBounds.getHeight())); //must be set before the images are drawn, because drawables require bounds to be displayed! the values don't really matter; they are only temporary.
    }
    else
    {
        DBG("play path size already set, no temporary values required.");
    }

    DBG("initial size of play path: " + getBounds().toString());
    DBG("bounds of the underlying path: " + underlyingPath.getBounds().toString());

    normalImage.setPath(underlyingPath);
    normalImage.setFill(juce::FillType(juce::Colours::transparentBlack));
    normalImage.setStrokeThickness(outlineThickness);
    normalImage.setStrokeFill(juce::FillType(fillColour));
    normalImage.setBounds(getBounds());

    overImage.setPath(underlyingPath);
    overImage.setFill(juce::FillType(juce::Colours::transparentBlack));
    overImage.setStrokeThickness(outlineThickness);
    overImage.setStrokeFill(juce::FillType(fillColour.brighter(0.2f)));
    overImage.setBounds(getBounds());

    downImage.setPath(underlyingPath);
    downImage.setFill(juce::FillType(juce::Colours::transparentBlack));
    downImage.setStrokeThickness(outlineThickness);
    downImage.setStrokeFill(juce::FillType(fillColour.darker(0.2f)));
    downImage.setBounds(getBounds());

    disabledImage.setPath(underlyingPath);
    disabledImage.setFill(juce::FillType(juce::Colours::transparentBlack));
    disabledImage.setStrokeThickness(outlineThickness);
    disabledImage.setStrokeFill(juce::FillType(fillColour.withAlpha(0.3f)));
    disabledImage.setBounds(getBounds());


    normalImageOn.setPath(underlyingPath);
    normalImageOn.setFill(juce::FillType(juce::Colours::transparentBlack));
    normalImageOn.setStrokeThickness(outlineThickness);
    //normalImageOn.setStrokeFill(juce::FillType(juce::Colour::fromRGB(255 - fillColour.getRed(), 255 - fillColour.getGreen(), 255 - fillColour.getBlue()))); //use inverted fill colour as outline colour -> pops more than black/white
    normalImageOn.setStrokeFill(juce::FillType(fillColourOn));
    normalImageOn.setBounds(getBounds());

    overImageOn.setPath(underlyingPath);
    overImageOn.setFill(juce::FillType(juce::Colours::transparentBlack));
    overImageOn.setStrokeThickness(outlineThickness);
    overImageOn.setStrokeFill(juce::FillType(normalImageOn.getStrokeFill().colour.brighter(0.2f)));
    overImageOn.setBounds(getBounds());

    downImageOn.setPath(underlyingPath);
    downImageOn.setFill(juce::FillType(juce::Colours::transparentBlack));
    downImageOn.setStrokeThickness(outlineThickness);
    downImageOn.setStrokeFill(juce::FillType(normalImageOn.getStrokeFill().colour.darker(0.2f)));
    downImageOn.setBounds(getBounds());

    disabledImageOn.setPath(underlyingPath);
    disabledImageOn.setFill(juce::FillType(juce::Colours::transparentBlack));
    disabledImageOn.setStrokeThickness(outlineThickness);
    disabledImageOn.setStrokeFill(juce::FillType(normalImageOn.getStrokeFill().colour.withAlpha(0.3f)));
    disabledImageOn.setBounds(getBounds());

    setImages(&normalImage, &overImage, &downImage, &disabledImage, &normalImageOn, &overImageOn, &downImageOn, &disabledImageOn);

    setColour(ColourIds::backgroundOnColourId, juce::Colours::transparentBlack); //otherwise, the button has a grey background colour while the button is toggled on
}

void PlayPath::transitionToState(PlayPathStateIndex stateToTransitionTo, bool keepPlayingAndEditing)
{
    bool nonInstantStateFound = false;

    while (!nonInstantStateFound) //check if any states can be skipped (might be the case depending on what has been prepared already)
    {
        switch (stateToTransitionTo)
        {
        case PlayPathStateIndex::notInteractable:
            nonInstantStateFound = true;

            setToggleState(false, juce::NotificationType::dontSendNotification);
            setToggleable(false);
            setClickingTogglesState(false);

            if (!keepPlayingAndEditing)
            {
                if (isPlaying)
                {
                    stopPlaying();
                }

                if (pathEditorWindow != nullptr)
                {
                    pathEditorWindow.deleteAndZero(); //close editor window
                }
            }

            DBG("PlayPath not interactable");
            break;

        case PlayPathStateIndex::editable:
            nonInstantStateFound = true;

            setClickingTogglesState(true); //also automatically sets isToggleable
            setToggleState(false, juce::NotificationType::dontSendNotification);

            DBG("PlayPath editable");
            break;

        case PlayPathStateIndex::playable:
            nonInstantStateFound = true;

            setClickingTogglesState(true); //selected exclusively for the playable state, also automatically sets isToggleable
            setToggleState(isPlaying, juce::NotificationType::dontSendNotification);

            DBG("PlayPath playable");
            break;

        default:
            throw std::exception("unhandled state index");
        }
    }

    //finally, update the current state index
    currentStateIndex = stateToTransitionTo;
    currentState = states[static_cast<int>(currentStateIndex)];
}

void PlayPath::paintOverChildren(juce::Graphics& g)
{
    //draw range borders (only required for some debugging)
    g.setColour(fillColour.contrasting());
    float r = 1.0f;
    juce::Point<float> pt;
    for (auto itRange = regionsByRange_range.begin(); itRange != regionsByRange_range.end(); ++itRange)
    {
        //draw start border
        pt = getPointAlongPath((*itRange).getStart());
        g.fillEllipse(pt.getX() - r, pt.getY() - r, 2 * r, 2 * r);

        //draw end border
        pt = getPointAlongPath((*itRange).getEnd());
        g.fillEllipse(pt.getX() - r, pt.getY() - r, 2 * r, 2 * r);
    }

    //WIP: draw courier (this is more of a bandaid fix because the couriers wouldn't show up despite being added properly (perhaps because they were out of bounds)
    for (auto* itCourier = couriers.begin(); itCourier != couriers.end(); ++itCourier)
    {
        g.setColour(fillColour.contrasting());
        g.fillEllipse((*itCourier)->getBounds().toFloat());
        g.setColour(fillColour);
        g.fillEllipse((*itCourier)->getBounds().reduced(1).toFloat());
    }
}
void PlayPath::resized()
{
    juce::DrawableButton::resized();

    //recalculate hitbox
    underlyingPath.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), false);

    //adjust any courier(s)
    for (auto* itCourier = couriers.begin(); itCourier != couriers.end(); ++itCourier)
    {
        (*itCourier)->parentPathLengthChanged(); //needed to adjust the ratio of the courier's radius to the window size
        (*itCourier)->updateBounds();
    }
}

bool PlayPath::hitTest(int x, int y)
{
    return currentState->hitTest(x, y);
}
bool PlayPath::hitTest_Interactable(int x, int y)
{
    //return underlyingPath.contains(static_cast<float>(x), static_cast<float>(y))
    //return underlyingPath.contains(static_cast<float>(x), static_cast<float>(y)) && !innerPath.contains(static_cast<float>(x), static_cast<float>(y));
    
    juce::Point<float> currentPoint(static_cast<float>(x), static_cast<float>(y));
    juce::Point<float> pointOnPath;
    underlyingPath.getNearestPoint(currentPoint, pointOnPath);
    float distanceSquared = (pointOnPath.getX() - currentPoint.getX()) * (pointOnPath.getX() - currentPoint.getX()) +
                            (pointOnPath.getY() - currentPoint.getY()) * (pointOnPath.getY() - currentPoint.getY());

    return distanceSquared <= (2.5f * outlineThickness) * (2.5f * outlineThickness); //only register interactions near the outline, not in the inner region
}

juce::String PlayPath::getTooltip()
{
    switch (currentStateIndex)
    {
    case PlayPathStateIndex::editable:
        return "Click on this play path to open its editor window. You may have editor windows of other play paths open at the same time.";

        //case PlayPathStateIndex::playable:
        //    return "Click on this play path to play it. It will keep playing until you click it again.";
            //^- i'm pretty sure this would only get annoying

    default:
        return "";
    }
}

juce::Point<float> PlayPath::getPointAlongPath(double normedDistanceFromStart)
{
    return underlyingPath.getPointAlongPath(normedDistanceFromStart * underlyingPath.getLength());
}
float PlayPath::getPathLength()
{
    return underlyingPath.getLength();
}

void PlayPath::clicked(const juce::ModifierKeys& modifiers)
{
    currentState->clicked(modifiers);
}

bool PlayPath::isEditorOpen()
{
    return pathEditorWindow != nullptr;
}
void PlayPath::sendEditorToFront()
{
    pathEditorWindow->toFront(true);
    pathEditorWindow->getContentComponent()->grabKeyboardFocus();
}
void PlayPath::openEditor()
{
    pathEditorWindow = juce::Component::SafePointer<PlayPathEditorWindow>(new PlayPathEditorWindow("Play Path " + juce::String(ID) + " Editor", this));
    pathEditorWindow->getContentComponent()->grabKeyboardFocus();
}

void PlayPath::triggerButtonStateChanged()
{
    juce::Button::buttonStateChanged();
}
void PlayPath::triggerDrawableButtonStateChanged()
{
    juce::DrawableButton::buttonStateChanged();
}

//bool PlayPath::getIsPlaying()
//{
//    return isPlaying;
//}
void PlayPath::setIsPlaying_Click(bool shouldBePlaying)
{
    isPlaying_click = shouldBePlaying;
}
bool PlayPath::getIsPlaying_Click()
{
    return isPlaying_click;
}
bool PlayPath::shouldBePlaying()
{
    return isPlaying_click || isPlaying_midi;
}

void PlayPath::startPlaying(bool toggleButtonState)
{
    if (!isPlaying && shouldBePlaying())
    {
        //play couriers (have been prepared in advance)
        DBG("playing path " + juce::String(ID));

        //try to set the button's toggle state to "down" (needs to be done cross-thread for MIDI messages because they do not run on the same thread as couriers and clicks)
        if (toggleButtonState)
        {
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            {
                setToggleState(true, juce::NotificationType::dontSendNotification);
                //setState(juce::Button::ButtonState::buttonDown);
            }
            else
            {
                juce::MessageManager::getInstance()->callFunctionOnMessageThread([](void* data)
                    {
                        static_cast<PlayPath*>(data)->setToggleState(true, juce::NotificationType::dontSendNotification);
                        return static_cast<void*>(nullptr);
                    }, this);
            }
        }

        for (auto* itCourier = couriers.begin(); itCourier != couriers.end(); ++itCourier)
        {
            (*itCourier)->startRunning();

            //check whether the courier is already within a region when it's at its starting point
            juce::Range<float> currentRange = (*itCourier)->getCurrentRange();
            auto itRegion = regionsByRange_region.begin();
            auto itRange = regionsByRange_range.begin();
            for (; itRange != regionsByRange_range.end(); ++itRegion, ++itRange)
            {
                if (getCollisionWithRegion(*itRange, currentRange, currentRange) == CollisionType::within)
                {
                    //if any courier starts out within a region, immediately start playing that region
                    (*itRegion)->signalCourierEntered();
                }
            }
        }
        isPlaying = true;
    }
}
void PlayPath::stopPlaying(bool toggleButtonState)
{
    if (isPlaying && !shouldBePlaying())
    {
        //stop and delete all current couriers
        DBG("stopping path " + juce::String(ID));

        //try to set the button's toggle state to "up" (needs to be done cross-thread for MIDI messages because they do not run on the same thread as couriers and clicks)
        if (toggleButtonState)
        {
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            {
                setToggleState(false, juce::NotificationType::dontSendNotification);
                //setState(juce::Button::ButtonState::buttonDown);
            }
            else
            {
                juce::MessageManager::getInstance()->callFunctionOnMessageThread([](void* data)
                    {
                        static_cast<PlayPath*>(data)->setToggleState(false, juce::NotificationType::dontSendNotification);
                        return static_cast<void*>(nullptr);
                    }, this);
            }
        }

        for (auto* itCourier = couriers.begin(); itCourier != couriers.end(); ++itCourier)
        {
            (*itCourier)->stopRunning();
            removeChildComponent(*itCourier);

            //for any region that the courier was currently in, signal to that region that the courier has left
            juce::Array<int> intersectedRegions = (*itCourier)->getCurrentlyIntersectedRegions();
            for (auto itID = intersectedRegions.begin(); itID != intersectedRegions.end(); ++itID)
            {
                for (auto itRegion = regionsByRange_region.begin(); itRegion != regionsByRange_region.end(); ++itRegion)
                {
                    if ((*itRegion)->getID() == *itID)
                    {
                        (*itRegion)->signalCourierLeft();
                        //no need to delete the index from the intersectedRegions array
                        break;
                    }
                }
            }

            //reset courier's currently intersected regions
            (*itCourier)->resetCurrentlyIntersectedRegions();
        }
        couriers.clear(true);

        //prepare one new courier for the next time that this path is played
        PlayPathCourier* newCourier = new PlayPathCourier(this, courierIntervalSeconds);
        addChildComponent(newCourier);
        couriers.add(newCourier);
        //DBG("added a courier. bounds: " + newCourier->getBounds().toString() + " (within " + getLocalBounds().toString() + ")");

        isPlaying = false;
    }
}

int PlayPath::getID()
{
    return ID;
}

juce::Colour PlayPath::getFillColour()
{
    return fillColour;
}
void PlayPath::setFillColour(juce::Colour newFillColour)
{
    fillColour = newFillColour;
    initialiseImages();
}

juce::Colour PlayPath::getFillColourOn()
{
    return fillColourOn;
}
void PlayPath::setFillColourOn(juce::Colour newFillColourOn)
{
    fillColourOn = newFillColourOn;
    initialiseImages();
}

float PlayPath::getCourierInterval_seconds()
{
    return courierIntervalSeconds;
}
void PlayPath::setCourierInterval_seconds(float newCourierIntervalSeconds)
{
    courierIntervalSeconds = newCourierIntervalSeconds;

    for (auto* itCourier = couriers.begin(); itCourier != couriers.end(); ++itCourier)
    {
        (*itCourier)->setInterval_seconds(courierIntervalSeconds);
    }
}

//void PlayPath::addIntersectingRegion(SegmentedRegion* region)
//{
//    juce::Range<float> distances(-1.0f, -1.0f);
//    float stepSize = juce::jmax(0.0005, juce::jmin(0.01, 2.0 / 3.0 * (PlayPathCourier::radius / underlyingPath.getLength())));
//    DBG("stepSize: " + juce::String(stepSize));
//    float pathLengthDenom = 1.0f / underlyingPath.getLength();
//    
//    //getPointAlongPath returns a point relative to this *PlayPath's* own bounds, but hitTest checks for collision relative to the *SegmentedRegion's* own bounds. so to apply a hitTest to the point, it needs to be shifted according to the difference of the PlayPath's and SegmentedRegion's position in their shared parent component.
//    int xDifference = region->getBoundsInParent().getX() - this->getBoundsInParent().getX(); //difference from the region's x position in its parent compared to this play path's x position in that parent
//    int yDifference = region->getBoundsInParent().getY() - this->getBoundsInParent().getY(); //^- same but for y position
//
//    //check whether the starting point is already contained within the region. if so, begin from the first point not contained within the region. otherwise, begin from the starting point.
//    juce::Point<int> startingPt = underlyingPath.getPointAlongPath(0.0f).toInt();
//    float initialDistance = 0.0f;
//    if (region->hitTest_Interactable(startingPt.getX() - xDifference, startingPt.getY() - yDifference))
//    {
//        //starting point is already contained within the region
//        //-> the end distance of that section will be found first -> remember for later and add that section last
//        initialDistance = -1.0f;
//
//        for (float distanceFromStart = 0.0f; distanceFromStart < underlyingPath.getLength(); distanceFromStart += stepSize)
//        {
//            juce::Point<int> pt = underlyingPath.getPointAlongPath(distanceFromStart).toInt();
//            if (!region->hitTest_Interactable(pt.getX() - xDifference, pt.getY() - yDifference)) //note the negation!
//            {
//                //end point found
//                DBG("initial end point: " + pt.toString() + " (" + juce::String(distanceFromStart) + " / " + juce::String(underlyingPath.getLength()) + ")");
//                initialDistance = distanceFromStart;
//                break;
//            }
//        }
//        if (initialDistance < 0.0f)
//        {
//            //region encompasses the entire path
//            //regionsByRange_range.add(juce::Range<float>(0.0f, underlyingPath.getLength()));
//            regionsByRange_range.add(juce::Range<float>(0.0f, 1.0f));
//            regionsByRange_region.add(region);
//            return;
//        }
//        //else: initialDistance has been set
//    }
//    //else: initialDistance is 0.0f
//
//    for (float startDistance = initialDistance; startDistance < underlyingPath.getLength(); startDistance += stepSize)
//    {
//        //check current position for collision
//        juce::Point<int> pt = underlyingPath.getPointAlongPath(startDistance).toInt();
//
//        if (region->hitTest_Interactable(pt.getX() - xDifference, pt.getY() - yDifference))
//        {
//            //collision! -> starting point found
//            //distances.setStart(startDistance);
//            distances.setStart(startDistance * pathLengthDenom);
//            DBG("start point: " + pt.toString() + " (" + juce::String(startDistance) + " / " + juce::String(underlyingPath.getLength()) + ")");
//
//            //look for end point
//            bool endPointFound = false;
//            for (float endDistance = startDistance + stepSize; endDistance < underlyingPath.getLength(); endDistance += stepSize)
//            {
//                //check current position for collision
//                pt = underlyingPath.getPointAlongPath(endDistance).toInt();
//
//                if (!region->hitTest_Interactable(pt.getX() - xDifference, pt.getY() - yDifference)) //note the negation!
//                {
//                    //no more collision! -> end point found
//                    endPointFound = true;
//                    //distances.setEnd(endDistance);
//                    distances.setEnd(endDistance * pathLengthDenom);
//                    DBG("end point: " + pt.toString() + " (" + juce::String(endDistance) + " / " + juce::String(underlyingPath.getLength()) + ")");
//
//                    //add to list
//                    insertIntoRegionsLists(juce::Range<float>(distances), region);
//
//                    //update start distance, reset distance variable and keep looking for more intersections
//                    startDistance = endDistance;
//                    distances = juce::Range<float>(-1.0f, -1.0f);
//                    break;
//                }
//            }
//            if (!endPointFound)
//            {
//                //found a starting point but no end point -> arrived at the last intersection -> loops around
//                //distances.setEnd(initialDistance); //remembered from the beginning of this method
//                distances.setEnd(initialDistance * pathLengthDenom + 1.0); //remembered from the beginning of this method (note that juce::Range.end may not be smaller than juce::Range.start! hence, 1.0 is added to signal the loop-around)
//
//                //add to list
//                insertIntoRegionsLists(juce::Range<float>(distances), region);
//
//                //done searching
//                break;
//            }
//        }
//    }
//
//}
void PlayPath::addIntersectingRegion(SegmentedRegion* region)
{
    juce::Range<float> distances(-1.0f, -1.0f);
    float stepSize = juce::jmax(0.0001, juce::jmin(0.005, 2.0 / 3.0 * (PlayPathCourier::radius / underlyingPath.getLength())));
    DBG("stepSize: " + juce::String(stepSize));

    //getPointAlongPath returns a point relative to this *PlayPath's* own bounds, but hitTest checks for collision relative to the *SegmentedRegion's* own bounds. so to apply a hitTest to the point, it needs to be shifted according to the difference of the PlayPath's and SegmentedRegion's position in their shared parent component.
    int xDifference = region->getBoundsInParent().getX() - this->getBoundsInParent().getX(); //difference from the region's x position in its parent compared to this play path's x position in that parent
    int yDifference = region->getBoundsInParent().getY() - this->getBoundsInParent().getY(); //^- same but for y position

    //check whether the starting point is already contained within the region. if so, begin from the first point not contained within the region. otherwise, begin from the starting point.
    juce::Point<int> startingPt = getPointAlongPath(0.0f).toInt();
    float initialDistance = 0.0f;
    if (region->hitTest_Interactable(startingPt.getX() - xDifference, startingPt.getY() - yDifference))
    {
        //starting point is already contained within the region
        //-> the end distance of that section will be found first -> remember for later and add that section last
        initialDistance = -1.0f;

        for (float distanceFromStart = 0.0f; distanceFromStart < 1.0; distanceFromStart += stepSize)
        {
            juce::Point<int> pt = getPointAlongPath(distanceFromStart).toInt();
            if (!region->hitTest_Interactable(pt.getX() - xDifference, pt.getY() - yDifference)) //note the negation!
            {
                //end point found
                DBG("initial end point: " + pt.toString() + " (" + juce::String(distanceFromStart) + " / " + juce::String(1.0) + ")");
                initialDistance = distanceFromStart;
                break;
            }
        }
        if (initialDistance < 0.0f)
        {
            //region encompasses the entire path
            //regionsByRange_range.add(juce::Range<float>(0.0f, underlyingPath.getLength()));
            regionsByRange_range.add(juce::Range<float>(0.0f, 1.0f));
            regionsByRange_region.add(region);
            return;
        }
        //else: initialDistance has been set
    }
    //else: initialDistance is 0.0f

    for (float startDistance = initialDistance; startDistance < 1.0; startDistance += stepSize)
    {
        //check current position for collision
        juce::Point<int> pt = getPointAlongPath(startDistance).toInt();

        if (region->hitTest_Interactable(pt.getX() - xDifference, pt.getY() - yDifference))
        {
            //collision! -> starting point found
            //distances.setStart(startDistance);
            distances.setStart(startDistance);
            DBG("start point: " + pt.toString() + " (" + juce::String(startDistance) + " / " + juce::String(1.0) + ")");

            //look for end point
            bool endPointFound = false;
            for (float endDistance = startDistance + stepSize; endDistance < 1.0; endDistance += stepSize)
            {
                //check current position for collision
                pt = getPointAlongPath(endDistance).toInt();

                if (!region->hitTest_Interactable(pt.getX() - xDifference, pt.getY() - yDifference)) //note the negation!
                {
                    //no more collision! -> end point found
                    endPointFound = true;
                    //distances.setEnd(endDistance);
                    distances.setEnd(endDistance);
                    DBG("end point: " + pt.toString() + " (" + juce::String(endDistance) + " / " + juce::String(1.0) + ")");

                    //add to list
                    insertIntoRegionsLists(juce::Range<float>(distances), region);

                    //update start distance, reset distance variable and keep looking for more intersections
                    startDistance = endDistance;
                    distances = juce::Range<float>(-1.0f, -1.0f);
                    break;
                }
            }
            if (!endPointFound)
            {
                //found a starting point but no end point -> arrived at the last intersection -> loops around
                //distances.setEnd(initialDistance); //remembered from the beginning of this method
                distances.setEnd(initialDistance + 1.0); //remembered from the beginning of this method (note that juce::Range.end may not be smaller than juce::Range.start! hence, 1.0 is added to signal the loop-around)

                //add to list
                insertIntoRegionsLists(juce::Range<float>(distances), region);

                //done searching
                break;
            }
        }
    }

}
void PlayPath::recalculateAllIntersectingRegions()
{
    DBG("calculating the range lists...");

    auto* availableRegions = &(static_cast<SegmentableImage*>(getParentComponent())->regions);
    regionsByRange_range.clear();
    regionsByRange_region.clear();

    for (auto itRegion = availableRegions->begin(); itRegion != availableRegions->end(); ++itRegion)
    {
        if ((*itRegion)->getBounds().intersects(getBounds()))
        {
            addIntersectingRegion(*itRegion);
        }
    }

    DBG(juce::String(regionsByRange_range.size() > 0 ? "range lists have been calculated:" : "range lists have been calculated: no collisions."));
    for (int i = 0; i < regionsByRange_range.size(); ++i)
    {
        DBG(juce::String(regionsByRange_region[i]->getID()) + ": [" + juce::String(regionsByRange_range[i].getStart()) + ", " + juce::String(regionsByRange_range[i].getEnd()) + "]");
    }
}
void PlayPath::insertIntoRegionsLists(juce::Range<float> regionRange, SegmentedRegion* region)
{
    //if (regionRange.getStart() < 0.0f || regionRange.getEnd() < 0.0f || regionRange.getStart() > underlyingPath.getLength() || regionRange.getEnd() > underlyingPath.getLength())
    if (regionRange.getStart() < 0.0f || regionRange.getEnd() < 0.0f || regionRange.getStart() > 1.0f || regionRange.getEnd() > 1.0f)
    {
        //invalid range
        return;
    }

    if (regionsByRange_range.size() == 0)
    {
        regionsByRange_range.add(regionRange);
        regionsByRange_region.add(region);
    }
    else
    {
        //insert according to starting distance (sorted in ascending order)
        for (int i = 0; i < regionsByRange_range.size(); ++i)
        {
            if (regionsByRange_range[i].getStart() > regionRange.getStart())
            {
                regionsByRange_range.insert(i, regionRange);
                regionsByRange_region.insert(i, region);
                return;
            }
        }
        regionsByRange_range.add(regionRange);
        regionsByRange_region.add(region);
    }
}
void PlayPath::removeIntersectingRegion(int regionID)
{
    for (int i = 0; i < regionsByRange_region.size(); ++i)
    {
        if (regionsByRange_region[i]->getID() == regionID)
        {
            regionsByRange_region.remove(i);
            regionsByRange_range.remove(i);
            --i;
            //no break, because regions can be contained a number of times!
        }
    }
}
void PlayPath::evaluateCourierPosition(PlayPathCourier* courier, juce::Range<float> previousPosition, juce::Range<float> newPosition)
{
    //WIP: note that this evaluation method isn't entirely accurate.
    //     it interprets the courier as a line along the path, not as a circle,
    //     i.e. it will ignore if a courier touches a region if that part of the region *doesn't intersect* with the path.
    //     HOWEVER, since the couriers' radius is usually small compared to regions' sizes, this is rather unlikely to occur often.
    //     also, this assumption makes the method easier (+ faster) to evaluate and, perhaps even more importantly,
    //     independent from the size of the window containing the path (since couriers have a fixed size for visibility reasons).

    auto itRange = regionsByRange_range.begin();
    auto itRegion = regionsByRange_region.begin();

    for (; itRange != regionsByRange_range.end(); ++itRange, ++itRegion)
    {
        switch (getCollisionWithRegion(*itRange, previousPosition, newPosition))
        {
        case CollisionType::entered:
            //DBG("courier entered region " + juce::String((*itRegion)->getID()) + ".");
            (*itRegion)->signalCourierEntered();
            courier->signalRegionEntered((*itRegion)->getID());
            break;

        case CollisionType::exited:
            //DBG("courier left region " + juce::String((*itRegion)->getID()) + ".");
            (*itRegion)->signalCourierLeft();
            courier->signalRegionExited((*itRegion)->getID());
            break;

            //default: either no collision or remaining within a region -> no change
        }
    }

}
PlayPath::CollisionType PlayPath::getCollisionWithRegion(const juce::Range<float>& regionRange, const juce::Range<float>& previousRange, const juce::Range<float>& currentRange)
{
    if (intersectsWithWraparound(previousRange, regionRange)) //(regionRange.intersects(previousRange))
    {
        //-> courier was within the region during its last tick
        if (!intersectsWithWraparound(currentRange, regionRange))
        {
            //-> no longer within the region -> request to stop playing
            return CollisionType::exited;
        }
        else //courier is still within the region -> no change
        {
            return CollisionType::within;
        }
    }
    else //range doesn't contain start
    {
        //-> courier was not within the region during its last tick
        if (intersectsWithWraparound(currentRange, regionRange))
        {
            //-> courier has now entered the region -> request to start playing
            return CollisionType::entered;
        }
        else //courier is currently not within the region -> either skipped a really small strip or the range was outside the given distances -> no change
        {
            return CollisionType::noCollision;
        }
    }

}
bool PlayPath::intersectsWithWraparound(const juce::Range<float>& r1, const juce::Range<float>& r2)
{
    //assumptions used:
    //- range.start <= range.end    (automatically enforced by juce::Range)
    //- 0.0 <= range.start < 1.0
    //- 0.0 <= range.end


    //CLEAN, LONG VERSION (for understanding what this algorithm does):
    
    //if (r1.getStart() < r2.getEnd())
    //{
    //    //start1 < end2
    //    if (r1.getEnd() >= r2.getStart())
    //    {
    //        //normal intersection
    //        return true;
    //    }
    //    else
    //    {
    //        //start1 < end2, end1 < start2
    //        //normal case: no intersection
    //        //overlap case: intersection if start1 <= end2 % 1
    //        if (r2.getEnd() > 1.0f && r1.getStart() <= r2.getEnd() - 1.0f)
    //        {
    //            return true;
    //        }
    //        else
    //        {
    //            return false;
    //        }
    //    }
    //}
    //else
    //{
    //    //start1 >= end2
    //    //normal case: no collision
    //    //overlap case: collision if end1 % 1 >= start2
    //    if (r1.getEnd() > 1.0f && r2.getStart() <= r1.getEnd() - 1.0f)
    //    {
    //        return true;
    //    }
    //    else
    //    {
    //        return false;
    //    }
    //}

    //SHORT VERSION:
    // -> performs the above checks as a one-liner and without short-circuiting (except where it's necessary). the formatting mimics the previous if-cases.
    //converting the code into this form makes a *big difference* since this method may be called at a very high frequency
    //and every if-case (incl. short-circuits) will cause a noticable slowdown because of branch prediction failures!
    return ((r1.getStart() < r2.getEnd()) & (
                (r1.getEnd() >= r2.getStart())
                |
                (r2.getEnd() > 1.0f && r1.getStart() <= r2.getEnd() - 1.0f)
                )
           )
           |
           (r1.getEnd() > 1.0f && r2.getStart() <= r1.getEnd() - 1.0f);
}

int PlayPath::getMidiChannel()
{
    return midiChannel;
}
void PlayPath::setMidiChannel(int newMidiChannel)
{
    midiChannel = newMidiChannel;
}
int PlayPath::getMidiNote()
{
    return noteNumber;
}
void PlayPath::setMidiNote(int newNoteNumber)
{
    noteNumber = newNoteNumber;
}
void PlayPath::handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    //DBG("registered ON note: " + juce::String(midiChannel) + " " + juce::String(midiNoteNumber));

    //LONG VERSION (easier to understand, but has too many if cases):
    //if (this->midiChannel >= 0 && noteNumber >= 0)
    //{
    //    //play path is setup to receive MIDI (WIP: maybe make this into a state - this isn't too urgent, though, because MIDI messages are rare compared to samples)

    //    if (this->midiChannel == 0 || (this->midiChannel == midiChannel))
    //    {
    //        //region listens to the given MIDI channel -> evaluate note

    //        if (noteNumber == midiNoteNumber)
    //        {
    //            //note number matches with the one that the play path responds to
    //            if (isPlaying)
    //            {
    //                stopPlaying();
    //            }
    //            else
    //            {
    //                startPlaying();
    //            }
    //        }
    //    }
    //}

    //SHORT VERSION (combines several of the above if cases into one case to improve performance):
    if ((this->midiChannel >= 0 && noteNumber >= 0) && (this->midiChannel == 0 || (this->midiChannel == midiChannel)) && (noteNumber == midiNoteNumber))
    {
        //play path is set up to receive MIDI, and the incoming MIDI corresponds to that which the play path recognises
        if (isPlaying_midi)
        {
            isPlaying_midi = false;
            juce::MessageManager::getInstance()->callFunctionOnMessageThread([](void* data)
                {
                    static_cast<PlayPath*>(data)->stopPlaying(); //needs to be called on the message thread because the couriers are also running there
                    return static_cast<void*>(nullptr);
                }, this);
        }
        else
        {
            isPlaying_midi = true;
            juce::MessageManager::getInstance()->callFunctionOnMessageThread([](void* data)
                {
                    static_cast<PlayPath*>(data)->startPlaying(); //needs to be called on the message thread because the couriers are also running there
                    return static_cast<void*>(nullptr);
                }, this);
        }

    }
}
void PlayPath::handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    //since play paths are always toggleable, off signals don't need to be handled.
}

bool PlayPath::serialise(juce::XmlElement* xmlPlayPath)
{
    DBG("serialising PlayPath...");
    bool serialisationSuccessful = true;

    xmlPlayPath->setAttribute("ID", ID);
    xmlPlayPath->setAttribute("underlyingPath", underlyingPath.toString());
    xmlPlayPath->setAttribute("fillColour", fillColour.toString());
    xmlPlayPath->setAttribute("fillColourOn", fillColourOn.toString());
    xmlPlayPath->setAttribute("courierIntervalSeconds", courierIntervalSeconds);

    juce::XmlElement* xmlRelativeBounds = xmlPlayPath->createNewChildElement("relativeBounds");
    xmlRelativeBounds->setAttribute("x", relativeBounds.getX());
    xmlRelativeBounds->setAttribute("y", relativeBounds.getY());
    xmlRelativeBounds->setAttribute("width", relativeBounds.getWidth());
    xmlRelativeBounds->setAttribute("height", relativeBounds.getHeight());

    xmlPlayPath->setAttribute("midiChannel", midiChannel);
    xmlPlayPath->setAttribute("noteNumber", noteNumber);

    //serialise range lists
    juce::XmlElement* xmlRangeLists = xmlPlayPath->createNewChildElement("regionRangeLists");
    jassert(regionsByRange_range.size() == regionsByRange_region.size());
    xmlRangeLists->setAttribute("size", regionsByRange_range.size());
    for (int i = 0; i < regionsByRange_range.size(); ++i)
    {
        juce::XmlElement* xmlItem = xmlRangeLists->createNewChildElement("item_" + juce::String(i));
        xmlItem->setAttribute("regionID", regionsByRange_region[i]->getID());
        xmlItem->setAttribute("startValue", regionsByRange_range[i].getStart());
        xmlItem->setAttribute("endValue", regionsByRange_range[i].getEnd());
    }

    DBG(juce::String(serialisationSuccessful ? "PlayPath has been serialised." : "PlayPath could not be serialised."));
    return serialisationSuccessful;
}
bool PlayPath::deserialise(juce::XmlElement* xmlPlayPath)
{
    DBG("deserialising PlayPath...");
    bool deserialisationSuccessful = true;
    stopPlaying(); //removes all couriers

    ID = xmlPlayPath->getIntAttribute("ID", -1);
    underlyingPath.clear();
    underlyingPath.restoreFromString(xmlPlayPath->getStringAttribute("underlyingPath", ""));
    fillColour = juce::Colour::fromString(xmlPlayPath->getStringAttribute("fillColour", juce::Colours::black.toString()));
    fillColourOn = juce::Colour::fromString(xmlPlayPath->getStringAttribute("fillColourOn", fillColour.darker(0.2f).toString()));
    setCourierInterval_seconds(xmlPlayPath->getDoubleAttribute("courierIntervalSeconds", 10.0f));

    midiChannel = xmlPlayPath->getIntAttribute("midiChannel", -1);
    noteNumber = xmlPlayPath->getIntAttribute("noteNumber", -1);

    juce::XmlElement* xmlRelativeBounds = xmlPlayPath->getChildByName("relativeBounds");
    if (xmlRelativeBounds != nullptr)
    {
        relativeBounds.setBounds(xmlRelativeBounds->getDoubleAttribute("x", 0.0),
            xmlRelativeBounds->getDoubleAttribute("y", 0.0),
            xmlRelativeBounds->getDoubleAttribute("width", 0.0),
            xmlRelativeBounds->getDoubleAttribute("height", 0.0)
        );

        //immediately resize (important in case ranges need to be recalculated later!)
        juce::Rectangle<float> parentBounds = getParentComponent()->getBounds().toFloat();
        juce::Rectangle<float> newBounds(relativeBounds.getX() * parentBounds.getWidth(),
                                         relativeBounds.getY() * parentBounds.getHeight(),
                                         relativeBounds.getWidth() * parentBounds.getWidth(),
                                         relativeBounds.getHeight() * parentBounds.getHeight());
        setBounds(newBounds.toNearestInt());
    }
    else
    {
        DBG("no relativeBounds data found.");
        relativeBounds.setBounds(0.0, 0.0, 0.0, 0.0);
        deserialisationSuccessful = false;
    }

    //deserialise range lists
    regionsByRange_range.clear();
    regionsByRange_region.clear();
    juce::XmlElement* xmlRangeLists = xmlPlayPath->getChildByName("regionRangeLists");
    auto* availableRegions = &(static_cast<SegmentableImage*>(getParentComponent())->regions);
    if (xmlRangeLists != nullptr)
    {
        int size = xmlRangeLists->getIntAttribute("size");

        DBG(juce::String(size > 0 ? "deserialising ranges:" : "no ranges contained."));

        for (int i = 0; i < size; ++i)
        {
            juce::XmlElement* xmlItem = xmlRangeLists->getChildByName("item_" + juce::String(i));
            if (xmlItem != nullptr)
            {
                int regionID = xmlItem->getIntAttribute("regionID", -1);
                juce::Range<float> range(xmlItem->getDoubleAttribute("startValue", -1.0), xmlItem->getDoubleAttribute("endValue", -1.0));
                DBG(juce::String(regionID) + ": [" + juce::String(range.getStart()) + ", " + juce::String(range.getEnd()) + "]");

                if (regionID >= 0 && range.getStart() >= 0.0f && range.getEnd() >= 0.0f)
                {
                    //valid item. -> get the pointer to the region from the region list in SegmentableImage
                    for (auto itRegion = availableRegions->begin(); itRegion != availableRegions->end(); ++itRegion)
                    {
                        if ((*itRegion)->getID() == regionID)
                        {
                            //corresponding region found -> add
                            DBG("confirmed.");
                            regionsByRange_region.add(*itRegion);
                            regionsByRange_range.add(range);
                            break;
                        }
                    }
                }
                else
                {
                    DBG("invalid or missing data values for regionRangeList item_" + juce::String(i) + ".");
                    deserialisationSuccessful = false;
                    break;
                }
            }
            else
            {
                DBG("data for regionRangeList item_" + juce::String(i) + " not found.");
                deserialisationSuccessful = false;
                break;
            }
        }
        DBG(juce::String(size > 0 ? "ranges deserialised." : ""));

        if (size == 0 && availableRegions->size() > 0)
        {
            //ranges might be missing -> double check
            recalculateAllIntersectingRegions();
        }

        if (!deserialisationSuccessful) //note the negation!
        {
            //one of the range list items was missing -> just recalculate all items manually, the it's fine
            recalculateAllIntersectingRegions();
            deserialisationSuccessful = true;
        }
    }
    else
    {
        DBG("data for regionRangeLists not found.");
        //deserialisationSuccessful = false; //-> not a big problem, just recalculate everything manually
        recalculateAllIntersectingRegions();
    }

    //data has been read. now, re-initialise all remaining components.
    initialiseImages(); //re-initialises all images
    resized();

    DBG(juce::String(deserialisationSuccessful ? "PlayPath has been deserialised." : "PlayPath could not be deserialised."));
    return deserialisationSuccessful;
}




void PlayPath::buttonStateChanged()
{
    currentState->buttonStateChanged();
}

void PlayPath::mouseDown(const juce::MouseEvent& e)
{
    if (isEnabled())
    {
        juce::DrawableButton::mouseDown(e);
    }
    else
    {
        getParentComponent()->mouseDown(e.getEventRelativeTo(getParentComponent())); //when the play path is disabled, pass clicks back to the segmentable image (setInterceptsMouseClicks(false, false) doesn't do anything for DrawableButtons sadly...)
    }
}
void PlayPath::mouseUp(const juce::MouseEvent& e)
{
    if (isEnabled())
    {
        juce::DrawableButton::mouseUp(e);
    }
    else
    {
        getParentComponent()->mouseUp(e.getEventRelativeTo(getParentComponent())); //when the play path is disabled, pass clicks back to the segmentable image (setInterceptsMouseClicks(false, false) doesn't do anything for DrawableButtons sadly...)
    }
}
