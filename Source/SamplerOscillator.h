/*
  ==============================================================================

    SamplerOscillator.h
    Created: 9 Jun 2022 9:58:51am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
struct SamplerOscillator  : public juce::SynthesiserSound //WIP: make this a SamplerSound later. SamplerSound has a lot more useful methods and parameters
{
public:
    SamplerOscillator(juce::AudioBuffer<float> fileBuffer, double origSampleRate)
    {
        this->fileBuffer = fileBuffer;
        this->origSampleRate = origSampleRate;
    }

    ~SamplerOscillator() override
    {
        fileBuffer.clear();
    }


    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }

    juce::AudioBuffer<float> fileBuffer;
    double origSampleRate;
    //double sampleRateConversionMultiplier;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerOscillator)
};
