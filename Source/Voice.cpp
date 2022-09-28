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

    //inititalise states
    states[static_cast<int>(VoiceStateIndex::unprepared)] = static_cast<VoiceState*>(new VoiceState_Unprepared(*this));
    states[static_cast<int>(VoiceStateIndex::noWavefile_noLfo)] = static_cast<VoiceState*>(new VoiceState_NoWavefile_NoLfo(*this));
    states[static_cast<int>(VoiceStateIndex::noWavefile_Lfo)] = static_cast<VoiceState*>(new VoiceState_NoWavefile_Lfo(*this));
    states[static_cast<int>(VoiceStateIndex::stopped_noLfo)] = static_cast<VoiceState*>(new VoiceState_Stopped_NoLfo(*this));
    states[static_cast<int>(VoiceStateIndex::stopped_Lfo)] = static_cast<VoiceState*>(new VoiceState_Stopped_Lfo(*this));
    states[static_cast<int>(VoiceStateIndex::playable_noLfo)] = static_cast<VoiceState*>(new VoiceState_Playable_NoLfo(*this));
    states[static_cast<int>(VoiceStateIndex::playable_Lfo)] = static_cast<VoiceState*>(new VoiceState_Playable_Lfo(*this));
    
    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];
}

Voice::Voice(juce::AudioSampleBuffer buffer, int origSampleRate, int regionID) :
    Voice::Voice()
{
    osc = new SamplerOscillator(buffer, origSampleRate);
    ID = regionID;
}

Voice::~Voice()
{
    delete osc;
    osc = nullptr;

    //release states
    currentState = nullptr;
    delete states[static_cast<int>(VoiceStateIndex::unprepared)];
    delete states[static_cast<int>(VoiceStateIndex::noWavefile_noLfo)];
    delete states[static_cast<int>(VoiceStateIndex::noWavefile_Lfo)];
    delete states[static_cast<int>(VoiceStateIndex::stopped_noLfo)];
    delete states[static_cast<int>(VoiceStateIndex::stopped_Lfo)];
    delete states[static_cast<int>(VoiceStateIndex::playable_noLfo)];
    delete states[static_cast<int>(VoiceStateIndex::playable_Lfo)];
}

//==============================================================================
void Voice::prepare(const juce::dsp::ProcessSpec& spec)
{
    DBG("preparing voice");

    setCurrentPlaybackSampleRate(spec.sampleRate);
    envelope.setSampleRate(spec.sampleRate);

    currentState->prepared(spec.sampleRate);
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
    currentState->playableChanged(true);

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
        currentState->playableChanged(false);
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
    currentState->renderNextBlock(outputBuffer, startSample);

    //if (bufferPosDelta != 0.0)
    //{
    //    if (/*currentSound*/ osc == nullptr || /*currentSound*/osc->fileBuffer.getNumSamples() == 0)
    //    {
    //        outputBuffer.clear();
    //        return;
    //    }

    //    while (--numSamples >= 0)
    //    {
    //        //evaluate modulated values
    //        updateBufferPosDelta(); //determines pitch shift

    //        double gainAdjustment = envelope.getNextEnvelopeSample() * levelParameter.getModulatedValue(); //= envelopeLevel * level (with all modulations)
    //        for (auto i = outputBuffer.getNumChannels() - 1; i >= 0; --i)
    //        {
    //            auto currentSample = (osc->fileBuffer.getSample(i % osc->fileBuffer.getNumChannels(), (int)currentBufferPos)) * gainAdjustment;
    //            outputBuffer.addSample(i, startSample, (float)currentSample);
    //        }

    //        currentBufferPos += bufferPosDelta;
    //        if (currentBufferPos >= osc->fileBuffer.getNumSamples())
    //        {
    //            currentBufferPos -= osc->fileBuffer.getNumSamples();
    //        }

    //        if (associatedLfo != nullptr)
    //            associatedLfo->advance(); //update LFO after every sample

    //        ++startSample;
    //    }

    //    if (envelope.isIdle()) //has finished playing (including release). may also occur if the sample rate suddenly changed, but in theory, that shouldn't happen I think
    //    {
    //        //stop note
    //        clearCurrentNote();
    //        bufferPosDelta = 0.0;
    //    }

    //}
}

void Voice::renderNextBlock_empty()
{
    //nothing set -> nothing happens
}
void Voice::renderNextBlock_onlyLfo()
{
    //bufferPosDelta = 0.0 or no wave set -> needn't render sound

    associatedLfo->advance();
}
void Voice::renderNextBlock_wave(juce::AudioSampleBuffer& outputBuffer, int sampleIndex)
{
    //while (--numSamples >= 0) //only rendered one sample at the time now
    //{

    //evaluate modulated values
    updateBufferPosDelta(); //determines pitch shift

    double gainAdjustment = envelope.getNextEnvelopeSample() * levelParameter.getModulatedValue(); //= envelopeLevel * level (with all modulations)
    for (auto i = outputBuffer.getNumChannels() - 1; i >= 0; --i)
    {
        auto currentSample = (osc->fileBuffer.getSample(i % osc->fileBuffer.getNumChannels(), (int)currentBufferPos)) * gainAdjustment;
        outputBuffer.addSample(i, sampleIndex, (float)currentSample);
    }

    currentBufferPos += bufferPosDelta;
    if (currentBufferPos >= osc->fileBuffer.getNumSamples())
    {
        currentBufferPos -= osc->fileBuffer.getNumSamples();
    }

    //no LFO set -> needn't advance

    //}

    if (envelope.isIdle()) //has finished playing (including release). may also occur if the sample rate suddenly changed, but in theory, that shouldn't happen I think
    {
        //stop note
        clearCurrentNote();
        bufferPosDelta = 0.0;
        currentState->playableChanged(false);
    }
}
void Voice::renderNextBlock_waveAndLfo(juce::AudioSampleBuffer& outputBuffer, int sampleIndex)
{
    //while (--numSamples >= 0) //only rendered one sample at the time now
    //{
    
    //evaluate modulated values
    updateBufferPosDelta(); //determines pitch shift

    double gainAdjustment = envelope.getNextEnvelopeSample() * levelParameter.getModulatedValue(); //= envelopeLevel * level (with all modulations)
    for (auto i = outputBuffer.getNumChannels() - 1; i >= 0; --i)
    {
        auto currentSample = (osc->fileBuffer.getSample(i % osc->fileBuffer.getNumChannels(), (int)currentBufferPos)) * gainAdjustment;
        outputBuffer.addSample(i, sampleIndex, (float)currentSample);
    }

    currentBufferPos += bufferPosDelta;
    if (currentBufferPos >= osc->fileBuffer.getNumSamples())
    {
        currentBufferPos -= osc->fileBuffer.getNumSamples();
    }

    associatedLfo->advance();

    //}

    if (envelope.isIdle()) //has finished playing (including release). may also occur if the sample rate suddenly changed, but in theory, that shouldn't happen I think
    {
        //stop note
        clearCurrentNote();
        bufferPosDelta = 0.0;
        currentState->playableChanged(false);
    }
}

//==============================================================================
void Voice::transitionToState(VoiceStateIndex stateToTransitionTo)
{
    bool nonInstantStateFound = false;

    while (!nonInstantStateFound) //check if any states can be skipped (might be the case depending on what has been prepared already)
    {
        switch (stateToTransitionTo)
        {
        case VoiceStateIndex::unprepared:
            if (getSampleRate() > 0.0)
            {
                stateToTransitionTo = VoiceStateIndex::stopped_noLfo;
                continue;
            }

            nonInstantStateFound = true;
            DBG("Voice unprepared");
            break;

        case VoiceStateIndex::noWavefile_noLfo:
            if (osc != nullptr && osc->fileBuffer.getNumSamples() > 0)
            {
                if (associatedLfo != nullptr)
                {
                    stateToTransitionTo = VoiceStateIndex::stopped_Lfo;
                    continue;
                }
                else
                {
                    stateToTransitionTo = VoiceStateIndex::stopped_noLfo;
                    continue;
                }
            }
            if (associatedLfo != nullptr)
            {
                stateToTransitionTo = VoiceStateIndex::noWavefile_Lfo;
                continue;
            }

            nonInstantStateFound = true;
            DBG("Voice without wavefile and without LFO");
            break;

        case VoiceStateIndex::noWavefile_Lfo:
            if (osc != nullptr && osc->fileBuffer.getNumSamples() > 0)
            {
                if (associatedLfo == nullptr)
                {
                    stateToTransitionTo = VoiceStateIndex::stopped_noLfo;
                    continue;
                }
                else
                {
                    stateToTransitionTo = VoiceStateIndex::stopped_Lfo;
                    continue;
                }
            }
            if (associatedLfo == nullptr)
            {
                stateToTransitionTo = VoiceStateIndex::noWavefile_noLfo;
                continue;
            }

            nonInstantStateFound = true;
            DBG("Voice without wavefile and with LFO");
            break;

        case VoiceStateIndex::stopped_noLfo:
            nonInstantStateFound = true;
            DBG("Voice stopped and without LFO");
            break;

        case VoiceStateIndex::stopped_Lfo:
            nonInstantStateFound = true;
            DBG("Voice stopped and with LFO");
            break;

        case VoiceStateIndex::playable_noLfo:
            nonInstantStateFound = true;
            DBG("Voice playable and without LFO");
            break;

        case VoiceStateIndex::playable_Lfo:
            nonInstantStateFound = true;
            DBG("Voice playable and with LFO");
            break;

        default:
            throw std::exception("unhandled state index");
        }
    }

    //finally, update the current state index
    currentStateIndex = stateToTransitionTo;
    currentState = states[static_cast<int>(currentStateIndex)];
}

//==============================================================================
ModulatableMultiplicativeParameter<double>* Voice::getLevelParameter()
{
    return &levelParameter;
}
void Voice::setBaseLevel(double newLevel)
{
    levelParameter.setBaseValue(newLevel);
}
double Voice::getBaseLevel()
{
    return levelParameter.getBaseValue();
}

ModulatableAdditiveParameter<double>* Voice::getPitchShiftParameter()
{
    return &pitchShiftParameter;
}
void Voice::setBasePitchShift(double newPitchShift)
{
    pitchShiftParameter.setBaseValue(newPitchShift);
}
double Voice::getBasePitchShift()
{
    return pitchShiftParameter.getBaseValue();
}

void Voice::updateBufferPosDelta()
{
    currentState->updateBufferPosDelta();
}
void Voice::updateBufferPosDelta_NotPlayable()
{
    bufferPosDelta = 0.0;
    //currentState->playableChanged(false);
    
    //DBG("bufferPosDelta: " + juce::String(bufferPosDelta));
}
void Voice::updateBufferPosDelta_Playable()
{
    bufferPosDelta = osc->origSampleRate / getSampleRate(); //normal playback speed

    double modulationSemis = pitchShiftParameter.getModulatedValue(); //= pitch shift with all modulations

    //for positive numbers: doubled per octave; for negative numbers: halved per octave
    //bufferPosDelta *= //std::pow(2.0, modulationSemis / 12.0); //exact calculation -> very slow!
    bufferPosDelta *= playbackMultApprox(modulationSemis); //approximation -> faster (one power function per sample per voice would be madness efficiency-wise)
    //currentState->playableChanged(true);

    //DBG("bufferPosDelta: " + juce::String(bufferPosDelta));
}

void Voice::setLfo(RegionLfo* newAssociatedLfo)
{
    associatedLfo = newAssociatedLfo;
    currentState->associatedLfoChanged(associatedLfo);
}

int Voice::getID()
{
    return ID;
}

DahdsrEnvelope* Voice::getEnvelope()
{
    return &envelope;
}