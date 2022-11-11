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




PlayPath::PlayPath(int ID, const juce::Path& path, const juce::Rectangle<float>& relativeBounds, juce::Colour fillColour) :
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
    initialiseImages();

    //initialise the first courier(s)
    stopPlaying(); //this also prepares the couriers that will be played next in advance

    //rest
    setToggleable(true);
}
PlayPath::~PlayPath()
{
    if (pathEditorWindow != nullptr) //is editor currently opened?
    {
        pathEditorWindow.deleteAndZero(); //close and release
    }

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
    normalImageOn.setStrokeFill(juce::FillType(fillColour.brighter(0.2f))); //use inverted fill colour as outline colour -> pops more than black/white
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

void PlayPath::transitionToState(PlayPathStateIndex stateToTransitionTo)
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

            if (isPlaying)
            {
                stopPlaying();
            }

            if (pathEditorWindow != nullptr)
            {
                pathEditorWindow.deleteAndZero(); //close editor window
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
    //recalculate hitbox
    underlyingPath.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), false);

    //adjust any courier(s)
    for (auto* itCourier = couriers.begin(); itCourier != couriers.end(); ++itCourier)
    {
        //(*itCourier)->pathLengthUpdated(); //needed to adjust the couriers' traveling speed
        (*itCourier)->parentPathLengthChanged(); //needed to adjust the ratio of the courier's radius to the window size
        (*itCourier)->updateBounds();
    }

    juce::DrawableButton::resized();
}

bool PlayPath::hitTest(int x, int y)
{
    return currentState->hitTest(x, y);
}
bool PlayPath::hitTest_Interactable(int x, int y)
{
    return underlyingPath.contains((float)x, (float)y);
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
}
void PlayPath::openEditor()
{
    pathEditorWindow = juce::Component::SafePointer<PlayPathEditorWindow>(new PlayPathEditorWindow("Play Path " + juce::String(ID) + " Editor", this));
}

void PlayPath::triggerButtonStateChanged()
{
    juce::Button::buttonStateChanged();
}
void PlayPath::triggerDrawableButtonStateChanged()
{
    juce::DrawableButton::buttonStateChanged();
}

void PlayPath::startPlaying()
{
    if (!isPlaying)
    {
        //play couriers (have been prepared in advance)
        DBG("playing path " + juce::String(ID));
        for (auto* itCourier = couriers.begin(); itCourier != couriers.end(); ++itCourier)
        {
            (*itCourier)->startRunning();
        }
        isPlaying = true;
    }
}
void PlayPath::stopPlaying()
{
    //stop and delete all current couriers
    DBG("stopping path " + juce::String(ID));
    for (auto* itCourier = couriers.begin(); itCourier != couriers.end(); ++itCourier)
    {
        (*itCourier)->stopRunning();
        removeChildComponent(*itCourier);
    }
    couriers.clear(true);

    //prepare one new courier for the next time that this path is played
    PlayPathCourier* newCourier = new PlayPathCourier(this, courierIntervalSeconds);
    addChildComponent(newCourier);
    couriers.add(newCourier);
    //DBG("added a courier. bounds: " + newCourier->getBounds().toString() + " (within " + getLocalBounds().toString() + ")");

    isPlaying = false;
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

void PlayPath::addIntersectingRegion(SegmentedRegion* region)
{
    juce::Range<float> distances(-1.0f, -1.0f);
    float stepSize = juce::jmax(0.0005, juce::jmin(0.01, 2.0 / 3.0 * (PlayPathCourier::radius / underlyingPath.getLength())));
    DBG("stepSize: " + juce::String(stepSize));
    float pathLengthDenom = 1.0f / underlyingPath.getLength();
    
    //getPointAlongPath returns a point relative to this *PlayPath's* own bounds, but hitTest checks for collision relative to the *SegmentedRegion's* own bounds. so to apply a hitTest to the point, it needs to be shifted according to the difference of the PlayPath's and SegmentedRegion's position in their shared parent component.
    int xDifference = region->getBoundsInParent().getX() - this->getBoundsInParent().getX(); //difference from the region's x position in its parent compared to this play path's x position in that parent
    int yDifference = region->getBoundsInParent().getY() - this->getBoundsInParent().getY(); //^- same but for y position

    //check whether the starting point is already contained within the region. if so, begin from the first point not contained within the region. otherwise, begin from the starting point.
    juce::Point<int> startingPt = underlyingPath.getPointAlongPath(0.0f).toInt();
    float initialDistance = 0.0f;
    if (region->hitTest_Interactable(startingPt.getX() - xDifference, startingPt.getY() - yDifference))
    {
        //starting point is already contained within the region
        //-> the end distance of that section will be found first -> remember for later and add that section last
        initialDistance = -1.0f;

        for (float distanceFromStart = 0.0f; distanceFromStart < underlyingPath.getLength(); distanceFromStart += stepSize)
        {
            juce::Point<int> pt = underlyingPath.getPointAlongPath(distanceFromStart).toInt();
            if (!region->hitTest_Interactable(pt.getX() - xDifference, pt.getY() - yDifference)) //note the negation!
            {
                //end point found
                DBG("initial end point: " + pt.toString() + " (" + juce::String(distanceFromStart) + " / " + juce::String(underlyingPath.getLength()) + ")");
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

    for (float startDistance = initialDistance; startDistance < underlyingPath.getLength(); startDistance += stepSize)
    {
        //check current position for collision
        juce::Point<int> pt = underlyingPath.getPointAlongPath(startDistance).toInt();

        if (region->hitTest_Interactable(pt.getX() - xDifference, pt.getY() - yDifference))
        {
            //collision! -> starting point found
            //distances.setStart(startDistance);
            distances.setStart(startDistance * pathLengthDenom);
            DBG("start point: " + pt.toString() + " (" + juce::String(startDistance) + " / " + juce::String(underlyingPath.getLength()) + ")");

            //look for end point
            bool endPointFound = false;
            for (float endDistance = startDistance + stepSize; endDistance < underlyingPath.getLength(); endDistance += stepSize)
            {
                //check current position for collision
                pt = underlyingPath.getPointAlongPath(endDistance).toInt();

                if (!region->hitTest_Interactable(pt.getX() - xDifference, pt.getY() - yDifference)) //note the negation!
                {
                    //no more collision! -> end point found
                    endPointFound = true;
                    //distances.setEnd(endDistance);
                    distances.setEnd(endDistance * pathLengthDenom);
                    DBG("end point: " + pt.toString() + " (" + juce::String(endDistance) + " / " + juce::String(underlyingPath.getLength()) + ")");

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
                distances.setEnd(initialDistance * pathLengthDenom + 1.0); //remembered from the beginning of this method (note that juce::Range.end may not be smaller than juce::Range.start! hence, 1.0 is added to signal the loop-around)

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
//void PlayPath::evaluateCourierPosition(juce::Range<float> previousPosition, juce::Range<float> newPosition)
//{
//    //WIP: note that this evaluation method isn't entirely accurate.
//    //     it interprets the courier as a line along the path, not as a circle,
//    //     i.e. it will ignore if a courier touches a region if that part of the region *doesn't intersect* with the path.
//    //     HOWEVER, since the couriers' radius is usually small compared to regions' sizes, this is rather unlikely to occur often.
//    //     also, this assumption makes the method easier (+ faster) to evaluate and, perhaps even more importantly,
//    //     independent from the size of the window containing the path (since couriers have a fixed size for visibility reasons).
//
//    auto itRange = regionsByRange_range.begin();
//    auto itRegion = regionsByRange_region.begin();
//
//    if (previousPosition.getStart() <= newPosition.getEnd()) //(distances.getStart() <= distances.getEnd())
//    {
//        //-> normal case: courier went along the path without looping around
//
//        for (; itRange != regionsByRange_range.end(); ++itRange, ++itRegion)
//        {
//            if ((*itRange).getStart() <= (*itRange).getEnd())
//            {
//                //-> normal range: starts and ends on the path without looping around
//
//                if ((*itRange).intersects(previousPosition)) //((*itRange).contains(distances.getStart()))
//                {
//                    //-> courier was within the region during its last tick
//                    if (!(*itRange).intersects(newPosition)) //(!(*itRange).contains(distances.getEnd()))
//                    {
//                        //-> no longer within the region -> request to stop playing
//                        (*itRegion)->signalCourierLeft();
//                        DBG("courier left region " + juce::String((*itRegion)->getID()) + ".");
//                    }
//                    //else: courier is still within the region -> no change
//                }
//                else //range doesn't contain start
//                {
//                    //-> courier was not within the region during its last tick
//                    if ((*itRange).intersects(newPosition)) //((*itRange).contains(distances.getEnd()))
//                    {
//                        //-> courier has now entered the region -> request to start playing
//                        (*itRegion)->signalCourierEntered();
//                        DBG("courier entered region " + juce::String((*itRegion)->getID()) + ".");
//                    }
//                    //else: courier is currently not within the region -> either skipped a really small strip or the range was outside the given distances -> no change
//                }
//
//            }
//            else //range start > range end
//            {
//                //-> special range: starts before and ends after the starting point of the path, i.e. it loops around
//                //-> check for [range.start, range.end + length] and [range.start - length, range.end] instead
//                juce::Range<float> r1((*itRange).getStart(),
//                                      (*itRange).getEnd() + underlyingPath.getLength());
//                juce::Range<float> r2((*itRange).getStart() - underlyingPath.getLength(),
//                                      (*itRange).getEnd());
//
//                if (r1.intersects(previousPosition) || r2.intersects(previousPosition)) //((*itRange).getStart() <= distances.getStart())
//                {
//                    //-> courier was within the region during its last tick
//                    if (!r1.intersects(newPosition) && !r2.intersects(newPosition))
//                    {
//                        //-> courier is no longer within the region now -> request to stop playing
//                        (*itRegion)->signalCourierLeft();
//                        DBG("courier left region " + juce::String((*itRegion)->getID()) + ".");
//                    }
//                    //else: courier is still within the region -> no change
//                }
//                else //range doesn't contain start
//                {
//                    //-> courier was not within the region during its last tick
//                    //since distance.start <= distance.end in this case, the courier couldn't have looped around -> no need to check for range.end
//                    if (r1.intersects(newPosition) || r2.intersects(newPosition)) //((*itRange).getStart() <= distances.getEnd())
//                    {
//                        //-> courier has now entered the region -> request to start playing
//                        (*itRegion)->signalCourierEntered();
//                        DBG("courier entered region " + juce::String((*itRegion)->getID()) + ".");
//                    }
//                    //else: courier is currently not within the region -> either skipped a really small strip or the range was outside the given distances -> no change
//                }
//            }
//        }
//
//    }
//    else //previousPosition.getStart() > newPosition.getEnd() //distances.getStart() > distances.getEnd()
//    {
//        //-> special case: courier looped around while walking
//        //-> always check for [range.start, range.end + length] and [range.start - length, range.end] instead
//        
//        //prepare new ranges for evaluation
//        juce::Range<float> previousPositionLoop1, previousPositionLoop2, newPositionLoop1, newPositionLoop2;
//        if (previousPosition.getStart() <= previousPosition.getEnd())
//        {
//            previousPositionLoop1 = previousPosition;
//            previousPositionLoop1 = previousPosition;
//        }
//        else
//        {
//            previousPositionLoop1.setStart(previousPosition.getStart());
//            previousPositionLoop1.setEnd(previousPosition.getEnd() + underlyingPath.getLength());
//            previousPositionLoop2.setStart(previousPosition.getStart() - underlyingPath.getLength());
//            previousPositionLoop2.setEnd(previousPosition.getEnd());
//        }
//        if (newPosition.getStart() <= newPosition.getEnd())
//        {
//            newPositionLoop1 = newPosition;
//            newPositionLoop1 = newPosition;
//        }
//        else
//        {
//            newPositionLoop1.setStart(newPosition.getStart());
//            newPositionLoop1.setEnd(newPosition.getEnd() + underlyingPath.getLength());
//            newPositionLoop2.setStart(newPosition.getStart() - underlyingPath.getLength());
//            newPositionLoop2.setEnd(newPosition.getEnd());
//        }
//
//        for (; itRange != regionsByRange_range.end(); ++itRange, ++itRegion)
//        {
//            if ((*itRange).getStart() <= (*itRange).getEnd())
//            {
//                //-> normal range: starts and ends on the path without looping around
//
//                if ((*itRange).intersects(previousPositionLoop1) || (*itRange).intersects(previousPositionLoop2)) //((*itRange).contains(distances.getStart()))
//                {
//                    //-> courier was within the region during its last tick
//                    if (!(*itRange).intersects(newPositionLoop1) && !(*itRange).intersects(newPositionLoop2)) //(!(*itRange).contains(distances.getEnd()))
//                    {
//                        //-> no longer within the region -> request to stop playing
//                        (*itRegion)->signalCourierLeft();
//                        DBG("courier left region " + juce::String((*itRegion)->getID()) + ".");
//                    }
//                    //else: courier is still within the region -> no change
//                }
//                else //range doesn't contain start
//                {
//                    //-> courier was not within the region during its last tick
//                    if ((*itRange).intersects(newPositionLoop1) || (*itRange).intersects(newPositionLoop2)) //((*itRange).contains(distances.getEnd()))
//                    {
//                        //-> courier has now entered the region -> request to start playing
//                        (*itRegion)->signalCourierEntered();
//                        DBG("courier entered region " + juce::String((*itRegion)->getID()) + ".");
//                    }
//                    //else: courier is currently not within the region -> either skipped a really small strip or the range was outside the given distances -> no change
//                }
//
//            }
//            else //range start > range end
//            {
//                //-> special range: starts before and ends after the starting point of the path, i.e. it loops around
//                //-> check for [range.start, range.end + length] and [range.start - length, range.end] instead
//                juce::Range<float> r1((*itRange).getStart(),
//                                      (*itRange).getEnd() + underlyingPath.getLength());
//                juce::Range<float> r2((*itRange).getStart() - underlyingPath.getLength(),
//                                      (*itRange).getEnd());
//
//                if (r1.intersects(previousPositionLoop1) || r1.intersects(previousPositionLoop2) || r2.intersects(previousPositionLoop1) || r2.intersects(previousPositionLoop2)) //((*itRange).getStart() <= distances.getStart())
//                {
//                    //-> courier was within the region during its last tick
//                    if (!r1.intersects(newPositionLoop1) && !r1.intersects(newPositionLoop2) && !r2.intersects(newPositionLoop1) && !r2.intersects(newPositionLoop2)) //((*itRange).getEnd() < distances.getEnd())
//                    {
//                        //-> courier is no longer within the range -> request to stop playing
//                        (*itRegion)->signalCourierLeft();
//                        DBG("courier left region " + juce::String((*itRegion)->getID()) + ".");
//                    }
//                    //else: courier is still within the range -> no change
//                }
//                else //range doesn't contain start
//                {
//                    //-> courier was not within the region during its last tick
//                    //note that the courier also loops around in this case
//                    if (r1.intersects(newPositionLoop1) || r1.intersects(newPositionLoop2) || r2.intersects(newPositionLoop1) || r2.intersects(newPositionLoop2)) //((*itRange).getEnd() >= distances.getEnd())
//                    {
//                        //-> courier has now entered the region -> request to start playing
//                        (*itRegion)->signalCourierEntered();
//                        DBG("courier entered region " + juce::String((*itRegion)->getID()) + ".");
//                    }
//                    //else: courier is currently not within the region -> either skipped a really small strip or the range was outside the given distances -> no change
//                }
//            }
//        }
//    }
//
//}
void PlayPath::evaluateCourierPosition(juce::Range<float> previousPosition, juce::Range<float> newPosition)
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
            (*itRegion)->signalCourierEntered();
            DBG("courier entered region " + juce::String((*itRegion)->getID()) + ".");
            break;

        case CollisionType::exited:
            (*itRegion)->signalCourierLeft();
            DBG("courier left region " + juce::String((*itRegion)->getID()) + ".");
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

    if (r1.getStart() < r2.getEnd())
    {
        //start1 < end2
        if (r1.getEnd() >= r2.getStart())
        {
            //normal intersection
            return true;
        }
        else
        {
            //start1 < end2, end1 < start2
            //normal case: no intersection
            //overlap case: intersection if start1 <= end2 % 1
            if (r2.getEnd() > 1.0f && r1.getStart() <= r2.getEnd() - 1.0f)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        //start1 >= end2
        //normal case: no collision
        //overlap case: collision if end1 % 1 >= start2
        if (r1.getEnd() > 1.0f && r2.getStart() <= r1.getEnd() - 1.0f)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

}

bool PlayPath::serialise(juce::XmlElement* xmlPlayPath)
{
    DBG("serialising PlayPath...");
    bool serialisationSuccessful = true;

    xmlPlayPath->setAttribute("ID", ID);
    xmlPlayPath->setAttribute("underlyingPath", underlyingPath.toString());
    xmlPlayPath->setAttribute("fillColour", fillColour.toString());
    xmlPlayPath->setAttribute("courierIntervalSeconds", courierIntervalSeconds);

    juce::XmlElement* xmlRelativeBounds = xmlPlayPath->createNewChildElement("relativeBounds");
    xmlRelativeBounds->setAttribute("x", relativeBounds.getX());
    xmlRelativeBounds->setAttribute("y", relativeBounds.getY());
    xmlRelativeBounds->setAttribute("width", relativeBounds.getWidth());
    xmlRelativeBounds->setAttribute("height", relativeBounds.getHeight());

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
    setCourierInterval_seconds(xmlPlayPath->getDoubleAttribute("courierIntervalSeconds", 10.0f));

    juce::XmlElement* xmlRelativeBounds = xmlPlayPath->getChildByName("relativeBounds");
    if (xmlRelativeBounds != nullptr)
    {
        relativeBounds.setBounds(xmlRelativeBounds->getDoubleAttribute("x", 0.0),
            xmlRelativeBounds->getDoubleAttribute("y", 0.0),
            xmlRelativeBounds->getDoubleAttribute("width", 0.0),
            xmlRelativeBounds->getDoubleAttribute("height", 0.0)
        );

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
    resized(); //updates focusAbs, calculates the current lfoLine and redraws the component (or rather, it's *supposed* to redraw it...)

    DBG(juce::String(deserialisationSuccessful ? "PlayPath has been deserialised." : "PlayPath could not be deserialised."));
    return deserialisationSuccessful;
}




void PlayPath::buttonStateChanged()
{
    currentState->buttonStateChanged();
}
