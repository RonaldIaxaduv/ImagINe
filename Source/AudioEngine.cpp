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




AudioEngine::AudioEngine(juce::MidiKeyboardState& keyState, juce::AudioProcessor& associatedProcessor) :
    keyboardState(keyState), associatedProcessor(associatedProcessor),
    specs()
{
    //for (auto i = 0; i < 4; ++i)                // [1]
    //    synth.addVoice(new Voice());

    //synth.addSound(new SamplerOscillator());       // [2]

    synth.addSound(new TempSound());
}
AudioEngine::~AudioEngine()
{
    DBG("destroying AudioEngine...");

    associatedImage = nullptr;

    lfos.clear(true);

    juce::AudioSource::~AudioSource();

    DBG("AudioEngine destroyed.");
}

void AudioEngine::serialise(juce::XmlElement* xml, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("serialising AudioEngine...");
    juce::XmlElement* xmlAudioEngine = xml->createNewChildElement("AudioEngine");

    //serialise image data (includes SegmentedRegions)
    serialiseImage(xmlAudioEngine, attachedData);

    //serialise audio engine's region data
    xmlAudioEngine->setAttribute("regionIdCounter", regionIdCounter);
    serialiseRegionColours(xmlAudioEngine);

    //serialise LFO data
    serialiseLFOs(xmlAudioEngine);

    //serialise voice data
    serialiseVoices(xmlAudioEngine);

    DBG("AudioEngine has been serialised.");
}
void AudioEngine::serialiseRegionColours(juce::XmlElement* xmlAudioEngine)
{
    DBG("serialising regionColours...");
    juce::XmlElement* xmlRegionColours = xmlAudioEngine->createNewChildElement("regionColours");

    xmlRegionColours->setAttribute("size", regionColours.size());

    int i = 0;
    for (auto itColour = regionColours.begin(); itColour != regionColours.end(); ++itColour, ++i)
    {
        xmlRegionColours->setAttribute("Colour_" + juce::String(i), itColour->toString()); //can be converted back via fromString()
    }

    DBG("regionColours has been serialised.");
}
void AudioEngine::serialiseLFOs(juce::XmlElement* xmlAudioEngine)
{
    xmlAudioEngine->setAttribute("lfos_size", lfos.size());

    for (auto itLfo = lfos.begin(); itLfo != lfos.end(); ++itLfo)
    {
        juce::XmlElement* xmlLfo = xmlAudioEngine->createNewChildElement("LFO_" + juce::String((*itLfo)->getRegionID()));
        (*itLfo)->serialise(xmlLfo);
    }
}
void AudioEngine::serialiseVoices(juce::XmlElement* xmlAudioEngine)
{
    //note: serialise only ONE voice per region, and the total number of voices for that region
    //-> pass the same XmlElement to all [number of voices] Voice members to initialise them all with the same values

    xmlAudioEngine->setAttribute("synth_numVoices", synth.getNumVoices());

    for (int id = 0; id <= regionIdCounter; ++id)
    {
        auto voices = getVoicesWithID(id);
        
        if (voices.size() > 0) //only create voice data for regions that exist
        {
            juce::XmlElement* xmlVoices = xmlAudioEngine->createNewChildElement("Voices_" + juce::String(id));
            voices[0]->serialise(xmlVoices); //it's enough to serialise one voice per region, because all other voices have exactly the same parameters
        }
    }
}
void AudioEngine::serialiseImage(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData)
{
    associatedImage->serialise(xmlAudioEngine, attachedData);
}

void AudioEngine::deserialise(juce::XmlElement* xml, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("deserialising AudioEngine...");
    juce::XmlElement* xmlAudioEngine = xml->getChildByName("AudioEngine");

    if (xmlAudioEngine != nullptr)
    {
        //deserialise image data (includes SegmentedRegions)
        deserialiseImage(xmlAudioEngine, attachedData);

        //deserialise audio engine's region data
        regionIdCounter = xmlAudioEngine->getIntAttribute("regionIdCounter", 0);
        deserialiseRegionColours(xmlAudioEngine);

        //deserialise LFO data (excluding mods)
        deserialiseLFOs_main(xmlAudioEngine);

        //deserialise voice data
        deserialiseVoices(xmlAudioEngine);

        //deserialise LFO mods (now that all regions, LFOs and voices have been deserialised)
        deserialiseLFOs_mods(xmlAudioEngine);

        DBG("AudioEngine has been deserialised.");
    }
    else
    {
        DBG("no AudioEngine data found.");
    }
}
void AudioEngine::deserialiseRegionColours(juce::XmlElement* xmlAudioEngine)
{
    DBG("deserialising regionColours...");
    juce::XmlElement* xmlRegionColours = xmlAudioEngine->getChildByName("regionColours");

    if (xmlRegionColours != nullptr)
    {
        regionColours.clear();

        int size = xmlRegionColours->getIntAttribute("size");
        for (int i = 0; i < size; ++i)
        {
            regionColours.add(juce::Colour::fromString(xmlRegionColours->getStringAttribute("Colour_" + juce::String(i))));
        }

        DBG("regionColours has been deserialised.");
    }
    else
    {
        DBG("no regionColours data found.");
    }
}
void AudioEngine::deserialiseLFOs_main(juce::XmlElement* xmlAudioEngine)
{
    int size = xmlAudioEngine->getIntAttribute("lfos_size", 0);

    jassert(size == lfos.size()); //LFOs should already be initialised at this point, because the SegmentedRegion objects are deserialised first

    for (int i = 0; i <= regionIdCounter; ++i)
    {
        juce::XmlElement* xmlLfo = xmlAudioEngine->getChildByName("LFO_" + juce::String(lfos[i]->getRegionID()));

        if (xmlLfo != nullptr)
        {
            lfos[i]->deserialise_main(xmlLfo);
        }
        else
        {
            DBG("LFO data of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO could not be found.");
            //don't do anything, leave the LFO at its initial state
        }
    }
}
void AudioEngine::deserialiseLFOs_mods(juce::XmlElement* xmlAudioEngine)
{
    DBG("Deserialising LFO mods...");

    int size = xmlAudioEngine->getIntAttribute("lfos_size", 0);

    jassert(size == lfos.size()); //LFOs should already be initialised at this point, because the SegmentedRegion objects are deserialised first

    for (int i = 0; i <= regionIdCounter; ++i)
    {
        juce::XmlElement* xmlLfo = xmlAudioEngine->getChildByName("LFO_" + juce::String(lfos[i]->getRegionID()));

        if (xmlLfo != nullptr)
        {
            //it's easier to directly implement this here instead of using a method lfos[i]->deserialise_mods(xmlLfo), because deserialising the mods requires access to the AudioEngine, which the LFOs don't have (and would be messy to implement)

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
                continue;
            }

            if (pIDs.size() != rIDs.size())
            {
                DBG("ID sizes of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO don't match!");
                continue;
            }

            juce::Array<ModulatableParameter<double>*> affectedParams;
            for (int j = 0; j < size; ++j)
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

                case LfoModulatableParameter::playbackPosition:
                case LfoModulatableParameter::playbackPosition_inverted:
                    affectedParams = getParameterOfRegion_PlaybackPosition(rIDs[j]);
                    break;

                case LfoModulatableParameter::lfoRate:
                case LfoModulatableParameter::lfoRate_inverted:
                    affectedParams = getParameterOfRegion_LfoRate(rIDs[j]);
                    break;

                case LfoModulatableParameter::lfoPhase:
                case LfoModulatableParameter::lfoPhase_inverted:
                    affectedParams = getParameterOfRegion_LfoPhase(rIDs[j]);
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
        }
        else
        {
            DBG("LFO data of region " + juce::String(lfos[i]->getRegionID()) + "'s LFO could not be found.");
            //don't do anything, leave the LFO at its initial state (without mods)
        }
    }

    DBG("LFO mods have been deserialised.");
}
void AudioEngine::deserialiseVoices(juce::XmlElement* xmlAudioEngine)
{
    //note: serialise only ONE voice per region, and the total number of voices for that region
    //-> pass the same XmlElement to all [number of voices] Voice members to initialise them all with the same values

    for (int id = 0; id <= regionIdCounter; ++id)
    {
        juce::XmlElement* xmlVoices = xmlAudioEngine->getChildByName("Voices_" + juce::String(id));

        if (xmlVoices != nullptr) //only apply voice data for regions that exist
        {
            //note: the correct polyphony is already ensured within the deserialisation of the associated SegmentedRegion object

            //load data of all voices (excluding the sound data, which is loaded from their associated SegmentedRegion)
            auto voices = getVoicesWithID(id);
            for (auto itVoice = voices.begin(); itVoice != voices.end(); ++itVoice)
            {
                (*itVoice)->deserialise(xmlVoices); //deserialise each voice with exactly the same parameters
            }
        }
        else
        {
            DBG("voice data of region " + juce::String(id) + "'s LFO could not be found. this isn't necessarily a problem because the code also checks for voices of already deleted regions.");
        }
    }

    jassert(synth.getNumVoices() == xmlAudioEngine->getIntAttribute("synth_numVoices", 0));
}
void AudioEngine::deserialiseImage(juce::XmlElement* xmlAudioEngine, juce::Array<juce::MemoryBlock>* attachedData)
{
    associatedImage->deserialise(xmlAudioEngine, attachedData);
}

void AudioEngine::registerImage(SegmentableImage* newAssociatedImage)
{
    associatedImage = newAssociatedImage;
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
void AudioEngine::resetRegionIDs()
{
    regionIdCounter = -1;
    regionColours.clear();
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
void AudioEngine::suspendProcessing(bool shouldBeSuspended)
{
    //synth.allNotesOff(0, false); //force stop all notes
    associatedProcessor.suspendProcessing(shouldBeSuspended);
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
