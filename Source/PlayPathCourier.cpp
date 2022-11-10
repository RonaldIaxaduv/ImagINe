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
    float previousNormedDistanceFromStart = currentNormedDistanceFromStart;
    currentNormedDistanceFromStart = std::fmod(currentNormedDistanceFromStart + normedDistancePerTick, 1.0);

    //check for collisions with any regions
    //note: since the radius of the courier is quite small, we can assume the path to be approximately linear within the courier's bounds.
    //      hence, it's alright to assume that the courier always covers an interval [pt - radius, pt + radius]. to account for looping over, modulo is used.
    juce::Range<float> startingPosition (previousNormedDistanceFromStart - normedRadius, previousNormedDistanceFromStart + normedRadius);
    juce::Range<float> currentPosition (currentNormedDistanceFromStart - normedRadius, currentNormedDistanceFromStart + normedRadius);

    //wrap values where necessary
    if (startingPosition.getStart() < 0.0f)
    {
        startingPosition.setStart(startingPosition.getStart() + 1.0);
    }
    if (startingPosition.getEnd() >= 1.0)
    {
        startingPosition.setEnd(startingPosition.getEnd() - 1.0);
    }
    if (currentPosition.getStart() < 0.0f)
    {
        currentPosition.setStart(currentPosition.getStart() + 1.0);
    }
    if (currentPosition.getEnd() >= 1.0)
    {
        currentPosition.setEnd(currentPosition.getEnd() - 1.0);
    }

    //DBG("[" + juce::String(currentPosition.getStart()) + ", " + juce::String(currentPosition.getEnd()) + "]");
    associatedPlayPath->evaluateCourierPosition(startingPosition, currentPosition); //automatically handles signaling the regions

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
    DBG("new normedDistancePerTick: " + juce::String(normedDistancePerTick));
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
    DBG("new normedRadius: " + juce::String(normedRadius));
}

void PlayPathCourier::startRunning()
{
    startTimerHz(tickRateInHz);
}
void PlayPathCourier::stopRunning()
{
    stopTimer();
}
