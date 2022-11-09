/*
  ==============================================================================

    PlayPathCourier.h
    Created: 7 Nov 2022 8:54:22am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class PlayPath;

class PlayPathCourier final : public juce::Component, juce::Timer
{
public:
    PlayPathCourier(PlayPath* associatedPlayPath, float intervalInSeconds);
    ~PlayPathCourier();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;
    void updateBounds();

    float getInterval_seconds();
    void setInterval_seconds(float newIntervalInSeconds);

    void startRunning();
    void stopRunning();

private:
    PlayPath* associatedPlayPath = nullptr;
    float currentDistanceFromStart = 0.0f;
    float pathLength = 0.0f;

    float intervalInSeconds = 0.0f;
    float tickRateInHz = 20.0f;
    float distancePerTick = 0.0f;

    static const float radius;

    bool isRunning = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayPathCourier)
};