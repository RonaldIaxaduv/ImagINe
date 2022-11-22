/*
  ==============================================================================

    AudioEngine.cpp
    Created: 27 Sep 2022 10:17:36am
    Author:  Aaron

  ==============================================================================
*/

#include "AudioEngine.h"

#include "SegmentableImage.h"

//constants
const int AudioEngine::defaultPolyphony = 1;




AudioEngine::AudioEngine(juce::MidiKeyboardState& keyState, juce::MidiMessageCollector& midiCollector, juce::AudioProcessor& associatedProcessor) :
    keyboardState(keyState), midiCollector(midiCollector), associatedProcessor(associatedProcessor),
    specs()
{
    //associatedImage = std::make_unique<SegmentableImage>(new SegmentableImage(this));
    associatedImage = new SegmentableImage(this);

    synth.addSound(new TempSound());
}
AudioEngine::~AudioEngine()
{
    DBG("destroying AudioEngine...");

    //associatedImage will be deleted automatically (unique_ptr)
    delete associatedImage;

    lfos.clear(true);

    juce::AudioSource::~AudioSource();

    DBG("AudioEngine destroyed.");
}

bool AudioEngine::serialise(juce::XmlElement* xml, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("serialising AudioEngine...");
    bool serialisationSuccessful = true;
    juce::XmlElement* xmlAudioEngine = xml->createNewChildElement("AudioEngine");

    //serialise image
    serialisationSuccessful = serialiseImage(xmlAudioEngine, attachedData);

    if (serialisationSuccessful)
    {
        //serialise audio engine's region data
        xmlAudioEngine->setAttribute("regionIdCounter", regionIdCounter);
        serialisationSuccessful = serialiseRegionColours(xmlAudioEngine);

        if (serialisationSuccessful)
        {
            //serialise LFO data
            serialisationSuccessful = serialiseLFOs(xmlAudioEngine);

            if (serialisationSuccessful)
            {
                //serialise voice data
                serialisationSuccessful = serialiseVoices(xmlAudioEngine);
            }
        }
    }

    DBG(juce::String(serialisationSuccessful ? "AudioEngine has been serialised." : "AudioEngine could not be serialised."));
    return serialisationSuccessful;
}
bool AudioEngine::serialiseImage(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData)
{
    //return associatedImage.get()->serialise(xmlAudioEngine, attachedData); //stores image and all its regions;
    return associatedImage->serialise(xmlAudioEngine, attachedData); //stores image and all its regions;
}
bool AudioEngine::serialiseRegionColours(juce::XmlElement* xmlAudioEngine)
{
    DBG("serialising regionColours...");
    bool serialisationSuccessful = true;
    juce::XmlElement* xmlRegionColours = xmlAudioEngine->createNewChildElement("regionColours");

    xmlRegionColours->setAttribute("size", regionColours.size());

    int i = 0;
    for (auto itColour = regionColours.begin(); itColour != regionColours.end(); ++itColour, ++i)
    {
        xmlRegionColours->setAttribute("Colour_" + juce::String(i), itColour->toString()); //can be converted back via fromString()
    }

    DBG(juce::String(serialisationSuccessful ? "regionColours has been serialised." : "regionColours could not be serialised."));
    return serialisationSuccessful;
}
bool AudioEngine::serialiseLFOs(juce::XmlElement* xmlAudioEngine)
{
    bool serialisationSuccessful = true;
    xmlAudioEngine->setAttribute("lfos_size", lfos.size());

    for (auto itLfo = lfos.begin(); serialisationSuccessful && itLfo != lfos.end(); ++itLfo)
    {
        juce::XmlElement* xmlLfo = xmlAudioEngine->createNewChildElement("LFO_" + juce::String((*itLfo)->getRegionID()));
        serialisationSuccessful = (*itLfo)->serialise(xmlLfo);
    }

    return serialisationSuccessful;
}
bool AudioEngine::serialiseVoices(juce::XmlElement* xmlAudioEngine)
{
    //note: serialise only ONE voice per region, and the total number of voices for that region
    //-> pass the same XmlElement to all [number of voices] Voice members to initialise them all with the same values

    bool serialisationSuccessful = true;
    xmlAudioEngine->setAttribute("synth_numVoices", synth.getNumVoices());

    for (int id = 0; serialisationSuccessful && id <= regionIdCounter; ++id)
    {
        auto voices = getVoicesWithID(id);
        
        if (voices.size() > 0) //only create voice data for regions that exist
        {
            juce::XmlElement* xmlVoices = xmlAudioEngine->createNewChildElement("Voices_" + juce::String(id));
            serialisationSuccessful = voices[0]->serialise(xmlVoices); //it's enough to serialise one voice per region, because all other voices have exactly the same parameters
        }
    }

    return serialisationSuccessful;
}

bool AudioEngine::deserialise(juce::XmlElement* xml, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("deserialising AudioEngine...");
    bool deserialisationSuccessful = true;

    juce::XmlElement* xmlAudioEngine = xml->getChildByName("AudioEngine");

    if (xmlAudioEngine != nullptr)
    {
        //deserialise image
        deserialisationSuccessful = deserialiseImage(xmlAudioEngine, attachedData);

        if (deserialisationSuccessful)
        {
            //deserialise audio engine's region data
            regionIdCounter = xmlAudioEngine->getIntAttribute("regionIdCounter", 0);
            deserialisationSuccessful = deserialiseRegionColours(xmlAudioEngine);

            if (deserialisationSuccessful)
            {
                //deserialise LFO data (excluding mods)
                deserialisationSuccessful = deserialiseLFOs_main(xmlAudioEngine);

                if (deserialisationSuccessful)
                {
                    //deserialise voice data
                    deserialisationSuccessful = deserialiseVoices(xmlAudioEngine);

                    if (deserialisationSuccessful)
                    {
                        //deserialise LFO mods (now that all regions, LFOs and voices have been deserialised)
                        deserialisationSuccessful = deserialiseLFOs_mods(xmlAudioEngine);
                    }
                }
            }
        }
    }
    else
    {
        DBG("no AudioEngine data found.");
        deserialisationSuccessful = false;
    }

    DBG(juce::String(deserialisationSuccessful ? "AudioEngine has been deserialised." : "AudioEngine could not be deserialised."));
    return deserialisationSuccessful;
}
bool AudioEngine::deserialiseImage(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData)
{
    //return associatedImage.get()->deserialise(xmlAudioEngine, attachedData); //restores image and all its regions;
    return associatedImage->deserialise(xmlAudioEngine, attachedData); //restores image and all its regions;
}
bool AudioEngine::deserialiseRegionColours(juce::XmlElement* xmlAudioEngine)
{
    DBG("deserialising regionColours...");
    bool deserialisationSuccessful = true;

    //clear previous data
    regionColours.clear();
    
    juce::XmlElement* xmlRegionColours = xmlAudioEngine->getChildByName("regionColours");

    if (xmlRegionColours != nullptr)
    {
        int size = xmlRegionColours->getIntAttribute("size");
        for (int i = 0; i < size; ++i)
        {
            regionColours.add(juce::Colour::fromString(xmlRegionColours->getStringAttribute("Colour_" + juce::String(i), juce::Colours::black.toString())));
        }
    }
    else
    {
        DBG("no regionColours data found.");
        deserialisationSuccessful = false;
    }

    DBG(juce::String(deserialisationSuccessful ? "regionColours has been deserialised." : "regionColours could not be deserialised."));
    return deserialisationSuccessful;
}
bool AudioEngine::deserialiseLFOs_main(juce::XmlElement* xmlAudioEngine)
{
    bool deserialisationSuccessful = true;
    int size = xmlAudioEngine->getIntAttribute("lfos_size", 0);

    jassert(size == lfos.size()); //LFOs should already be initialised at this point, because the SegmentedRegion objects are deserialised first

    for (int i = 0; deserialisationSuccessful && i <= regionIdCounter; ++i)
    {
        juce::XmlElement* xmlLfo = xmlAudioEngine->getChildByName("LFO_" + juce::String(lfos[i]->getRegionID()));

        if (xmlLfo != nullptr)
        {
            deserialisationSuccessful = lfos[i]->deserialise_main(xmlLfo);
        }
        else
        {
            DBG("LFO data of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO could not be found.");
            deserialisationSuccessful = false;
        }
    }

    return deserialisationSuccessful;
}
bool AudioEngine::deserialiseLFOs_mods(juce::XmlElement* xmlAudioEngine)
{
    DBG("deserialising LFO mods...");
    bool deserialisationSuccessful = true;

    int lfos_size = xmlAudioEngine->getIntAttribute("lfos_size", 0);

    jassert(lfos_size == lfos.size()); //LFOs should already be initialised at this point, because the SegmentedRegion objects are deserialised first
    deserialisationSuccessful = (lfos_size == lfos.size());

    for (int i = 0; deserialisationSuccessful && i <= regionIdCounter; ++i)
    {
        juce::XmlElement* xmlLfo = xmlAudioEngine->getChildByName("LFO_" + juce::String(lfos[i]->getRegionID()));

        if (xmlLfo != nullptr)
        {
            //it's easier to directly implement this here instead of using a method lfos[i]->deserialise_mods(xmlLfo), because deserialising the mods requires access to the AudioEngine, which the LFOs don't have (and would be messy to implement)
            DBG("deserialising mods of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO...");

            juce::XmlElement* xmlParameterIDs = xmlLfo->getChildByName("modulatedParameterIDs");
            juce::Array<LfoModulatableParameter> pIDs;
            if (xmlParameterIDs != nullptr)
            {
                int size = xmlParameterIDs->getIntAttribute("size");
                for (int j = 0; j < size; ++j)
                {
                    pIDs.add(static_cast<LfoModulatableParameter>(xmlParameterIDs->getIntAttribute("ID_" + juce::String(j))));
                }
            }
            else
            {
                DBG("parameter IDs of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO could not be found.");
                //deserialisationSuccessful = false; //not problematic
                continue;
            }

            juce::XmlElement* xmlRegionIDs = xmlLfo->getChildByName("affectedRegionIDs");
            juce::Array<int> rIDs;
            if (xmlRegionIDs != nullptr)
            {
                int size = xmlRegionIDs->getIntAttribute("size");
                for (int j = 0; j < size; ++j)
                {
                    rIDs.add(xmlRegionIDs->getIntAttribute("ID_" + juce::String(j)));
                }
            }
            else
            {
                DBG("region IDs of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO could not be found.");
                //deserialisationSuccessful = false; //not problematic
                continue;
            }

            if (pIDs.size() != rIDs.size())
            {
                DBG("ID sizes of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO don't match!");
                deserialisationSuccessful = false;
                continue;
            }

            juce::Array<ModulatableParameter<double>*> affectedParams;
            for (int j = 0; j < pIDs.size(); ++j)
            {
                //get affected parameters
                switch (pIDs[j])
                {
                case LfoModulatableParameter::volume:
                case LfoModulatableParameter::volume_inverted:
                    affectedParams = getParameterOfRegion_Volume(rIDs[j]);
                    break;

                case LfoModulatableParameter::pitch:
                case LfoModulatableParameter::pitch_inverted:
                    affectedParams = getParameterOfRegion_Pitch(rIDs[j]);
                    break;

                case LfoModulatableParameter::playbackPositionInterval:
                case LfoModulatableParameter::playbackPositionInterval_inverted:
                    affectedParams = getParameterOfRegion_PlaybackPositionInterval(rIDs[j]);
                    break;

                case LfoModulatableParameter::lfoRate:
                case LfoModulatableParameter::lfoRate_inverted:
                    affectedParams = getParameterOfRegion_LfoRate(rIDs[j]);
                    break;

                case LfoModulatableParameter::lfoPhaseInterval:
                case LfoModulatableParameter::lfoPhaseInterval_inverted:
                    affectedParams = getParameterOfRegion_LfoPhaseInterval(rIDs[j]);
                    break;

                case LfoModulatableParameter::lfoUpdateInterval:
                case LfoModulatableParameter::lfoUpdateInterval_inverted:
                    affectedParams = getParameterOfRegion_LfoUpdateInterval(rIDs[j]);
                    break;

                default:
                    throw std::exception("Unknown or unimplemented region modulation");
                }

                //apply modulation
                lfos[i]->addRegionModulation(pIDs[j], rIDs[j], affectedParams);
            }

            DBG(juce::String(deserialisationSuccessful ? "mods of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO have been deserialised." : "mods of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO could not be deserialised."));
        }
        else
        {
            DBG("LFO data of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO could not be found.");
            deserialisationSuccessful = false;
        }
    }

    DBG(juce::String(deserialisationSuccessful ? "LFO mods have been deserialised." : "LFO mods could not be deserialised."));
    return deserialisationSuccessful;
}
bool AudioEngine::deserialiseVoices(juce::XmlElement* xmlAudioEngine)
{
    //note: serialise only ONE voice per region, and the total number of voices for that region
    //-> pass the same XmlElement to all [number of voices] Voice members to initialise them all with the same values
    bool deserialisationSuccessful = true;

    for (int id = 0; deserialisationSuccessful && id <= regionIdCounter; ++id)
    {
        juce::XmlElement* xmlVoices = xmlAudioEngine->getChildByName("Voices_" + juce::String(id));

        if (xmlVoices != nullptr) //only apply voice data for regions that exist
        {
            //note: the correct polyphony is already ensured within the deserialisation of the associated SegmentedRegion object
            DBG("deserialising voice data of region " + juce::String(id) + "...");

            //load data of all voices (excluding the sound data, which is loaded from their associated SegmentedRegion)
            auto voices = getVoicesWithID(id);
            for (auto itVoice = voices.begin(); itVoice != voices.end(); ++itVoice)
            {
                deserialisationSuccessful = (*itVoice)->deserialise(xmlVoices); //deserialise each voice with exactly the same parameters
            }

            DBG(juce::String(deserialisationSuccessful ? "voice data of region " + juce::String(id) + " has been deserialised." : "voice data of region " + juce::String(id) + " could not be deserialised."));
        }
        else
        {
            DBG("voice data of region " + juce::String(id) + " could not be found. this isn't necessarily a problem because the code also checks for voices of already deleted regions.");
        }
    }

    DBG("final number of voices: " + juce::String(synth.getNumVoices()) + " (target was " + juce::String(xmlAudioEngine->getIntAttribute("synth_numVoices", 0)) + ")");
    jassert(synth.getNumVoices() == xmlAudioEngine->getIntAttribute("synth_numVoices", 0));
    deserialisationSuccessful = synth.getNumVoices() == xmlAudioEngine->getIntAttribute("synth_numVoices", 0);

    return deserialisationSuccessful;
}

SegmentableImage* AudioEngine::getImage()
{
    //return *(associatedImage.get());
    return associatedImage;
}

int AudioEngine::getNextRegionID()
{
    return ++regionIdCounter;
}
int AudioEngine::getLastRegionID()
{
    return regionIdCounter;
}
int AudioEngine::addNewRegion(const juce::Colour& regionColour, juce::MidiKeyboardState::Listener* listenerRegion)
{
    regionColours.add(regionColour);
    int newRegionID = getNextRegionID();

    //create LFO
    addLfo(new RegionLfo(newRegionID));

    //create voices
    initialiseVoicesForRegion(newRegionID);

    //register MIDI listener
    keyboardState.addListener(listenerRegion);

    return newRegionID;
}
void AudioEngine::resetRegionIDs()
{
    regionIdCounter = -1;
    regionColours.clear();
}
void AudioEngine::removeRegion(juce::MidiKeyboardState::Listener* listenerRegion)
{
    keyboardState.removeListener(listenerRegion);
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

void AudioEngine::initialiseVoicesForRegion(int regionID, int voiceCount)
{
    for (int i = 0; i < voiceCount; ++i)
    {
        addVoice(new Voice(regionID));
    }
}
int AudioEngine::addVoice(Voice* newVoice)
{
    newVoice->prepare(specs);

    auto* associatedLfo = getLfo(newVoice->getID());
    if (associatedLfo != nullptr)
    {
        //newVoice->setLfoAdvancer(associatedLfo->getAdvancerFunction()); //make the new voice advance the LFO associated with the same region
        newVoice->setLfo(associatedLfo); //make the new voice advance the LFO associated with the same region
    }

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
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_PlaybackPositionStart(int regionID)
{
    juce::Array<Voice*> voices = getVoicesWithID(regionID);
    juce::Array<ModulatableParameter<double>*> parameters;

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        parameters.add((*it)->getPlaybackPositionStartParameter());
    }

    return parameters;
}
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_PlaybackPositionInterval(int regionID)
{
    juce::Array<Voice*> voices = getVoicesWithID(regionID);
    juce::Array<ModulatableParameter<double>*> parameters;

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        parameters.add((*it)->getPlaybackPositionIntervalParameter());
    }

    return parameters;
}
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_PlaybackPositionCurrent(int regionID)
{
    juce::Array<Voice*> voices = getVoicesWithID(regionID);
    juce::Array<ModulatableParameter<double>*> parameters;

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        parameters.add((*it)->getPlaybackPositionCurrentParameter());
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
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_LfoStartingPhase(int regionID)
{
    juce::Array<ModulatableParameter<double>*> parameters;

    RegionLfo* lfo = getLfo(regionID);
    if (lfo != nullptr)
    {
        parameters.add(lfo->getStartingPhaseModParameter()); //only one LFO per region, so only one entry in parameters necessary
    }

    return parameters;
}
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_LfoPhaseInterval(int regionID)
{
    juce::Array<ModulatableParameter<double>*> parameters;

    RegionLfo* lfo = getLfo(regionID);
    if (lfo != nullptr)
    {
        parameters.add(lfo->getPhaseIntervalModParameter()); //only one LFO per region, so only one entry in parameters necessary
    }

    return parameters;
}
juce::Array<ModulatableParameter<double>*> AudioEngine::getParameterOfRegion_LfoCurrentPhase(int regionID)
{
    juce::Array<ModulatableParameter<double>*> parameters;

    RegionLfo* lfo = getLfo(regionID);
    if (lfo != nullptr)
    {
        parameters.add(lfo->getCurrentPhaseModParameter()); //only one LFO per region, so only one entry in parameters necessary
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
    specs.sampleRate = sampleRate;

    synth.setCurrentPlaybackSampleRate(sampleRate); // [3]
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        static_cast<Voice*>(synth.getVoice(i))->prepare(specs);
    }

    for (auto it = lfos.begin(); it != lfos.end(); ++it)
    {
        (*it)->prepare(specs);
    }

    DBG("AudioEngine prepared to play. sample rate: " + juce::String(sampleRate));
}
void AudioEngine::suspendProcessing(bool shouldBeSuspended)
{
    //synth.allNotesOff(0, false); //force stop all notes
    associatedProcessor.suspendProcessing(shouldBeSuspended);
}
bool AudioEngine::isSuspended()
{
    return associatedProcessor.isSuspended();
}

void AudioEngine::releaseResources()
{
    DBG("AudioEngine: releasing resources...");
    lfos.clear(true);
    DBG("AudioEngine: resources have been released.");
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    juce::MidiBuffer incomingMidi;
    midiCollector.removeNextBlockOfMessages(incomingMidi, bufferToFill.numSamples);
    keyboardState.processNextMidiBuffer(incomingMidi, bufferToFill.startSample, bufferToFill.numSamples, true);       // [4]
    //auto itMidi = incomingMidi.begin();
    //int nextMidiSamplePosition = -1;
    //if (itMidi != incomingMidi.end())
    //{
    //    nextMidiSamplePosition = static_cast<juce::MidiMessageMetadata>(*itMidi).samplePosition;
    //    DBG("first msg: " + juce::String(nextMidiSamplePosition));
    //}

    //evaluate all voices sample by sample! this is crucial since every voice *must* be rendered in sync with its LFO.
    //but since different voices' LFOs can influence one another, all voices must be rendered perfectly in sync as well!
    //the main disadvantage of this is that some processing (e.g. many types of filters, EQs or convolution-based effects)
    //might not be simple to implement (or even possible) anymore like this since they need to be rendered in block (since they require an FFT).
    //there may be workarounds for this, though.
    //if (nextMidiSamplePosition < 0)
    //{
        //no MIDI received
        for (int i = bufferToFill.startSample; i < bufferToFill.startSample + bufferToFill.numSamples; ++i)
        {
            synth.renderNextBlock(*bufferToFill.buffer, incomingMidi, i, 1); //sample by sample
        }
    //}
    //else
    //{
    //    //received MIDI
    //    for (int i = bufferToFill.startSample; i < bufferToFill.startSample + bufferToFill.numSamples; ++i)
    //    {
    //        if (nextMidiSamplePosition == i)
    //        {
    //            //evaluate MIDI message

    //            //associatedImage->handleMidiMessage(static_cast<juce::MidiMessageMetadata>(*itMidi).getMessage());
    //            ++itMidi; //move to next message
    //            if (itMidi != incomingMidi.end())
    //            {
    //                nextMidiSamplePosition = static_cast<juce::MidiMessageMetadata>(*itMidi).samplePosition; //update sample position
    //            }
    //            else
    //            {
    //                nextMidiSamplePosition = -1;
    //            }
    //        }

    //        synth.renderNextBlock(*bufferToFill.buffer, incomingMidi, i, 1); //sample by sample
    //    }
    //}
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

    if (lfoIndex < 0)
    {
        //no LFO with this ID found
        return;
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

    //lfos.remove(lfoIndex, true); //removed and deleted
    lfos.remove(lfoIndex, true);
}
