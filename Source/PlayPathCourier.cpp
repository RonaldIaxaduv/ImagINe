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
const float PlayPathCourier::radius = 3.0f;




PlayPathCourier::PlayPathCourier(PlayPath* associatedPlayPath, float intervalInSeconds)
{
    this->associatedPlayPath = associatedPlayPath;
    pathLength = associatedPlayPath->getPathLength();

    setInterval_seconds(intervalInSeconds);

    updateBounds();
}
PlayPathCourier::~PlayPathCourier()
{
    stopRunning();
    associatedPlayPath = nullptr;
}

void PlayPathCourier::paint(juce::Graphics& g)
{
    g.setColour(associatedPlayPath->getFillColour());
    g.fillEllipse(getBounds().toFloat()); //note: the size of the courier is already set to 2*radius by 2*radius
}
void PlayPathCourier::resized()
{ }

void PlayPathCourier::timerCallback()
{
    float previousDistanceFromStart = currentDistanceFromStart;
    currentDistanceFromStart = std::fmod(currentDistanceFromStart + distancePerTick, pathLength);

    //check for collisions with any regions
    //note: since the radius of the courier is quite small, we can assume the path to be approximately linear within the courier's bounds.
    //      hence, it's alright to assume that the courier always covers an interval [pt - radius, pt + radius]. to account for looping over, modulo is used.
    juce::Range<float> startingPosition (previousDistanceFromStart - radius, previousDistanceFromStart + radius);
    juce::Range<float> currentPosition (currentDistanceFromStart - radius, currentDistanceFromStart + radius);

    //wrap values where necessary
    if (startingPosition.getStart() < 0.0f)
    {
        startingPosition.setStart(startingPosition.getStart() + pathLength);
    }
    if (startingPosition.getEnd() >= pathLength)
    {
        startingPosition.setEnd(startingPosition.getEnd() - pathLength);
    }
    if (currentPosition.getStart() < 0.0f)
    {
        currentPosition.setStart(currentPosition.getStart() + pathLength);
    }
    if (currentPosition.getEnd() >= pathLength)
    {
        currentPosition.setEnd(currentPosition.getEnd() - pathLength);
    }

    associatedPlayPath->evaluateCourierPosition(startingPosition, currentPosition); //automatically handles signaling the regions

    //update position on screen
    updateBounds();
}
void PlayPathCourier::updateBounds()
{
    juce::Point<float> currentPoint = associatedPlayPath->getPointAlongPath(currentDistanceFromStart);
    setBounds(currentPoint.getX() - radius, currentPoint.getY() - radius, radius * 2, radius * 2); //also repaints
}

float PlayPathCourier::getInterval_seconds()
{
    return intervalInSeconds;
}
void PlayPathCourier::setInterval_seconds(float newIntervalInSeconds)
{
    intervalInSeconds = newIntervalInSeconds;
    distancePerTick = pathLength / (intervalInSeconds * tickRateInHz); //= (pathLength / intervalInSeconds) / tickRateInHz
}

void PlayPathCourier::startRunning()
{
    startTimerHz(tickRateInHz);
}
void PlayPathCourier::stopRunning()
{
    stopTimer();
}
