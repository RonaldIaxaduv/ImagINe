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
    levelParameter(0.25), pitchShiftParameter(0.0),
    playbackPositionStartParameter(0.0), playbackPositionIntervalParameter(1.0, 0.001), playbackPositionCurrentParameter(0.0),
    filterPositionParameter(20000.0, 20.0)
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
    int initialisedStates = 7;
    jassert(initialisedStates == static_cast<int>(VoiceStateIndex::StateIndexCount));

    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];

    pitchQuantisationFuncPt = &Voice::getQuantisedPitch_continuous; //default: no quantisation (cheapest)
    setPitchQuantisationScale_minor(); //set to minor scale by default (will be overwritten once the player chooses a different quantisation method than continous, but it's safer to initialise the array just in case)

    filter.parameters->setCutOffFrequency(48000.0, 22050.0); //sample rate will be set again later during preparation
    filter.parameters->type = juce::dsp::StateVariableFilter::StateVariableFilterType::lowPass; //WIP: maybe make other types avaible too
    filter.snapToZero(); //the filter will be doing sample-by-sample processing

    //DBG("init base level: " + juce::String(levelParameter.getBaseValue()));
    //DBG("init base playback pos: " + juce::String(playbackPositionParameter.getBaseValue()));
}

Voice::Voice(int regionID) :
    Voice::Voice()
{
    ID = regionID;
}

Voice::Voice(juce::AudioSampleBuffer buffer, int origSampleRate, int regionID) :
    Voice::Voice(regionID)
{
    setOsc(buffer, origSampleRate);
}

Voice::~Voice()
{
    DBG("destroying Voice...");

    //unsubscribe all modulators (otherwise, access violations will happen when the window is closed while regions are playing!)
    int unsubscribedModulators = 0;
    
    auto levelModulators = levelParameter.getModulators();
    for (auto* it = levelModulators.begin(); it != levelModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getID());
    }
    unsubscribedModulators++;

    auto pitchModulators = pitchShiftParameter.getModulators();
    for (auto* it = pitchModulators.begin(); it != pitchModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getID());
    }
    unsubscribedModulators++;

    auto playbackPositionStartModulators = playbackPositionStartParameter.getModulators();
    for (auto* it = playbackPositionStartModulators.begin(); it != playbackPositionStartModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getID());
    }
    unsubscribedModulators++;

    auto playbackPositionIntervalModulators = playbackPositionIntervalParameter.getModulators();
    for (auto* it = playbackPositionIntervalModulators.begin(); it != playbackPositionIntervalModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getID());
    }
    unsubscribedModulators++;

    auto playbackPositionCurrentModulators = playbackPositionCurrentParameter.getModulators();
    for (auto* it = playbackPositionCurrentModulators.begin(); it != playbackPositionCurrentModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getID());
    }
    unsubscribedModulators++;

    auto filterPositionModulators = filterPositionParameter.getModulators();
    for (auto* it = filterPositionModulators.begin(); it != filterPositionModulators.end(); it++)
    {
        (*it)->removeRegionModulation(getID());
    }
    unsubscribedModulators++;

    jassert(unsubscribedModulators == 6);

    //release osc
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
    int deletedStates = 7;
    jassert(deletedStates == static_cast<int>(VoiceStateIndex::StateIndexCount));

    DBG("Voice destroyed.");
}

//==============================================================================
void Voice::prepare(const juce::dsp::ProcessSpec& spec)
{
    DBG("preparing voice");

    setCurrentPlaybackSampleRate(spec.sampleRate);
    envelope.setSampleRate(spec.sampleRate);

    filter.prepare(spec);

    currentState->prepared(spec.sampleRate);
}

void Voice::setOsc(juce::AudioSampleBuffer buffer, int origSampleRate)
{
    if (osc == nullptr)
    {
        osc = new SamplerOscillator(buffer, origSampleRate);
    }
    else
    {
        osc->fileBuffer = buffer;
        osc->origSampleRate = origSampleRate;
    }
    currentBufferPos = 0.0;
    currentState->wavefileChanged(buffer.getNumSamples());
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
    //currentBufferPos = 0.0; //if voices restarted while they are still tailing off, it be a rather unintuitive behaviour. instead, only reset the current buffer position if the voice has ended or the buffer changes.

    //auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    //auto cyclesPerSample = cyclesPerSecond / getSampleRate();

    //currentSound = dynamic_cast<SamplerOscillator*>(sound);

    envelope.noteOn();
    if (associatedLfo != nullptr)
    {
        associatedLfo->resetSamplesUntilUpdate(); //feels more consistent and intuitive
    }
    currentState->playableChanged(true);

    updateBufferPosDelta();

    //DBG("note on. current state index: " + juce::String(static_cast<int>(currentStateIndex)));
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
    evaluateBufferPosModulation(); //advances currentBufferPos if required

    //calculate buffer position
    double effectivePhase = static_cast<double>(currentBufferPos / static_cast<float>(osc->fileBuffer.getNumSamples())); //convert currentTablePos to currentPhase
    //effectivePhase = std::fmod(effectivePhase, playbackPositionIntervalParameter.getModulatedValue()); //convert to new interval while preserving deltaTablePos (i.e. the frequency)!
    effectivePhase = std::fmod(effectivePhase, playbackPositionIntervalParameter.getModulatedValue()); //convert to new interval while preserving deltaTablePos (i.e. the frequency)! note that playbackPositionIntervalParameter is a capped parameter that cannot become 0.0
    jassert(!isnan(effectivePhase));
    effectivePhase = std::fmod(effectivePhase + playbackPositionStartParameter.getModulatedValue(), 1.0); //shift the starting phase from 0 to the value stated by playbackPositionStartParameter and wrap, so that the value stays within [0,1) (i.e. within wavetable later on)
    float effectiveBufferPos = static_cast<float>(effectivePhase) * static_cast<float>(osc->fileBuffer.getNumSamples() - 1); //convert phase back to index within wavetable (-1 at the end to ensure that floating-point rounding won't let the variable take on out-of-range values!)
    //^- see updateCurrentValues method in RegionLfo for some more details

    //pre-calculate volume of the next sample (for all channels)
    double gainAdjustment = envelope.getNextEnvelopeSample() * levelParameter.getModulatedValue(); //= envelopeLevel * level (with all modulations)

    //update filter position
    filter.parameters->setCutOffFrequency(getSampleRate(), filterPositionParameter.getModulatedValue());

    //calculate sample(s)
    for (auto i = outputBuffer.getNumChannels() - 1; i >= 0; --i)
    {
        //auto currentSample = (osc->fileBuffer.getSample(i % osc->fileBuffer.getNumChannels(), (int)currentBufferPos)) * gainAdjustment;
        auto currentSample = (osc->fileBuffer.getSample(i % osc->fileBuffer.getNumChannels(), static_cast<int>(effectiveBufferPos))) * gainAdjustment;
        currentSample = filter.processSample(currentSample);
        outputBuffer.addSample(i, sampleIndex, (float)currentSample);
    }

    //currentBufferPos += bufferPosDelta;
    //if (currentBufferPos >= osc->fileBuffer.getNumSamples())
    //{
    //    currentBufferPos -= osc->fileBuffer.getNumSamples();
    //}

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
    evaluateBufferPosModulation(); //advances currentBufferPos if required

    //calculate buffer position
    double effectivePhase = static_cast<double>(currentBufferPos / static_cast<float>(osc->fileBuffer.getNumSamples())); //convert currentTablePos to currentPhase
    //effectivePhase = std::fmod(effectivePhase, playbackPositionIntervalParameter.getModulatedValue()); //convert to new interval while preserving deltaTablePos (i.e. the frequency)!
    effectivePhase = std::fmod(effectivePhase, playbackPositionIntervalParameter.getModulatedValue()); //convert to new interval while preserving deltaTablePos (i.e. the frequency)! note that playbackPositionIntervalParameter is a capped parameter that cannot become 0.0
    jassert(!isnan(effectivePhase));
    effectivePhase = std::fmod(effectivePhase + playbackPositionStartParameter.getModulatedValue(), 1.0); //shift the starting phase from 0 to the value stated by playbackPositionStartParameter and wrap, so that the value stays within [0,1) (i.e. within wavetable later on)
    float effectiveBufferPos = static_cast<float>(effectivePhase) * static_cast<float>(osc->fileBuffer.getNumSamples() - 1); //convert phase back to index within wavetable (-1 at the end to ensure that floating-point rounding won't let the variable take on out-of-range values!)
    //^- see updateCurrentValues method in RegionLfo for some more details

    //pre-calculate volume of the next sample (for all channels)
    double gainAdjustment = envelope.getNextEnvelopeSample() * levelParameter.getModulatedValue(); //= envelopeLevel * level (with all modulations)

    //update filter position
    filter.parameters->setCutOffFrequency(getSampleRate(), filterPositionParameter.getModulatedValue());

    //calculate sample(s)
    for (auto i = outputBuffer.getNumChannels() - 1; i >= 0; --i)
    {
        //auto currentSample = (osc->fileBuffer.getSample(i % osc->fileBuffer.getNumChannels(), (int)currentBufferPos)) * gainAdjustment;
        auto currentSample = (osc->fileBuffer.getSample(i % osc->fileBuffer.getNumChannels(), static_cast<int>(effectiveBufferPos))) * gainAdjustment;
        currentSample = filter.processSample(currentSample);
        outputBuffer.addSample(i, sampleIndex, (float)currentSample);
    }

    //currentBufferPos += bufferPosDelta;
    //if (currentBufferPos >= osc->fileBuffer.getNumSamples())
    //{
    //    currentBufferPos -= osc->fileBuffer.getNumSamples();
    //}

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
            currentBufferPos = 0.0; //reset when stopping
            nonInstantStateFound = true;
            DBG("Voice stopped and without LFO");
            break;

        case VoiceStateIndex::stopped_Lfo:
            currentBufferPos = 0.0; //reset when stopping
            //associatedLfo->resetSamplesUntilUpdate(); //necessary so that the LFO line on the region doesn't go out of sync
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

bool Voice::isPlaying()
{
    return currentStateIndex == VoiceStateIndex::playable_Lfo || currentStateIndex == VoiceStateIndex::playable_noLfo;
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

ModulatableAdditiveParameter<double>* Voice::getPlaybackPositionStartParameter()
{
    return &playbackPositionStartParameter;
}
void Voice::setBasePlaybackPositionStart(double newPlaybackPositionStart)
{
    playbackPositionStartParameter.setBaseValue(newPlaybackPositionStart);
}
double Voice::getBasePlaybackPositionStart()
{
    return playbackPositionStartParameter.getBaseValue();
}

ModulatableMultiplicativeParameterLowerCap<double>* Voice::getPlaybackPositionIntervalParameter()
{
    return &playbackPositionIntervalParameter;
}
void Voice::setBasePlaybackPositionInterval(double newPlaybackPosition)
{
    playbackPositionIntervalParameter.setBaseValue(newPlaybackPosition);
}
double Voice::getBasePlaybackPositionInterval()
{
    return playbackPositionIntervalParameter.getBaseValue();
}

ModulatableAdditiveParameter<double>* Voice::getPlaybackPositionCurrentParameter()
{
    return &playbackPositionCurrentParameter;
}
void Voice::setBasePlaybackPositionCurrent(double newPlaybackPositionCurrent)
{
    playbackPositionCurrentParameter.setBaseValue(newPlaybackPositionCurrent);
}
double Voice::getBasePlaybackPositionCurrent()
{
    return playbackPositionCurrentParameter.getBaseValue();
}

ModulatableMultiplicativeParameterLowerCap<double>* Voice::getFilterPositionParameter()
{
    return &filterPositionParameter;
}
void Voice::setBaseFilterPosition(double newBaseFilterPosition)
{
    filterPositionParameter.setBaseValue(newBaseFilterPosition);
}
double Voice::getBaseFilterPosition()
{
    return filterPositionParameter.getBaseValue();
}

void Voice::setFilterType(juce::dsp::StateVariableFilter::StateVariableFilterType newFilterType)
{
    filter.parameters->type = newFilterType;
}
juce::dsp::StateVariableFilter::StateVariableFilterType Voice::getFilterType()
{
    return filter.parameters->type;
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

    double modulationSemis = (*this.*pitchQuantisationFuncPt)(); //= pitch shift with all modulations + pitch quantisation

    //for positive numbers: doubled per octave; for negative numbers: halved per octave
    //bufferPosDelta *= //std::pow(2.0, modulationSemis / 12.0); //exact calculation -> very slow!
    bufferPosDelta *= playbackMultApprox(modulationSemis); //approximation -> faster (one power function per sample per voice would be madness efficiency-wise)
    //currentState->playableChanged(true);

    //DBG("bufferPosDelta: " + juce::String(bufferPosDelta));
}

void Voice::evaluateBufferPosModulation()
{
    double modulatedBufferPos = currentBufferPos;
    if (playbackPositionCurrentParameter.modulateValueIfUpdated(&modulatedBufferPos)) //true if it updated the value
    {
        currentBufferPos = static_cast<float>(std::fmod(modulatedBufferPos, 1.0)) * static_cast<float>(osc->fileBuffer.getNumSamples() - 1); //subtracting -1 should *theoretically* not be necessary here bc it will be multiplied with a value within [0,1), *but* due to rounding, it would be possible that it takes on an out-of-range value! it shouldn't make a noticable difference sound-wise.
        //^- see evaluateTablePosModulation method in RegionLfo for further notes

        //don't advance; stick to the target phase!

        envelope.noteOn(false, true); //restart envelope (from attack, not from delay) for a cleaner sound. ignored during release (otherwise, voices with long envelopes and relatively short update intervals that modulate their own playback position would play indefinitely!)
    }
    else
    {
        //no need to adjust currentTablePos (it didn't change).

        //advance
        currentBufferPos += bufferPosDelta;
        double latestPlaybackPosInterval = playbackPositionIntervalParameter.getModulatedValue();
        if (currentBufferPos >= latestPlaybackPosInterval * static_cast<float>(osc->fileBuffer.getNumSamples() - 1)) //-1 because the last sample is equal to the first; multiplied with the phase interval because otherwise, there will be doubling!
        {
            currentBufferPos -= latestPlaybackPosInterval * static_cast<float>(osc->fileBuffer.getNumSamples() - 1);
        }
        jassert(currentBufferPos >= 0 && currentBufferPos < osc->fileBuffer.getNumSamples());
    }
}

void Voice::setPitchQuantisationMethod(PitchQuantisationMethod newPitchQuantisationMethod)
{
    switch (newPitchQuantisationMethod)
    {
    case PitchQuantisationMethod::continuous:
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_continuous;
        break;

    case PitchQuantisationMethod::semitones:
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_semitones;
        break;

    case PitchQuantisationMethod::scale_major:
        setPitchQuantisationScale_major();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minor:
        setPitchQuantisationScale_minor();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_semitonesReversed:
        setPitchQuantisationScale_semitonesReversed();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_majorReversed:
        setPitchQuantisationScale_majorReversed();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minorReversed:
        setPitchQuantisationScale_minorReversed();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_hijazKar:
        setPitchQuantisationScale_hijazKar();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_hungarianMinor:
        setPitchQuantisationScale_hungarianMinor();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_persian:
        setPitchQuantisationScale_persian();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_bluesHexa:
        setPitchQuantisationScale_bluesHexa();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_bluesHepta:
        setPitchQuantisationScale_bluesHepta();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_bluesNona:
        setPitchQuantisationScale_bluesNona();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_ritsu:
        setPitchQuantisationScale_ritsu();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_ryo:
        setPitchQuantisationScale_ryo();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minyo:
        setPitchQuantisationScale_minyo();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_miyakoBushi:
        setPitchQuantisationScale_miyakoBushi();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_wholeTone:
        setPitchQuantisationScale_wholeTone();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_pentatonicMajor:
        setPitchQuantisationScale_pentatonicMajor();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_pentatonicMinor:
        setPitchQuantisationScale_pentatonicMinor();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minorSecondUp:
        setPitchQuantisationScale_minorSecondUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minorSecondDown:
        setPitchQuantisationScale_minorSecondDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_majorSecondUp:
        setPitchQuantisationScale_majorSecondUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_majorSecondDown:
        setPitchQuantisationScale_majorSecondDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minorThirdUp:
        setPitchQuantisationScale_minorThirdUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minorThirdDown:
        setPitchQuantisationScale_minorThirdDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_majorThirdUp:
        setPitchQuantisationScale_majorThirdUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_majorThirdDown:
        setPitchQuantisationScale_majorThirdDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_perfectFourthUp:
        setPitchQuantisationScale_perfectFourthUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_perfectFourthDown:
        setPitchQuantisationScale_perfectFourthDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_diminishedFifthUp:
        setPitchQuantisationScale_diminishedFifthUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_diminishedFifthDown:
        setPitchQuantisationScale_diminishedFifthDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_perfectFifthUp:
        setPitchQuantisationScale_perfectFifthUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_perfectFifthDown:
        setPitchQuantisationScale_perfectFifthDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minorSixthUp:
        setPitchQuantisationScale_minorSixthUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minorSixthDown:
        setPitchQuantisationScale_minorSixthDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_majorSixthUp:
        setPitchQuantisationScale_majorSixthUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_majorSixthDown:
        setPitchQuantisationScale_majorSixthDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minorSeventhUp:
        setPitchQuantisationScale_minorSeventhUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_minorSeventhDown:
        setPitchQuantisationScale_minorSeventhDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_majorSeventhUp:
        setPitchQuantisationScale_majorSeventhUp();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_majorSeventhDown:
        setPitchQuantisationScale_majorSeventhDown();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    case PitchQuantisationMethod::scale_perfectOctave:
        setPitchQuantisationScale_perfectOctave();
        pitchQuantisationFuncPt = &Voice::getQuantisedPitch_scale;
        break;

    default:
        throw std::exception("Unknown or unhandled value of PitchQuantisationMethod.");
    }

    currentPitchQuantisationMethod = newPitchQuantisationMethod;
}
PitchQuantisationMethod Voice::getPitchQuantisationMethod()
{
    return currentPitchQuantisationMethod;
}

double Voice::getQuantisedPitch_continuous()
{
    return pitchShiftParameter.getModulatedValue(); //no special processing needed
}
double Voice::getQuantisedPitch_semitones()
{
    double base = pitchShiftParameter.getBaseValue();
    double modulation = pitchShiftParameter.getModulatedValue() - base;
    int modulation_semi = static_cast<int>(modulation); //acts as a floor function

    return base + static_cast<double>(modulation_semi);
}
double Voice::getQuantisedPitch_scale()
{
    double base = pitchShiftParameter.getBaseValue();
    double modulation = pitchShiftParameter.getModulatedValue() - base;
    int modulation_semi = static_cast<int>(modulation); //acts as a floor function

    int octave = modulation_semi / 12; //can be negative
    int noteIndex = modulation_semi % 12; //value within {0,...,11} -> index of the note regardless of its octave -> this is what will be quantised to the scale

    if (noteIndex < 0) //the % operator yields the remainder, not true modulo
    {
        //mirror
        noteIndex += 12;
        octave -= 1;
    }

    return base + static_cast<double>(pitchQuantisationScale[noteIndex] + 12 * octave);
}

void Voice::setPitchQuantisationScale_major()
{
    pitchQuantisationScale[0] = 0; //c -> c
    pitchQuantisationScale[1] = 0; //c# -> c
    pitchQuantisationScale[2] = 2; //d -> d
    pitchQuantisationScale[3] = 2; //d# -> d
    pitchQuantisationScale[4] = 4; //e -> e
    pitchQuantisationScale[5] = 5; //f -> f
    pitchQuantisationScale[6] = 5; //f# -> f
    pitchQuantisationScale[7] = 7; //g -> g
    pitchQuantisationScale[8] = 7; //g# -> g
    pitchQuantisationScale[9] = 9; //a -> a
    pitchQuantisationScale[10] = 9; //a# -> a
    pitchQuantisationScale[11] = 11; //h -> h
}
void Voice::setPitchQuantisationScale_minor()
{
    pitchQuantisationScale[0] = 0; //c -> c
    pitchQuantisationScale[1] = 0; //c# -> c
    pitchQuantisationScale[2] = 2; //d -> d
    pitchQuantisationScale[3] = 3; //d# -> d#
    pitchQuantisationScale[4] = 3; //e -> d#
    pitchQuantisationScale[5] = 5; //f -> f
    pitchQuantisationScale[6] = 5; //f# -> f
    pitchQuantisationScale[7] = 7; //g -> g
    pitchQuantisationScale[8] = 8; //g# -> g#
    pitchQuantisationScale[9] = 8; //a -> g#
    pitchQuantisationScale[10] = 10; //a# -> a#
    pitchQuantisationScale[11] = 10; //h -> a#
}
void Voice::setPitchQuantisationScale_semitonesReversed()
{
    pitchQuantisationScale[0] = 11; //c
    pitchQuantisationScale[1] = 10; //c#
    pitchQuantisationScale[2] = 9; //d
    pitchQuantisationScale[3] = 8; //d#
    pitchQuantisationScale[4] = 7; //e
    pitchQuantisationScale[5] = 6; //f
    pitchQuantisationScale[6] = 5; //f#
    pitchQuantisationScale[7] = 4; //g
    pitchQuantisationScale[8] = 3; //g#
    pitchQuantisationScale[9] = 2; //a
    pitchQuantisationScale[10] = 1; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_majorReversed()
{
    pitchQuantisationScale[0] = 11; //c
    pitchQuantisationScale[1] = 9; //c#
    pitchQuantisationScale[2] = 9; //d
    pitchQuantisationScale[3] = 7; //d#
    pitchQuantisationScale[4] = 7; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 5; //f#
    pitchQuantisationScale[7] = 4; //g
    pitchQuantisationScale[8] = 2; //g#
    pitchQuantisationScale[9] = 2; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_minorReversed()
{
    pitchQuantisationScale[0] = 10; //c
    pitchQuantisationScale[1] = 10; //c#
    pitchQuantisationScale[2] = 8; //d
    pitchQuantisationScale[3] = 8; //d#
    pitchQuantisationScale[4] = 7; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 5; //f#
    pitchQuantisationScale[7] = 3; //g
    pitchQuantisationScale[8] = 3; //g#
    pitchQuantisationScale[9] = 2; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_hijazKar()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 1; //c#
    pitchQuantisationScale[2] = 1; //d
    pitchQuantisationScale[3] = 4; //d#
    pitchQuantisationScale[4] = 4; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 5; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 8; //g#
    pitchQuantisationScale[9] = 8; //a
    pitchQuantisationScale[10] = 11; //a#
    pitchQuantisationScale[11] = 11; //h
}
void Voice::setPitchQuantisationScale_hungarianMinor()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 2; //d
    pitchQuantisationScale[3] = 3; //d#
    pitchQuantisationScale[4] = 3; //e
    pitchQuantisationScale[5] = 6; //f
    pitchQuantisationScale[6] = 6; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 8; //g#
    pitchQuantisationScale[9] = 8; //a
    pitchQuantisationScale[10] = 11; //a#
    pitchQuantisationScale[11] = 11; //h
}
void Voice::setPitchQuantisationScale_persian()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 1; //c#
    pitchQuantisationScale[2] = 1; //d
    pitchQuantisationScale[3] = 4; //d#
    pitchQuantisationScale[4] = 4; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 6; //f#
    pitchQuantisationScale[7] = 6; //g
    pitchQuantisationScale[8] = 8; //g#
    pitchQuantisationScale[9] = 8; //a
    pitchQuantisationScale[10] = 11; //a#
    pitchQuantisationScale[11] = 11; //h
}
void Voice::setPitchQuantisationScale_bluesHexa()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 3; //d
    pitchQuantisationScale[3] = 3; //d#
    pitchQuantisationScale[4] = 5; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 6; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 7; //g#
    pitchQuantisationScale[9] = 10; //a
    pitchQuantisationScale[10] = 10; //a#
    pitchQuantisationScale[11] = 10; //h
}
void Voice::setPitchQuantisationScale_bluesHepta()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 2; //d
    pitchQuantisationScale[3] = 3; //d#
    pitchQuantisationScale[4] = 3; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 6; //f#
    pitchQuantisationScale[7] = 6; //g
    pitchQuantisationScale[8] = 9; //g#
    pitchQuantisationScale[9] = 9; //a
    pitchQuantisationScale[10] = 10; //a#
    pitchQuantisationScale[11] = 10; //h
}
void Voice::setPitchQuantisationScale_bluesNona()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 2; //d
    pitchQuantisationScale[3] = 3; //d#
    pitchQuantisationScale[4] = 4; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 5; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 7; //g#
    pitchQuantisationScale[9] = 9; //a
    pitchQuantisationScale[10] = 10; //a#
    pitchQuantisationScale[11] = 11; //h
}
void Voice::setPitchQuantisationScale_ritsu()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 2; //d
    pitchQuantisationScale[3] = 2; //d#
    pitchQuantisationScale[4] = 5; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 5; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 7; //g#
    pitchQuantisationScale[9] = 9; //a
    pitchQuantisationScale[10] = 9; //a#
    pitchQuantisationScale[11] = 9; //h
}
void Voice::setPitchQuantisationScale_ryo()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 2; //d
    pitchQuantisationScale[3] = 2; //d#
    pitchQuantisationScale[4] = 4; //e
    pitchQuantisationScale[5] = 4; //f
    pitchQuantisationScale[6] = 4; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 7; //g#
    pitchQuantisationScale[9] = 9; //a
    pitchQuantisationScale[10] = 9; //a#
    pitchQuantisationScale[11] = 9; //h
}
void Voice::setPitchQuantisationScale_minyo()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 3; //d
    pitchQuantisationScale[3] = 3; //d#
    pitchQuantisationScale[4] = 3; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 5; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 7; //g#
    pitchQuantisationScale[9] = 10; //a
    pitchQuantisationScale[10] = 10; //a#
    pitchQuantisationScale[11] = 10; //h
}
void Voice::setPitchQuantisationScale_miyakoBushi()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 1; //c#
    pitchQuantisationScale[2] = 1; //d
    pitchQuantisationScale[3] = 1; //d#
    pitchQuantisationScale[4] = 5; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 7; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 8; //g#
    pitchQuantisationScale[9] = 8; //a
    pitchQuantisationScale[10] = 8; //a#
    pitchQuantisationScale[11] = 8; //h
}
void Voice::setPitchQuantisationScale_wholeTone()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 2; //d
    pitchQuantisationScale[3] = 2; //d#
    pitchQuantisationScale[4] = 4; //e
    pitchQuantisationScale[5] = 4; //f
    pitchQuantisationScale[6] = 6; //f#
    pitchQuantisationScale[7] = 6; //g
    pitchQuantisationScale[8] = 8; //g#
    pitchQuantisationScale[9] = 8; //a
    pitchQuantisationScale[10] = 10; //a#
    pitchQuantisationScale[11] = 10; //h
}
void Voice::setPitchQuantisationScale_pentatonicMajor()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 2; //d
    pitchQuantisationScale[3] = 2; //d#
    pitchQuantisationScale[4] = 4; //e
    pitchQuantisationScale[5] = 4; //f
    pitchQuantisationScale[6] = 4; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 7; //g#
    pitchQuantisationScale[9] = 9; //a
    pitchQuantisationScale[10] = 9; //a#
    pitchQuantisationScale[11] = 9; //h
}
void Voice::setPitchQuantisationScale_pentatonicMinor()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 3; //d
    pitchQuantisationScale[3] = 3; //d#
    pitchQuantisationScale[4] = 3; //e
    pitchQuantisationScale[5] = 5; //f
    pitchQuantisationScale[6] = 5; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 7; //g#
    pitchQuantisationScale[9] = 10; //a
    pitchQuantisationScale[10] = 10; //a#
    pitchQuantisationScale[11] = 10; //h
}
void Voice::setPitchQuantisationScale_minorSecondDown()
{
    pitchQuantisationScale[0] = -1; //c
    pitchQuantisationScale[1] = -1; //c#
    pitchQuantisationScale[2] = -1; //d
    pitchQuantisationScale[3] = -1; //d#
    pitchQuantisationScale[4] = -1; //e
    pitchQuantisationScale[5] = -1; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_minorSecondUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 1; //f#
    pitchQuantisationScale[7] = 1; //g
    pitchQuantisationScale[8] = 1; //g#
    pitchQuantisationScale[9] = 1; //a
    pitchQuantisationScale[10] = 1; //a#
    pitchQuantisationScale[11] = 1; //h
}
void Voice::setPitchQuantisationScale_majorSecondDown()
{
    pitchQuantisationScale[0] = -2; //c
    pitchQuantisationScale[1] = -2; //c#
    pitchQuantisationScale[2] = -2; //d
    pitchQuantisationScale[3] = -2; //d#
    pitchQuantisationScale[4] = -2; //e
    pitchQuantisationScale[5] = -2; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_majorSecondUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 2; //f#
    pitchQuantisationScale[7] = 2; //g
    pitchQuantisationScale[8] = 2; //g#
    pitchQuantisationScale[9] = 2; //a
    pitchQuantisationScale[10] = 2; //a#
    pitchQuantisationScale[11] = 2; //h
}
void Voice::setPitchQuantisationScale_minorThirdDown()
{
    pitchQuantisationScale[0] = -3; //c
    pitchQuantisationScale[1] = -3; //c#
    pitchQuantisationScale[2] = -3; //d
    pitchQuantisationScale[3] = -3; //d#
    pitchQuantisationScale[4] = -3; //e
    pitchQuantisationScale[5] = -3; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_minorThirdUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 3; //f#
    pitchQuantisationScale[7] = 3; //g
    pitchQuantisationScale[8] = 3; //g#
    pitchQuantisationScale[9] = 3; //a
    pitchQuantisationScale[10] = 3; //a#
    pitchQuantisationScale[11] = 3; //h
}
void Voice::setPitchQuantisationScale_majorThirdDown()
{
    pitchQuantisationScale[0] = -4; //c
    pitchQuantisationScale[1] = -4; //c#
    pitchQuantisationScale[2] = -4; //d
    pitchQuantisationScale[3] = -4; //d#
    pitchQuantisationScale[4] = -4; //e
    pitchQuantisationScale[5] = -4; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_majorThirdUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 4; //f#
    pitchQuantisationScale[7] = 4; //g
    pitchQuantisationScale[8] = 4; //g#
    pitchQuantisationScale[9] = 4; //a
    pitchQuantisationScale[10] = 4; //a#
    pitchQuantisationScale[11] = 4; //h
}
void Voice::setPitchQuantisationScale_perfectFourthDown()
{
    pitchQuantisationScale[0] = -5; //c
    pitchQuantisationScale[1] = -5; //c#
    pitchQuantisationScale[2] = -5; //d
    pitchQuantisationScale[3] = -5; //d#
    pitchQuantisationScale[4] = -5; //e
    pitchQuantisationScale[5] = -5; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_perfectFourthUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 5; //f#
    pitchQuantisationScale[7] = 5; //g
    pitchQuantisationScale[8] = 5; //g#
    pitchQuantisationScale[9] = 5; //a
    pitchQuantisationScale[10] = 5; //a#
    pitchQuantisationScale[11] = 5; //h
}
void Voice::setPitchQuantisationScale_diminishedFifthDown()
{
    pitchQuantisationScale[0] = -6; //c
    pitchQuantisationScale[1] = -6; //c#
    pitchQuantisationScale[2] = -6; //d
    pitchQuantisationScale[3] = -6; //d#
    pitchQuantisationScale[4] = -6; //e
    pitchQuantisationScale[5] = -6; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_diminishedFifthUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 6; //f#
    pitchQuantisationScale[7] = 6; //g
    pitchQuantisationScale[8] = 6; //g#
    pitchQuantisationScale[9] = 6; //a
    pitchQuantisationScale[10] = 6; //a#
    pitchQuantisationScale[11] = 6; //h
}
void Voice::setPitchQuantisationScale_perfectFifthDown()
{
    pitchQuantisationScale[0] = -7; //c
    pitchQuantisationScale[1] = -7; //c#
    pitchQuantisationScale[2] = -7; //d
    pitchQuantisationScale[3] = -7; //d#
    pitchQuantisationScale[4] = -7; //e
    pitchQuantisationScale[5] = -7; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_perfectFifthUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 7; //f#
    pitchQuantisationScale[7] = 7; //g
    pitchQuantisationScale[8] = 7; //g#
    pitchQuantisationScale[9] = 7; //a
    pitchQuantisationScale[10] = 7; //a#
    pitchQuantisationScale[11] = 7; //h
}
void Voice::setPitchQuantisationScale_minorSixthDown()
{
    pitchQuantisationScale[0] = -8; //c
    pitchQuantisationScale[1] = -8; //c#
    pitchQuantisationScale[2] = -8; //d
    pitchQuantisationScale[3] = -8; //d#
    pitchQuantisationScale[4] = -8; //e
    pitchQuantisationScale[5] = -8; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_minorSixthUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 8; //f#
    pitchQuantisationScale[7] = 8; //g
    pitchQuantisationScale[8] = 8; //g#
    pitchQuantisationScale[9] = 8; //a
    pitchQuantisationScale[10] = 8; //a#
    pitchQuantisationScale[11] = 8; //h
}
void Voice::setPitchQuantisationScale_majorSixthDown()
{
    pitchQuantisationScale[0] = -9; //c
    pitchQuantisationScale[1] = -9; //c#
    pitchQuantisationScale[2] = -9; //d
    pitchQuantisationScale[3] = -9; //d#
    pitchQuantisationScale[4] = -9; //e
    pitchQuantisationScale[5] = -9; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_majorSixthUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 9; //f#
    pitchQuantisationScale[7] = 9; //g
    pitchQuantisationScale[8] = 9; //g#
    pitchQuantisationScale[9] = 9; //a
    pitchQuantisationScale[10] = 9; //a#
    pitchQuantisationScale[11] = 9; //h
}
void Voice::setPitchQuantisationScale_minorSeventhDown()
{
    pitchQuantisationScale[0] = -10; //c
    pitchQuantisationScale[1] = -10; //c#
    pitchQuantisationScale[2] = -10; //d
    pitchQuantisationScale[3] = -10; //d#
    pitchQuantisationScale[4] = -10; //e
    pitchQuantisationScale[5] = -10; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_minorSeventhUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 10; //f#
    pitchQuantisationScale[7] = 10; //g
    pitchQuantisationScale[8] = 10; //g#
    pitchQuantisationScale[9] = 10; //a
    pitchQuantisationScale[10] = 10; //a#
    pitchQuantisationScale[11] = 10; //h
}
void Voice::setPitchQuantisationScale_majorSeventhDown()
{
    pitchQuantisationScale[0] = -11; //c
    pitchQuantisationScale[1] = -11; //c#
    pitchQuantisationScale[2] = -11; //d
    pitchQuantisationScale[3] = -11; //d#
    pitchQuantisationScale[4] = -11; //e
    pitchQuantisationScale[5] = -11; //f
    pitchQuantisationScale[6] = 0; //f#
    pitchQuantisationScale[7] = 0; //g
    pitchQuantisationScale[8] = 0; //g#
    pitchQuantisationScale[9] = 0; //a
    pitchQuantisationScale[10] = 0; //a#
    pitchQuantisationScale[11] = 0; //h
}
void Voice::setPitchQuantisationScale_majorSeventhUp()
{
    pitchQuantisationScale[0] = 0; //c
    pitchQuantisationScale[1] = 0; //c#
    pitchQuantisationScale[2] = 0; //d
    pitchQuantisationScale[3] = 0; //d#
    pitchQuantisationScale[4] = 0; //e
    pitchQuantisationScale[5] = 0; //f
    pitchQuantisationScale[6] = 11; //f#
    pitchQuantisationScale[7] = 11; //g
    pitchQuantisationScale[8] = 11; //g#
    pitchQuantisationScale[9] = 11; //a
    pitchQuantisationScale[10] = 11; //a#
    pitchQuantisationScale[11] = 11; //h
}
void Voice::setPitchQuantisationScale_perfectOctave()
{
    pitchQuantisationScale[0] = 0; //c -> c
    pitchQuantisationScale[1] = 0; //c# -> c
    pitchQuantisationScale[2] = 0; //d -> c
    pitchQuantisationScale[3] = 0; //d# -> c
    pitchQuantisationScale[4] = 0; //e -> c
    pitchQuantisationScale[5] = 0; //f -> c
    pitchQuantisationScale[6] = 0; //f# -> c
    pitchQuantisationScale[7] = 0; //g -> c
    pitchQuantisationScale[8] = 0; //g# -> c
    pitchQuantisationScale[9] = 0; //a -> c
    pitchQuantisationScale[10] = 0; //a# -> c
    pitchQuantisationScale[11] = 0; //h -> c
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
void Voice::setID(int newID)
{
    ID = newID;
}

DahdsrEnvelope* Voice::getEnvelope()
{
    return &envelope;
}

bool Voice::serialise(juce::XmlElement* xmlVoice)
{
    DBG("serialising Voice...");
    bool serialisationSuccessful = true;

    //basic members
    xmlVoice->setAttribute("regionID", ID);
    //bufferPos, bufferPosDelta: not needed
    xmlVoice->setAttribute("currentPitchQuantisationMethod", static_cast<int>(currentPitchQuantisationMethod));

    //parameters
    xmlVoice->setAttribute("levelParameter_base", levelParameter.getBaseValue());
    xmlVoice->setAttribute("pitchShiftParameter_base", pitchShiftParameter.getBaseValue());
    xmlVoice->setAttribute("playbackPositionStartParameter_base", playbackPositionStartParameter.getBaseValue());
    xmlVoice->setAttribute("playbackPositionIntervalParameter_base", playbackPositionIntervalParameter.getBaseValue());
    xmlVoice->setAttribute("filterPositionParameter_base", filterPositionParameter.getBaseValue());
    xmlVoice->setAttribute("filterType", static_cast<int>(filter.parameters->type));

    //envelope
    serialisationSuccessful = envelope.serialise(xmlVoice);

    //osc: needn't be serialised, because when the SegmentedRegion associated with this voice is deserialised, it will already initialise osc

    DBG(juce::String(serialisationSuccessful ? "Voice has been serialised." : "Voice could not be serialised."));
    return serialisationSuccessful;
}
bool Voice::deserialise(juce::XmlElement* xmlVoice)
{
    DBG("deserialising Voice...");
    bool deserialisationSuccessful = true;

    //basic members
    ID = xmlVoice->getIntAttribute("regionID", -1);
    //bufferPos, bufferPosDelta: not needed
    setPitchQuantisationMethod(static_cast<PitchQuantisationMethod>(xmlVoice->getIntAttribute("currentPitchQuantisationMethod", static_cast<int>(PitchQuantisationMethod::continuous))));

    //parameters
    levelParameter.setBaseValue(xmlVoice->getDoubleAttribute("levelParameter_base", 0.25));
    pitchShiftParameter.setBaseValue(xmlVoice->getDoubleAttribute("pitchShiftParameter_base", 0.0));
    playbackPositionIntervalParameter.setBaseValue(xmlVoice->getDoubleAttribute("playbackPositionIntervalParameter_base", 1.0));
    playbackPositionStartParameter.setBaseValue(xmlVoice->getDoubleAttribute("playbackPositionStartParameter_base", 0.0));
    filterPositionParameter.setBaseValue(xmlVoice->getDoubleAttribute("filterPositionParameter_base", 22050.0));
    filter.parameters->type = static_cast<juce::dsp::StateVariableFilter::StateVariableFilterType>(xmlVoice->getIntAttribute("filterType", 0));

    //envelope
    deserialisationSuccessful = envelope.deserialise(xmlVoice);

    //osc: needn't be deserialised, because when the SegmentedRegion associated with this voice is deserialised, it will already initialise osc

    DBG(juce::String(deserialisationSuccessful ? "Voice has been deserialised." : "Voice could not be deserialised."));
    return deserialisationSuccessful;
}
