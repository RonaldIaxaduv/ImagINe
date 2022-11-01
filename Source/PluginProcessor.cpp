/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ImageINeDemoAudioProcessor::ImageINeDemoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    audioEngine(keyboardState, *this)
#endif
{
}

ImageINeDemoAudioProcessor::~ImageINeDemoAudioProcessor()
{
}

//==============================================================================
const juce::String ImageINeDemoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ImageINeDemoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ImageINeDemoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ImageINeDemoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ImageINeDemoAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ImageINeDemoAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ImageINeDemoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ImageINeDemoAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ImageINeDemoAudioProcessor::getProgramName (int index)
{
    return {};
}

void ImageINeDemoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ImageINeDemoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    audioEngine.prepareToPlay(samplesPerBlock, sampleRate);
}

void ImageINeDemoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ImageINeDemoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ImageINeDemoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    
    //for (int channel = 0; channel < totalNumInputChannels; ++channel)
    //{
    //    auto* channelData = buffer.getWritePointer (channel);

    //    // ..do something to the data...
    //}

    //audioEngine.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    auto asci = juce::AudioSourceChannelInfo(&buffer, 0, buffer.getNumSamples());
    audioEngine.getNextAudioBlock(asci);
}

//==============================================================================
bool ImageINeDemoAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ImageINeDemoAudioProcessor::createEditor()
{
    return new ImageINeDemoAudioProcessorEditor (*this);
}

//==============================================================================
void ImageINeDemoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    std::unique_ptr<juce::XmlElement> xml(new juce::XmlElement("ImageINe_Data"));
    juce::Array<juce::MemoryBlock> attachedData;
    
    xml->setAttribute("Plugin_Version", JucePlugin_VersionString);
    xml->setAttribute("Serialisation_Version", serialisation_version);

    audioEngine.serialise(xml.get(), &attachedData);
    
    copyXmlToBinary(*xml, destData);

    //TRICK: define an empty list of MemoryBlocks
    //-> pass a pointer to that list up to the serialisation of SegmentableImage and SegmentedRegion
    //-> whenever there's an object that cannot be converted to XML, convert it into a MemoryBlock, attach that block to the list and only save the *index* of the block in that list in the XML file
    //-> when the serialisation is done, the XML file is final (-> size know), and so are the memory blocks (-> size known). thus, they can be written to destData without any problems
    //  -> before writing the blocks, store the number of contained separate blocks! and for each block (XML included), prepend its size on the stream!
    //when deserialising, all information to separate the blocks from one another are contained on the stream.
    //-> restore the XML file and list and pass both into the deserialisation methods
    //-> whenever there's an object that couldn't be converted to XML, read its index in the list and simply convert the MemoryBlock back
    //  -> note: is endianness handled?
}

void ImageINeDemoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr && xmlState->hasTagName("ImageINe_Data"))
    {
        //WIP: check whether the XML's serialisation version (see const member variable) is ahead of the current serialisation version.
        //     if so, don't load it (-> error prompt)!

        audioEngine.deserialise(xmlState.get());
    }

}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImageINeDemoAudioProcessor();
}
