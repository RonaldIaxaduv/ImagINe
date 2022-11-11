/*
  ==============================================================================

    PlayPathCourier.cpp
    Created: 7 Nov 2022 8:54:22am
    Author:  Aaron

  ==============================================================================
*/

#include "PlayPathCourier.h"
#include "PlayPath.h"

//constants
const float PlayPathCourier::radius = 5.0f;




PlayPathCourier::PlayPathCourier(PlayPath* associatedPlayPath, float intervalInSeconds)
{
    this->associatedPlayPath = associatedPlayPath;

    setInterval_seconds(intervalInSeconds);
    parentPathLengthChanged();

    //update bounds (without repainting the parent)
    juce::Point<float> currentPoint = associatedPlayPath->getPointAlongPath(currentNormedDistanceFromStart);
    setBounds(static_cast<int>(currentPoint.getX() - radius),
              static_cast<int>(currentPoint.getY() - radius),
              static_cast<int>(radius * 2.0f),
              static_cast<int>(radius * 2.0f));
}
PlayPathCourier::~PlayPathCourier()
{
    stopRunning();
    associatedPlayPath = nullptr;
}

void PlayPathCourier::paint(juce::Graphics& g)
{
    g.setColour(associatedPlayPath->getFillColour().contrasting());
    g.fillEllipse(getBounds().toFloat()); //note: the size of the courier is already set to a 2*radius by 2*radius square shape
    g.setColour(associatedPlayPath->getFillColour());
    g.fillEllipse(getBounds().reduced(1).toFloat());
}
void PlayPathCourier::resized()
{ }

void PlayPathCourier::timerCallback()
{
    double previousNormedDistanceFromStart = currentNormedDistanceFromStart;
    currentNormedDistanceFromStart = std::fmod(currentNormedDistanceFromStart + normedDistancePerTick, 1.0);

    //check for collisions with any regions
    //note: since the radius of the courier is quite small, we can assume the path to be approximately linear within the courier's bounds.
    //      hence, it's alright to assume that the courier always covers an interval [pt - radius, pt + radius]. to account for looping over, modulo is used.
    juce::Range<float> startingPosition (previousNormedDistanceFromStart - normedRadius, previousNormedDistanceFromStart + normedRadius);
    juce::Range<float> currentPosition (currentNormedDistanceFromStart - normedRadius, currentNormedDistanceFromStart + normedRadius);

    //wrap values where necessary
    //(IMPORTANT NOTE: juce::Range enforces that start < end! if values are set so that start>end, the other value will be *moved* to the other value (-> new range length = 0), causing things to break in the context of this use case here!)
    if (startingPosition.getStart() < 0.0f)
    {
        //shift end first, otherwise start would be larger than end, causing juce::Range to break stuff
        startingPosition.setEnd(startingPosition.getEnd() + 1.0);

        //shift start
        startingPosition.setStart(startingPosition.getStart() + 1.0);
    }
    //if (startingPosition.getEnd() >= 1.0)
    //{
    //    startingPosition.setEnd(startingPosition.getEnd() - 1.0);
    //}
    if (currentPosition.getStart() < 0.0f)
    {
        //shift end first, otherwise start would be larger than end, causing juce::Range to break stuff
        currentPosition.setEnd(currentPosition.getEnd() + 1.0);

        //shift start
        currentPosition.setStart(currentPosition.getStart() + 1.0);
    }
    //if (currentPosition.getEnd() >= 1.0)
    //{
    //    currentPosition.setEnd(currentPosition.getEnd() - 1.0);
    //}

    //DBG("[" + juce::String(currentPosition.getStart()) + ", " + juce::String(currentPosition.getEnd()) + "]");
    associatedPlayPath->evaluateCourierPosition(this, startingPosition, currentPosition); //automatically handles signaling the regions

    //update position on screen
    updateBounds();
}
void PlayPathCourier::updateBounds()
{
    juce::Rectangle<int> previousBounds = getBounds();

    juce::Point<float> currentPoint = associatedPlayPath->getPointAlongPath(currentNormedDistanceFromStart);
    setBounds(static_cast<int>(currentPoint.getX() - radius),
              static_cast<int>(currentPoint.getY() - radius),
              static_cast<int>(radius * 2.0f),
              static_cast<int>(radius * 2.0f));

    getParentComponent()->repaint(getBounds().getUnion(previousBounds));
    //DBG("courier bounds: " + getBounds().toString());
}

float PlayPathCourier::getInterval_seconds()
{
    return intervalInSeconds;
}
void PlayPathCourier::setInterval_seconds(float newIntervalInSeconds)
{
    intervalInSeconds = newIntervalInSeconds;
    normedDistancePerTick = 1.0 / static_cast<double>(intervalInSeconds * tickRateInHz); //= (1.0 / intervalInSeconds) / tickRateInHz
    //DBG("new normedDistancePerTick: " + juce::String(normedDistancePerTick));
}

juce::Range<float> PlayPathCourier::getCurrentRange()
{
    return juce::Range<float>(currentNormedDistanceFromStart - normedRadius, currentNormedDistanceFromStart + normedRadius);
}

void PlayPathCourier::parentPathLengthChanged()
{
    if (associatedPlayPath->getPathLength() > 0)
    {
        normedRadius = radius / associatedPlayPath->getPathLength();
    }
    else
    {
        normedRadius = 0.0;
    }
    //DBG("new normedRadius: " + juce::String(normedRadius));
}

void PlayPathCourier::startRunning()
{
    startTimerHz(tickRateInHz);
}
void PlayPathCourier::stopRunning()
{
    stopTimer();
    currentNormedDistanceFromStart = 0.0;
}

void PlayPathCourier::signalRegionEntered(int regionID)
{
    currentlyIntersectedRegions.add(regionID);
}
void PlayPathCourier::signalRegionExited(int regionID)
{
    int i = 0;
    for (auto itID = currentlyIntersectedRegions.begin(); itID != currentlyIntersectedRegions.end(); ++itID)
    {
        if (*itID == regionID)
        {
            //found the region to remove
            currentlyIntersectedRegions.remove(i);
            break; //should only be contained once
        }
    }
}
juce::Array<int> PlayPathCourier::getCurrentlyIntersectedRegions()
{
    juce::Array<int> output;
    output.addArray(currentlyIntersectedRegions);

    return output;
}
void PlayPathCourier::resetCurrentlyIntersectedRegions()
{
    currentlyIntersectedRegions.clear();
}
