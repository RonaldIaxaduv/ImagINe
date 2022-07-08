/*
  ==============================================================================

    Voice.h
    Created: 9 Jun 2022 9:58:36am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SamplerOscillator.h"
#include "DahdsrEnvelope.h"


//==============================================================================
/*
*/
struct Voice  : public juce::SynthesiserVoice
{
    Voice() :
        playbackMultApprox([](double semis) { return std::pow(2.0, semis / 12.0); }, -60.0, 60.0, 60 + 60 + 1), //1 point per semi should be enough
        envelope()
    {
        osc = nullptr;
        advanceLfoFunction = [] {; };
    }

    Voice(juce::AudioSampleBuffer buffer, int origSampleRate, int regionID) :
        playbackMultApprox([](double semis) { return std::pow(2.0, semis / 12.0); }, -60.0, 60.0, 60 + 60 + 1), //1 point per semi should be enough
        envelope()
    {
        osc = new SamplerOscillator(buffer, origSampleRate);
        ID = regionID;
        advanceLfoFunction = [] {; };
    }

    ~Voice() override
    {
        delete osc;
        osc = nullptr;
    }

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        DBG("preparing voice");

        //tempBlock = juce::dsp::AudioBlock<float>(heapBlock, spec.numChannels, spec.maximumBlockSize);
        setCurrentPlaybackSampleRate(spec.sampleRate);
        envelope.setSampleRate(spec.sampleRate);

        /*if (associatedLfo != nullptr)
            associatedLfo->prepare(spec);*/

        //reset modulated modifiers for the first time (afterwards, they'll be reset after every rendered sample)
        totalLevelMultiplier = 1.0;
        totalPitchShiftModulation = 0.0;
    }

    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        //return dynamic_cast<SamplerOscillator*> (sound) != nullptr;
        return true;
    }

    //==============================================================================
    /*void noteStarted() override
    {
        auto velocity = getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat();
        auto freqHz = (float)getCurrentlyPlayingNote().getFrequencyInHertz();

        processorChain.get<osc1Index>().setFrequency(freqHz, true);
        processorChain.get<osc1Index>().setLevel(velocity);

        processorChain.get<osc2Index>().setFrequency(freqHz * 1.01f, true);
        processorChain.get<osc2Index>().setLevel(velocity);
    }*/
    void startNote(int midiNoteNumber, float velocity,
        juce::SynthesiserSound* sound, int /*currentPitchWheelPosition*/) override
    {
        currentBufferPos = 0.0;

        //auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        //auto cyclesPerSample = cyclesPerSecond / getSampleRate();

        //currentSound = dynamic_cast<SamplerOscillator*>(sound);

        envelope.noteOn();
        
        updateBufferPosDelta();
    }

    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            /*clearCurrentNote();
            bufferPosDelta = 0.0;*/
            envelope.noteOff();
        }
        else
        {
            clearCurrentNote();
            bufferPosDelta = 0.0;
            envelope.forceStop();
        }
    }

    //==============================================================================
    /*void notePressureChanged() override {}
    void noteTimbreChanged()   override {}
    void noteKeyStateChanged() override {}*/

    void pitchWheelMoved(int) override {}
    /*void notePitchbendChanged() override
   {
       auto freqHz = (float)getCurrentlyPlayingNote().getFrequencyInHertz();
       processorChain.get<osc1Index>().setFrequency(freqHz);
       processorChain.get<osc2Index>().setFrequency(freqHz * 1.01f);
   }*/
    void controllerMoved(int, int) override {}

    //==============================================================================
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (bufferPosDelta != 0.0)
        {
            if (/*currentSound*/ osc == nullptr || /*currentSound*/osc->fileBuffer.getNumSamples() == 0)
            {
                outputBuffer.clear();
                return;
            }

            bool hasBeenReleasing = envelope.isReleasing();

            while (--numSamples >= 0)
            {
                //evaluate modulated values
                updateBufferPosDelta(); //determines pitch shift

                //double envelopeLevel = envelope.getNextEnvelopeSample();
                //double levelModulation = level * totalLevelMultiplier;
                double gainAdjustment = envelope.getNextEnvelopeSample() * level * totalLevelMultiplier; //= envelopeLevel * levelModulation
                for (auto i = outputBuffer.getNumChannels() - 1; i >= 0; --i)
                {
                    auto currentSample = (osc->fileBuffer.getSample(i % osc->fileBuffer.getNumChannels(), (int)currentBufferPos)) * gainAdjustment;
                    outputBuffer.addSample(i, startSample, (float)currentSample);
                }

                currentBufferPos += bufferPosDelta;
                if (currentBufferPos >= osc->fileBuffer.getNumSamples())
                {
                    currentBufferPos -= osc->fileBuffer.getNumSamples();
                }

                //reset parameter modulator values after each sample
                totalLevelMultiplier = 1.0;
                totalPitchShiftModulation = 0.0;

                /*if (associatedLfo != nullptr)
                    associatedLfo->advance();*/ //update LFO after every sample
                advanceLfoFunction(); //re-updates the multipliers, also automatically calls RegionLfo::evaluateFrequencyModulation() beforehand to reset the Lfo freq modulation (otherwise that would require another std::function...)

                ++startSample;
            }

            if (hasBeenReleasing && envelope.isIdle()) //has finished releasing
            {
                //stop note
                clearCurrentNote();
                bufferPosDelta = 0.0;
            }

        }
    }

    void setBaseLevel(double newLevel)
    {
        level = newLevel;
    }
    double getBaseLevel()
    {
        return level;
    }
    void modulateLevel(double newMultiplier)
    {
        //totalLevelMultiplier is reset after every rendered sample
        //it's done this way since LFOs can manipulate voices of other regions, not only that of their own
        totalLevelMultiplier *= newMultiplier;
    }

    void setBasePitchShift(double newPitchShift)
    {
        pitchShiftBase = newPitchShift;
    }
    double getBasePitchShift()
    {
        return pitchShiftBase;
    }
    void modulatePitchShift(double semitonesToAdd)
    {
        totalPitchShiftModulation += semitonesToAdd;
    }
    void updateBufferPosDelta()
    {
        if (osc != nullptr) //(currentSound != nullptr)
        {
            bufferPosDelta = osc->origSampleRate / getSampleRate(); //normal playback speed

            double modulationSemis = pitchShiftBase + totalPitchShiftModulation;
            
            //for positive numbers: doubled per octave; for negative numbers: halved per octave
            //bufferPosDelta *= //std::pow(2.0, modulationSemis / 12.0);
            bufferPosDelta *= playbackMultApprox(modulationSemis); //approximation -> faster (one power function per sample per voice would be madness efficiency-wise)
        }
        else
        {
            bufferPosDelta = 0.0;
        }
        //DBG("bufferPosDelta: " + juce::String(bufferPosDelta));
    }

    void setLfoFreqModFunction(std::function<void(float semitonesToAdd)> newLfoFreqModFunction)
    {
        lfoFreqModFunction = newLfoFreqModFunction;
    }
    std::function<void(float semitonesToAdd)> getLfoFreqModFunction()
    {
        return lfoFreqModFunction;
    }

    void setLfoAdvancer(std::function<void()> newAdvanceLfoFunction)
    {
        advanceLfoFunction = newAdvanceLfoFunction;
    }

    int getID()
    {
        return ID;
    }

    DahdsrEnvelope* getEnvelope()
    {
        return &envelope;
    }

private:
    int ID = -1;

    double currentBufferPos = 0.0, bufferPosDelta = 0.0;

    double level = 0.25;
    double totalLevelMultiplier = 1.0;

    double pitchShiftBase = 0.0; //in semitones difference to the normal bufferPosDelta
    double totalPitchShiftModulation = 0.0; //in semitones (can also be negative)
    juce::dsp::LookupTableTransform<double> playbackMultApprox; //calculating the resulting playback speed from the semitones of pitch shift needs the power function (2^(semis/12)) which is expensive. this pre-calculates values within a certain range (-60...+60)

    std::function<void(float semitonesToAdd)> lfoFreqModFunction; //used to modulate the frequency of the LFO of the region that this voice belongs to

    SamplerOscillator* osc;

    //RegionLfo* associatedLfo = nullptr;
    //int associatedLfoIndex = -1; //must be handled like this and NOT via a RegionLfo pointer, because otherwise the RegionLfo class and Voice class would cross-reference each other -> not compilable
    std::function<void()> advanceLfoFunction; //nvm, referencing AudioEngine would cause the same problem. guess it's gotta be a function then. noooot messy at all xD

    DahdsrEnvelope envelope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Voice)
};