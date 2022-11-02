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
    //TRICK: define an empty list of MemoryBlocks
    //-> pass a pointer to that list up to the serialisation of SegmentableImage and SegmentedRegion
    //-> whenever there's an object that cannot be converted to XML (image/audio files), convert it into a MemoryBlock, attach that block to the list and only save the *index* of the block in that list in the XML file
    //-> when the serialisation is done, the XML file is final (-> size known), and so are the memory blocks (-> sizes known). thus, they can be written to destData without any problems
    //  -> before writing the blocks, store the number of contained separate blocks! and for each block (XML included), prepend its size on the stream!
    //when deserialising, all information to separate the blocks from one another are contained on the stream.
    //-> restore the XML file and list and pass both into the deserialisation methods
    //-> whenever there's an object that couldn't be converted to XML, read its index in the list and simply convert the MemoryBlock back
    //note: endianness is handled by using juce::MemoryInputStream and juce::MemoryOutputStream objects, which have methods to write variables with a certain endianness

    DBG("serialising all data...");

    std::unique_ptr<juce::XmlElement> xml(new juce::XmlElement("ImageINe_Data")); //XML file containing most of the data
    
    //some header attributes to enable dealing with backwards compatibility
    xml->setAttribute("Plugin_Version", JucePlugin_VersionString);
    xml->setAttribute("Serialisation_Version", serialisation_version);

    //serialise all data. data that cannot be converted to XML objects will be stored in additional attached memory blocks, instead. these blocks will be appended after the XML's data block.
    juce::Array<juce::MemoryBlock> attachedData;
    audioEngine.serialise(xml.get(), &attachedData);
    
    //convert XML file into a memory block
    juce::MemoryBlock xmlData = juce::MemoryBlock(); //the (temporary) memory block
    juce::MemoryOutputStream xmlOutStream = juce::MemoryOutputStream(xmlData, false);
    xml->writeToStream(xmlOutStream, juce::XmlElement::TextFormat::TextFormat().dtd); //write XML to the temporary memory block
    xmlOutStream.flush(); //if the block that's written on is user-supplied, it's safer to explicitly flush the stream before any more operations, apparently


    //first, write the XML block (including its size) onto the output block
    juce::MemoryOutputStream outStream = juce::MemoryOutputStream(destData, true); //stream that will write on the final block of memory
    outStream.writeString("ImageINe_Data_start"); //write an identifier so that the decoder can quickly check whether it's looking at a valid file

    juce::MemoryInputStream xmlInStream = juce::MemoryInputStream(xmlData, false);
    outStream.writeInt64(static_cast<juce::int64>(xmlData.getSize())); //write XML's size (required to restore the block)
    outStream.writeFromInputStream(xmlInStream, xmlData.getSize()); //write XML's content


    //then, write all of the attached data blocks (including their number and sizes) onto the output block
    outStream.writeInt(attachedData.size()); //write, how many attached data blocks there are (incl. if there are none)
    for (auto* itData = attachedData.begin(); itData != attachedData.end(); ++itData)
    {
        auto block = *itData;
        juce::MemoryInputStream blockStream = juce::MemoryInputStream(block, false);

        outStream.writeInt64(static_cast<juce::int64>(block.getSize())); //write block's size (required to restore the block)
        outStream.writeFromInputStream(blockStream, block.getSize()); //write block's content
    }

    DBG("all data has been serialised.");
}

void ImageINeDemoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    //used to restore data created by getStateInformation()

    DBG("deserialising all data...");

    juce::MemoryInputStream inStream = juce::MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false); //main stream for reading the stored data

    //check whether the file actually contains data for this program. to do so, it checks for the identifier that's written in front of all files.
    juce::String identifier = inStream.readString();
    if (identifier != "ImageINe_Data_start")
    {
        //throw std::exception("invalid file.");
        DBG("invalid file. deserialisation aborted.");
        return; //do nothing
    }
    //else: valid file -> go on

    //extract the contained XML file
    size_t xmlSize = inStream.readInt64();
    juce::MemoryBlock xmlData = juce::MemoryBlock(xmlSize);
    juce::MemoryOutputStream xmlOutStream = juce::MemoryOutputStream(xmlData, false);
    xmlOutStream.writeFromInputStream(inStream, xmlSize); //copy data from the input stream to the XML's memory block
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(xmlData.getData(), xmlSize)); //convert memory block back to an XML file
    
    if (xmlState.get() != nullptr && xmlState->hasTagName("ImageINe_Data"))
    {
        //valid XML file -> go on

        //WIP: check whether the XML's serialisation version (see const member variable) is ahead of the current serialisation version.
        //     if so, don't load it (-> error prompt)!

        //extract attached data blocks (contain image/audio files)
        juce::Array<juce::MemoryBlock> attachedData;
        int attachedDataSize = inStream.readInt(); //check how many attached objects there are in total
        for (int i = 0; i < attachedDataSize; ++i)
        {
            size_t blockSize = inStream.readInt64(); //size of the upcoming memory block
            juce::MemoryBlock block = juce::MemoryBlock(blockSize);
            juce::MemoryOutputStream blockStream = juce::MemoryOutputStream(block, false); //stream that writes data onto the block
            blockStream.writeFromInputStream(inStream, blockSize); //copy data from input stream to the block
            attachedData.add(block); //block completed -> append to list
        }

        //deserialise all data using the XML file and the attached data blocks
        audioEngine.deserialise(xmlState.get(), &attachedData);
    }
    else
    {
        DBG("invalid XML file. deserialisation aborted.");
        return; //do nothing
    }

    DBG("all data has been deserialised.");
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImageINeDemoAudioProcessor();
}
