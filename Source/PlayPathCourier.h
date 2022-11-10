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

    void parentPathLengthChanged();

    void startRunning();
    void stopRunning();

    static const float radius;

private:
    PlayPath* associatedPlayPath = nullptr;
    double currentNormedDistanceFromStart = 0.0;

    float intervalInSeconds = 0.0f;
    float tickRateInHz = 20.0f;
    double normedDistancePerTick = 0.0;

    double normedRadius = 0.0;

    bool isRunning = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayPathCourier)
};