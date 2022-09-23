/*
  ==============================================================================

    Voice.cpp
    Created: 20 Sep 2022 2:20:25pm
    Author:  Aaron

  ==============================================================================
*/

#include "Voice.h"

Voice::Voice() :
    playbackMultApprox([](double semis) { return std::pow(2.0, semis / 12.0); }, -60.0, 60.0, 60 + 60 + 1), //1 point per semi should be enough
    envelope(),
    levelParameter(0.25), pitchShiftParameter(0.0)
{
    osc = nullptr;
    //advanceLfoFunction = [] {; };
}

Voice::Voice(juce::AudioSampleBuffer buffer, int origSampleRate, int regionID) :
    playbackMultApprox([](double semis) { return std::pow(2.0, semis / 12.0); }, -60.0, 60.0, 60 + 60 + 1), //1 point per semi should be enough
    envelope(),
    levelParameter(0.25), pitchShiftParameter(0.0)
{
    osc = new SamplerOscillator(buffer, origSampleRate);
    ID = regionID;
    //advanceLfoFunction = [] {; };
}

Voice::~Voice()
{
    delete osc;
    osc = nullptr;
}

//==============================================================================
void Voice::prepare(const juce::dsp::ProcessSpec& spec)
{
    DBG("preparing voice");

    //tempBlock = juce::dsp::AudioBlock<float>(heapBlock, spec.numChannels, spec.maximumBlockSize);
    setCurrentPlaybackSampleRate(spec.sampleRate);
    envelope.setSampleRate(spec.sampleRate);

    /*if (associatedLfo != nullptr)
        associatedLfo->prepare(spec);*/

        //reset modulated modifiers for the first time (afterwards, they'll be reset after every rendered sample)
    //totalLevelMultiplier = 1.0;
    //totalPitchShiftModulation = 0.0;
}

bool Voice::canPlaySound(juce::SynthesiserSound* sound)
{
    //return dynamic_cast<SamplerOscillator*> (sound) != nullptr;
    return true;
}

//==============================================================================
/*void Voice::noteStarted()
{
    auto velocity = getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat();
    auto freqHz = (float)getCurrentlyPlayingNote().getFrequencyInHertz();

    processorChain.get<osc1Index>().setFrequency(freqHz, true);
    processorChain.get<osc1Index>().setLevel(velocity);

    processorChain.get<osc2Index>().setFrequency(freqHz * 1.01f, true);
    processorChain.get<osc2Index>().setLevel(velocity);
}*/
void Voice::startNote(int midiNoteNumber, float velocity,
    juce::SynthesiserSound* sound, int /*currentPitchWheelPosition*/)
{
    currentBufferPos = 0.0;

    //auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    //auto cyclesPerSample = cyclesPerSecond / getSampleRate();

    //currentSound = dynamic_cast<SamplerOscillator*>(sound);

    envelope.noteOn();

    updateBufferPosDelta();
}

void Voice::stopNote(float /*velocity*/, bool allowTailOff)
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
/*void Voice::notePressureChanged() {}
void Voice::noteTimbreChanged() {}
void Voice::noteKeyStateChanged() {}*/

void Voice::pitchWheelMoved(int) {}
/*void Voice::notePitchbendChanged()
{
    auto freqHz = (float)getCurrentlyPlayingNote().getFrequencyInHertz();
    processorChain.get<osc1Index>().setFrequency(freqHz);
    processorChain.get<osc2Index>().setFrequency(freqHz * 1.01f);
}*/
void Voice::controllerMoved(int, int) {}

//==============================================================================
void Voice::renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
{
    if (bufferPosDelta != 0.0)
    {
        if (/*currentSound*/ osc == nullptr || /*currentSound*/osc->fileBuffer.getNumSamples() == 0)
        {
            outputBuffer.clear();
            return;
        }

        while (--numSamples >= 0)
        {
            //evaluate modulated values
            updateBufferPosDelta(); //determines pitch shift

            //double envelopeLevel = envelope.getNextEnvelopeSample();
            //double levelModulation = level * totalLevelMultiplier;
            //double gainAdjustment = envelope.getNextEnvelopeSample() * level * totalLevelMultiplier; //= envelopeLevel * levelModulation
            double gainAdjustment = envelope.getNextEnvelopeSample() * levelParameter.getModulatedValue(); //= envelopeLevel * level (with all modulations)
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
            //totalLevelMultiplier = 1.0;
            //totalPitchShiftModulation = 0.0;

            if (associatedLfo != nullptr)
                associatedLfo->advance(); //update LFO after every sample
            //advanceLfoFunction(); //re-updates the multipliers, also automatically calls RegionLfo::evaluateFrequencyModulation() beforehand to reset the Lfo freq modulation (otherwise that would require another std::function...)

            ++startSample;
        }

        if (envelope.isIdle()) //has finished playing (including release). may also occur if the sample rate suddenly changed, but in theory, that shouldn't happen I think
        {
            //stop note
            clearCurrentNote();
            bufferPosDelta = 0.0;
        }

    }
}

ModulatableMultiplicativeParameter<double>* Voice::getLevelParameter()
{
    return &levelParameter;
}
void Voice::setBaseLevel(double newLevel)
{
    //level = newLevel;
    levelParameter.setBaseValue(newLevel);
}
double Voice::getBaseLevel()
{
    //return level;
    return levelParameter.getBaseValue();
}
//void Voice::modulateLevel(double newMultiplier)
//{
//    //totalLevelMultiplier is reset after every rendered sample
//    //it's done this way since LFOs can manipulate voices of other regions, not only that of their own
//    totalLevelMultiplier *= newMultiplier;
//}

ModulatableAdditiveParameter<double>* Voice::getPitchShiftParameter()
{
    return &pitchShiftParameter;
}
void Voice::setBasePitchShift(double newPitchShift)
{
    //pitchShiftBase = newPitchShift;
    pitchShiftParameter.setBaseValue(newPitchShift);
}
double Voice::getBasePitchShift()
{
    //return pitchShiftBase;
    return pitchShiftParameter.getBaseValue();
}
//void Voice::modulatePitchShift(double semitonesToAdd)
//{
//    totalPitchShiftModulation += semitonesToAdd;
//}
void Voice::updateBufferPosDelta()
{
    if (osc != nullptr) //(currentSound != nullptr)
    {
        bufferPosDelta = osc->origSampleRate / getSampleRate(); //normal playback speed

        //double modulationSemis = pitchShiftBase + totalPitchShiftModulation;
        double modulationSemis = pitchShiftParameter.getModulatedValue(); //= pitch shift with all modulations

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

//void Voice::setLfoAdvancer(std::function<void()> newAdvanceLfoFunction)
//{
//    advanceLfoFunction = newAdvanceLfoFunction;
//}
void Voice::setLfo(RegionLfo* newAssociatedLfo)
{
    associatedLfo = newAssociatedLfo;
}

int Voice::getID()
{
    return ID;
}

DahdsrEnvelope* Voice::getEnvelope()
{
    return &envelope;
}