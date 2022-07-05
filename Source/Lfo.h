/*
  ==============================================================================

    Lfo.h
    Created: 29 Jun 2022 9:23:48am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/// <summary>
/// Continuous low-frequency oscillator (LFO) that modulates a parameter using a passed function.
/// </summary>
class Lfo
{
public:
    Lfo() :
        waveTable(0, 0),
        playbackMultApprox([](float semis) { return std::powf(2.0f, semis / 12.0f); }, -36.0f, 36.0f, 36 + 36 + 1) //1 point per semi should be enough, as should 36 semitones in total (maximum mult: 1/8...8)
    { }

    Lfo(const juce::AudioBuffer<float>& waveTable, const std::function<void(float lfoValue)>& modulationFunction) :
        waveTable(waveTable),
        modulationFunction(modulationFunction),
        playbackMultApprox([](float semis) { return std::powf(2.0f, semis / 12.0f); }, -36.0f, 36.0f, 36 + 36 + 1) //1 point per semi should be enough, as should 36 semitones in total (maximum mult: 1/8...8)
    {
        updateModulatedParameter();
    }

    ~Lfo()
    {
        waveTable.setSize(0, 0);
    }

    void initialise(const juce::AudioBuffer<float>& waveTable, const std::function<void(float lfoValue)>& modulationFunction)
    {
        this->waveTable = waveTable;
        this->modulationFunction = modulationFunction;

        updateModulatedParameter();
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        setBaseFrequency(baseFrequency);
        resetPhase(false);
    }

    void setBaseFrequency(float newBaseFrequency)
    {
        baseFrequency = newBaseFrequency;
        evaluateFrequencyModulation();

        //if (waveTable.getNumSamples() > 0)
        //{
        //    baseFrequency = newBaseFrequency;

        //    //if (baseFrequency > 0.0f)
        //    //{
        //    //    //auto samplesPerCycle = static_cast<float>(sampleRate) / baseFrequency; //number of samples of the waveTable that will be played during one full cycle at the current frequency
        //    //    //tablePosDelta = waveTable.getNumSamples() / samplesPerCycle;
        //    //    
        //    //}
        //    //else
        //    //{
        //    //    tablePosDelta = 0.0f;
        //    //}
        //}
        //else
        //{
        //    tablePosDelta = 0.0f;
        //}
    }
    float getBaseFrequency()
    {
        return baseFrequency;
    }
    void modulateFrequency(float semitonesToAdd)
    {
        totalFrequencyModulation += semitonesToAdd;
    }
    void evaluateFrequencyModulation()
    {
        auto cyclesPerSample = baseFrequency / static_cast<float>(sampleRate); //0.0f if frequency is 0
        tablePosDelta = static_cast<float>(waveTable.getNumSamples()) * cyclesPerSample; //0.0f if waveTable empty
        tablePosDelta *= playbackMultApprox(totalFrequencyModulation); //approximation -> faster (one power function per sample per LFO would be madness efficiency-wise)
        
        totalFrequencyModulation = 0.0; //automatically reset modulation (would be more complicated to do from the voice processor)
    }

    /// <summary>
    /// advances the LFO's phase by 1 sample
    /// </summary>
    void virtual advance()
    {
        if (waveTable.getNumSamples() > 0)
        {
            currentTablePos += tablePosDelta;
            if (static_cast<int>(currentTablePos) >= waveTable.getNumSamples() - 1) //-1 because the last sample is equal to the first
            {
                currentTablePos -= static_cast<float>(waveTable.getNumSamples() - 1);
            }

            updateModulatedParameter();
        }
    }

    void virtual resetPhase(bool updateParameter = true)
    {
        if (waveTable.getNumSamples() > 0)
        {
            currentTablePos = 0.0f;

            if (updateParameter)
                updateModulatedParameter();
        }
    }

    /// <summary>
    /// Sets the relative phase of the LFO.
    /// </summary>
    /// <param name="relativeTablePos">Value between 0.0f and 1.0f, where 0.0f is the first sample of the wave table, and 1.0f is the last.</param>
    void setPhase(float relativeTablePos)
    {
        if (waveTable.getNumSamples() > 0)
        {
            if (relativeTablePos < 0.0f || relativeTablePos > 1.0f)
                throw std::invalid_argument("relativeTablePos was out of range!");

            currentTablePos = (static_cast<float>(waveTable.getNumSamples() - 1)) * relativeTablePos;

            updateModulatedParameter();
        }
    }

    /// <summary>
    /// Gets the relative phase of the LFO.
    /// </summary>
    /// <returns>Value between 0.0f and 1.0f, where 0.0f is the first sample of the wave table, and 1.0f is the last.</returns>
    float getPhase()
    {
        if (waveTable.getNumSamples() > 0)
        {
            return currentTablePos / static_cast<float>(waveTable.getNumSamples());
        }
        else
        {
            return 0.0f;
        }
    }

    void setModulatedParameterID(int newID)
    {
        modulatedParameterID = newID;
    }
    int getModulatedParameterID()
    {
        return modulatedParameterID;
    }

protected:
    double sampleRate = 0.0;

    float baseFrequency = 0.0f;
    float totalFrequencyModulation = 0.0; //in semitones (can also be negative)
    juce::dsp::LookupTableTransform<float> playbackMultApprox;

    juce::AudioBuffer<float> waveTable; //determines the LFO's shape
    float tablePosDelta = 0.0f;
    float currentTablePos = 0.0f;

    std::function<void(float lfoValue)> modulationFunction;

    int modulatedParameterID = -1;

    virtual void updateModulatedParameter()
    {
        if (waveTable.getNumSamples() > 0) //WIP: might be better to create a separate LFO state (interface) for when the wavetable is empty. otherwise, this is called during every sample and unnecessarily takes up resources that way
        {
            auto* samples = waveTable.getReadPointer(0);

            int sampleIndex1 = static_cast<int>(currentTablePos);
            int sampleIndex2 = sampleIndex1 + 1;
            float frac = currentTablePos - static_cast<float>(sampleIndex1);

            auto sample = samples[sampleIndex1] + frac * (samples[sampleIndex2] - samples[sampleIndex1]); //interpolate between samples (good especially at slower freqs)

            modulationFunction(sample);
        }
    }
};