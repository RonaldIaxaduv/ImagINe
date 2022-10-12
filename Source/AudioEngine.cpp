/*
  ==============================================================================

    AudioEngine.cpp
    Created: 27 Sep 2022 10:17:36am
    Author:  Aaron

  ==============================================================================
*/

#include "AudioEngine.h"

AudioEngine::AudioEngine(juce::MidiKeyboardState& keyState) :
    keyboardState(keyState),
    specs()
{
    //for (auto i = 0; i < 4; ++i)                // [1]
    //    synth.addVoice(new Voice());

    //synth.addSound(new SamplerOscillator());       // [2]

    synth.addSound(new TempSound());
}

int AudioEngine::getNextRegionID()
{
    return ++regionIdCounter;
}
int AudioEngine::getLastRegionID()
{
    return regionIdCounter;
}
int AudioEngine::addNewRegion(const juce::Colour& regionColour)
{
    regionColours.add(regionColour);
    int newRegionID = getNextRegionID();

    //create LFO
    addLfo(new RegionLfo(newRegionID));

    //create voices
    initialiseVoicesForRegion(newRegionID);

    return newRegionID;
}

juce::Colour AudioEngine::getRegionColour(int regionID)
{
    if (regionID >= 0 && regionID <= getLastRegionID())
        return regionColours[regionID];
    else
        return juce::Colours::transparentBlack;
}
void AudioEngine::changeRegionColour(int regionID, juce::Colour newColour)
{
    if (regionID >= 0 && regionID <= getLastRegionID())
    {
        regionColours.set(regionID, newColour);
    }
}

void AudioEngine::initialiseVoicesForRegion(int regionID)
{
    int voiceCount = 1; //WIP: change this later to allow for polyphony if desired. for now, regions are monophone
    for (int i = 0; i < voiceCount; ++i)
    {
        addVoice(new Voice(regionID));
    }
}
int AudioEngine::addVoice(Voice* newVoice)
{
    auto* associatedLfo = getLfo(newVoice->getID());
    if (associatedLfo != nullptr)
    {
        //newVoice->setLfoAdvancer(associatedLfo->getAdvancerFunction()); //make the new voice advance the LFO associated with the same region
        newVoice->setLfo(associatedLfo); //make the new voice advance the LFO associated with the same region
    }
    newVoice->prepare(specs);

    synth.addVoice(newVoice);

    DBG("successfully added voice #" + juce::String(synth.getNumVoices() - 1) + ". associated region: " + juce::String(newVoice->getID()));

    return synth.getNumVoices() - 1;
}
juce::Array<Voice*> AudioEngine::getVoicesWithID(int regionID)
{
    juce::Array<Voice*> voiceList;

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        auto curVoice = static_cast<Voice*>(synth.getVoice(i));

        if (curVoice->getID() == regionID)
        {
            voiceList.add(curVoice);
        }
    }

    return voiceList;
}
bool AudioEngine::checkRegionHasVoice(int regionID)
{
    if (getVoicesWithID(regionID).size() > 0)
        return true;
    else
        return false;
}
void AudioEngine::removeVoicesWithID(int regionID)
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        auto* curVoice = static_cast<Voice*>(synth.getVoice(i));
        if (curVoice->getID() == regionID)
        {
            synth.removeVoice(i);
            --i;
        }
    }
}

juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_Volume(int regionID)
{
    juce::Array<Voice*> voices = getVoicesWithID(regionID);
    juce::Array<ModulatableParameter<double>*> parameters;

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        parameters.add((*it)->getLevelParameter());
    }

    return parameters;
}
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_Pitch(int regionID)
{
    juce::Array<Voice*> voices = getVoicesWithID(regionID);
    juce::Array<ModulatableParameter<double>*> parameters;

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        parameters.add((*it)->getPitchShiftParameter());
    }

    return parameters;
}
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_PlaybackPosition(int regionID)
{
    juce::Array<Voice*> voices = getVoicesWithID(regionID);
    juce::Array<ModulatableParameter<double>*> parameters;

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        parameters.add((*it)->getPlaybackPositionParameter());
    }

    return parameters;
}
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_LfoRate(int regionID)
{
    juce::Array<ModulatableParameter<double>*> parameters;

    RegionLfo* lfo = getLfo(regionID);
    if (lfo != nullptr)
    {
        parameters.add(lfo->getFrequencyModParameter()); //only one LFO per region, so only one entry in parameters necessary
    }

    return parameters;
}
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_LfoPhase(int regionID)
{
    juce::Array<ModulatableParameter<double>*> parameters;

    RegionLfo* lfo = getLfo(regionID);
    if (lfo != nullptr)
    {
        parameters.add(lfo->getPhaseModParameter()); //only one LFO per region, so only one entry in parameters necessary
    }

    return parameters;
}
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_LfoUpdateInterval(int regionID)
{
    juce::Array<ModulatableParameter<double>*> parameters;

    RegionLfo* lfo = getLfo(regionID);
    if (lfo != nullptr)
    {
        parameters.add(lfo->getUpdateIntervalParameter()); //only one LFO per region, so only one entry in parameters necessary
    }

    return parameters;
}


void AudioEngine::prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate)
{
    synth.setCurrentPlaybackSampleRate(sampleRate); // [3]

    specs.sampleRate = sampleRate;

    for (auto it = lfos.begin(); it != lfos.end(); ++it)
    {
        (*it)->prepare(specs);
    }
}

void AudioEngine::releaseResources()
{
    lfos.clear();
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    juce::MidiBuffer incomingMidi;
    keyboardState.processNextMidiBuffer(incomingMidi, bufferToFill.startSample,
        bufferToFill.numSamples, true);       // [4]

    //synth.renderNextBlock(*bufferToFill.buffer, incomingMidi,
    //    bufferToFill.startSample, bufferToFill.numSamples); // [5]

    //evaluate all voices sample by sample! this is crucial since every voice *must* be rendered in sync with its LFO.
    //but since different voices' LFOs can influence one another, all voices must be rendered perfectly in sync as well!
    //the main disadvantage of this is that some processing (e.g. many types of filters, EQs or convolution-based effects)
    //might not be possible anymore like this since they need to be rendered in block (since they require an FFT).
    //there may be workarounds for this, though.
    for (int i = bufferToFill.startSample; i < bufferToFill.startSample + bufferToFill.numSamples; ++i)
    {
        synth.renderNextBlock(*bufferToFill.buffer, incomingMidi, i, 1); //sample by sample
    }
}

juce::Synthesiser* AudioEngine::getSynth()
{
    return &synth;
}

void AudioEngine::addLfo(RegionLfo* newLfo)
{
    int regionID = newLfo->getRegionID(); //the LFO belongs to this region
    int newLfoIndex = lfos.size();
    newLfo->prepare(specs);
    newLfo->setBaseFrequency(0.2f);
    lfos.add(newLfo);

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        auto* curVoice = static_cast<Voice*>(synth.getVoice(i));
        if (curVoice->getID() == regionID)
        {
            //all voices belonging to the same region as the LFO will now advance this LFO
            //(as long as they aren't tailing off, i.e. it's only 1 voice advancing it, but it could be any of these voices)
            //curVoice->setLfoIndex(newLfoIndex);
            //curVoice->setLfoAdvancer(newLfo->getAdvancerFunction());
            curVoice->setLfo(newLfo);
        }
    }

    DBG("successfully added LFO to region " + juce::String(newLfo->getRegionID()));
}
RegionLfo* AudioEngine::getLfo(int regionID)
{
    for (auto it = lfos.begin(); it != lfos.end(); ++it)
    {
        if ((*it)->getRegionID() == regionID)
        {
            return (*it);
        }
    }
    return nullptr; //no LFO associated with this region ID found
}
void AudioEngine::removeLfo(int regionID) //removes the LFO of the region with the given ID
{
    int lfoIndex = -1;

    for (auto it = lfos.begin(); it != lfos.end(); ++it)
    {
        if ((*it)->getRegionID() == regionID)
        {
            //LFO found -> remove
            lfoIndex = lfos.indexOf((*it));
            break;
        }
    }

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        auto* curVoice = static_cast<Voice*>(synth.getVoice(i));
        if (curVoice->getID() == regionID)
        {
            //curVoice->setLfoAdvancer([] {; }); //clear advancer function
            curVoice->setLfo(nullptr); //clear associated LFO
        }
    }

    lfos.remove(lfoIndex, true); //removed and deleted
}
