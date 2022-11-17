/*
  ==============================================================================

    RegionEditor.cpp
    Created: 8 Jul 2022 10:00:28am
    Author:  Aaron

  ==============================================================================
*/

#include "RegionEditor.h"
#include "SegmentableImage.h"


//public

RegionEditor::RegionEditor(SegmentedRegion* region) :
    dahdsrEditor(juce::Array<DahdsrEnvelope*>()), //initialised with existing envelopes later
    lfoEditor(region->getAudioEngine(), region->getAssociatedLfo())
{
    associatedRegion = region;

    //file selection
    selectFileButton.setButtonText("Select Sound File");
    selectFileButton.onClick = [this] { selectFile(); };
    addChildComponent(selectFileButton);

    selectedFileLabel.setText("Please select a file", juce::NotificationType::dontSendNotification); //WIP: changes to "Selected file: (file name)" once a file has been selected
    addChildComponent(selectedFileLabel);
    selectedFileLabel.attachToComponent(&selectFileButton, false);

    //colour picker
    colourPickerWIP.setText("[colour picker WIP]", juce::NotificationType::dontSendNotification);
    //colourPickerWIP.setColour(...); //when a region has actually been selected, the background colour of this component will change
    addChildComponent(colourPickerWIP);

    //focus position + LFO depth
    focusPositionX.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    focusPositionX.setIncDecButtonsMode(juce::Slider::IncDecButtonMode::incDecButtonsDraggable_Vertical);
    focusPositionX.setNumDecimalPlacesToDisplay(3);
    focusPositionX.setRange(0.0, 1.0, 0.001);
    focusPositionX.onValueChange = [this]
    {
        updateFocusPosition();
        if (focusPositionX.getThumbBeingDragged() < 0)
        {
            renderLfoWaveform();
        }
    };
    focusPositionX.onDragEnd = [this] { renderLfoWaveform(); };
    addChildComponent(focusPositionX);
    lfoDepth.setText("LFO depth: ", juce::NotificationType::dontSendNotification);
    lfoDepth.attachToComponent(&focusPositionX, false); //above!
    addChildComponent(lfoDepth);

    focusPositionY.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    focusPositionY.setIncDecButtonsMode(juce::Slider::IncDecButtonMode::incDecButtonsDraggable_Vertical);
    focusPositionY.setNumDecimalPlacesToDisplay(3);
    focusPositionY.setRange(0.0, 1.0, 0.001);
    focusPositionY.onValueChange = [this]
    {
        updateFocusPosition();
        if (focusPositionY.getThumbBeingDragged() < 0)
        {
            renderLfoWaveform();
        }
    };
    focusPositionY.onDragEnd = [this] { renderLfoWaveform(); };
    addChildComponent(focusPositionY);

    focusPositionLabel.setText("Focus: ", juce::NotificationType::dontSendNotification);
    addChildComponent(focusPositionLabel);
    focusPositionLabel.attachToComponent(&focusPositionX, true);

    //toggle mode
    toggleModeButton.setButtonText("Toggle Mode");
    addChildComponent(toggleModeButton);

    //DAHDSR
    if (region != nullptr)
    {
        juce::Array<DahdsrEnvelope*> associatedEnvelopes;
        juce::Array<Voice*> associatedVoices = region->getAssociatedVoices();

        for (auto itVoice = associatedVoices.begin(); itVoice != associatedVoices.end(); itVoice++)
        {
            associatedEnvelopes.add((*itVoice)->getEnvelope());
        }

        dahdsrEditor.setAssociatedEnvelopes(associatedEnvelopes);
    }
    dahdsrEditor.setDisplayedName("Amp");
    addChildComponent(dahdsrEditor);

    //volume
    volumeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    volumeSlider.setTextValueSuffix("dB");
    volumeSlider.setRange(-60.0, 6.0, 0.01);
    volumeSlider.setSkewFactorFromMidPoint(-6.0);
    volumeSlider.onValueChange = [this] { updateVolume(); };
    volumeSlider.setValue(-6.0, juce::NotificationType::dontSendNotification);
    addChildComponent(volumeSlider);

    volumeLabel.setText("Volume: ", juce::NotificationType::dontSendNotification);
    addChildComponent(volumeLabel);
    volumeLabel.attachToComponent(&volumeSlider, true);

    //pitch
    pitchSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    pitchSlider.setTextValueSuffix("st");
    pitchSlider.setRange(-60.0, 60.0, 0.1);
    pitchSlider.onValueChange = [this] { updatePitch(); };
    pitchSlider.setValue(0.0, juce::NotificationType::dontSendNotification);
    addChildComponent(pitchSlider);

    pitchLabel.setText("Pitch: ", juce::NotificationType::dontSendNotification);
    addChildComponent(pitchLabel);
    pitchLabel.attachToComponent(&pitchSlider, true);

    pitchQuantisationChoice.addItem("Continuous (no quantisation)", static_cast<int>(PitchQuantisationMethod::continuous) + 1); //always adding 1 because 0 is not a valid ID (reserved for other purposes)
    pitchQuantisationChoice.addItem("Semitones", static_cast<int>(PitchQuantisationMethod::semitones) + 1);
    pitchQuantisationChoice.addItem("Scale: Major", static_cast<int>(PitchQuantisationMethod::scale_major) + 1);
    pitchQuantisationChoice.addItem("Scale: Minor", static_cast<int>(PitchQuantisationMethod::scale_minor) + 1);
    pitchQuantisationChoice.addItem("Scale: Octaves", static_cast<int>(PitchQuantisationMethod::scale_octaves) + 1);
    pitchQuantisationChoice.onChange = [this]
    {
        auto voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());

        for (auto it = voices.begin(); it != voices.end(); it++)
        {
            (*it)->setPitchQuantisationMethod(static_cast<PitchQuantisationMethod>(pitchQuantisationChoice.getSelectedId() - 1)); //set the new pitch quantisation method for all associated voices
        }
    };
    addChildComponent(pitchQuantisationChoice);

    pitchQuantisationLabel.setText("Pitch Quantisation: ", juce::NotificationType::dontSendNotification);
    addChildComponent(pitchQuantisationLabel);
    pitchQuantisationLabel.attachToComponent(&pitchQuantisationChoice, true);

    //MIDI listening
    midiChannelChoice.addItem("None", -1); 
    midiChannelChoice.addItem("Any", 1); //this and all following IDs will be decreased by 1 when passed to the region
    midiChannelChoice.addItem("1", 2);
    midiChannelChoice.addItem("2", 3);
    midiChannelChoice.addItem("3", 4);
    midiChannelChoice.addItem("4", 5);
    midiChannelChoice.addItem("5", 6);
    midiChannelChoice.addItem("6", 7);
    midiChannelChoice.addItem("7", 8);
    midiChannelChoice.addItem("8", 9);
    midiChannelChoice.addItem("9", 10);
    midiChannelChoice.addItem("10", 11);
    midiChannelChoice.addItem("11", 12);
    midiChannelChoice.addItem("12", 13);
    midiChannelChoice.addItem("13", 14);
    midiChannelChoice.addItem("14", 15);
    midiChannelChoice.addItem("15", 16);
    midiChannelChoice.addItem("16", 17);
    midiChannelChoice.onChange = [this]
    {
        if (midiChannelChoice.getSelectedId() < 0)
        {
            associatedRegion->setMidiChannel(-1);
        }
        else if (midiChannelChoice.getSelectedId() > 0)
        {
            associatedRegion->setMidiChannel(midiChannelChoice.getSelectedId() - 1);
        }
    };
    addChildComponent(midiChannelChoice);

    midiChannelLabel.setText("MIDI Channel: ", juce::NotificationType::dontSendNotification);
    midiChannelLabel.attachToComponent(&midiChannelChoice, true);
    addChildComponent(midiChannelLabel);

    //list MIDI notes from the highest to the lowest. only the 88 keys on a piano are used for simplicity
    midiNoteChoice.addItem("none", -1);
    midiNoteChoice.addItem("C8", 108);
    for (int octave = 7; octave >= 1; --octave)
    {
        midiNoteChoice.addItem("B" + juce::String(octave), 23 + 12 * octave);
        midiNoteChoice.addItem("A#" + juce::String(octave) + " / Bb" + juce::String(octave), 22 + 12 * octave);
        midiNoteChoice.addItem("A" + juce::String(octave), 21 + 12 * octave);
        midiNoteChoice.addItem("G#" + juce::String(octave) + " / Ab" + juce::String(octave), 20 + 12 * octave);
        midiNoteChoice.addItem("G" + juce::String(octave), 19 + 12 * octave);
        midiNoteChoice.addItem("F#" + juce::String(octave) + " / Gb" + juce::String(octave), 18 + 12 * octave);
        midiNoteChoice.addItem("F" + juce::String(octave), 17 + 12 * octave);
        midiNoteChoice.addItem("E" + juce::String(octave), 16 + 12 * octave);
        midiNoteChoice.addItem("D#" + juce::String(octave) + " / Eb" + juce::String(octave), 15 + 12 * octave);
        midiNoteChoice.addItem("D" + juce::String(octave), 14 + 12 * octave);
        midiNoteChoice.addItem("C#" + juce::String(octave) + " / Db" + juce::String(octave), 13 + 12 * octave);
        midiNoteChoice.addItem("C" + juce::String(octave), 12 + 12 * octave);
    }
    midiNoteChoice.addItem("B0", 23);
    midiNoteChoice.addItem("A#0 / Bb0", 22);
    midiNoteChoice.addItem("A0", 21);
    midiNoteChoice.onChange = [this]
    {
        associatedRegion->setMidiNote(midiNoteChoice.getSelectedId());
    };
    addChildComponent(midiNoteChoice);

    midiNoteLabel.setText("MIDI Note: ", juce::NotificationType::dontSendNotification);
    midiNoteLabel.attachToComponent(&midiNoteChoice, true);
    addChildComponent(midiNoteLabel);


    //LFO editor
    addChildComponent(lfoEditor);

    //other preparations
    copyRegionParameters();
    setChildVisibility(true);

    setSize(350, 700);

    formatManager.registerBasicFormats();
}

RegionEditor::~RegionEditor()
{
    DBG("destroying RegionEditor...");

    associatedRegion = nullptr;

    DBG("RegionEditor destroyed.");
}

void RegionEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour(juce::Colours::lightgrey);
    g.drawRect(getLocalBounds(), 1);   // draw an outline around the component
}

void RegionEditor::resized()
{
    auto area = getLocalBounds();

    selectedFileLabel.setBounds(area.removeFromTop(20));
    selectFileButton.setBounds(area.removeFromTop(20));

    colourPickerWIP.setBounds(area.removeFromTop(20));

    auto focusArea = area.removeFromTop(20);
    focusArea.removeFromLeft(focusArea.getWidth() / 3);
    focusPositionX.setBounds(focusArea.removeFromLeft(focusArea.getWidth() / 2));
    focusPositionY.setBounds(focusArea);

    toggleModeButton.setBounds(area.removeFromTop(20));
    toggleModeButton.onClick = [this] { updateToggleable(); };

    dahdsrEditor.setBounds(area.removeFromTop(80));

    auto volumeArea = area.removeFromTop(20);
    volumeLabel.setBounds(volumeArea.removeFromLeft(volumeArea.getWidth() / 3));
    volumeSlider.setBounds(volumeArea);

    auto pitchArea = area.removeFromTop(20);
    pitchLabel.setBounds(pitchArea.removeFromLeft(pitchArea.getWidth() / 3));
    pitchSlider.setBounds(pitchArea);

    auto pitchQuantisationArea = area.removeFromTop(20);
    pitchQuantisationLabel.setBounds(pitchQuantisationArea.removeFromLeft(pitchQuantisationArea.getWidth() / 3));
    pitchQuantisationChoice.setBounds(pitchQuantisationArea);

    auto midiArea = area.removeFromTop(20);
    midiChannelLabel.setBounds(midiArea.removeFromLeft(midiArea.getWidth() / 3));
    midiChannelChoice.setBounds(midiArea);
    midiArea = area.removeFromTop(20);
    midiNoteLabel.setBounds(midiArea.removeFromLeft(midiArea.getWidth() / 3));
    midiNoteChoice.setBounds(midiArea);

    lfoEditor.setBounds(area); //fill rest with lfoEditor
}

SegmentedRegion* RegionEditor::getAssociatedRegion()
{
    return associatedRegion;
}

void RegionEditor::refreshParameters()
{
    copyRegionParameters();
}




//private

void RegionEditor::setChildVisibility(bool shouldBeVisible)
{
    selectedFileLabel.setVisible(shouldBeVisible);
    selectFileButton.setVisible(shouldBeVisible);

    colourPickerWIP.setVisible(false); //WIP - probably not necessary anymore tbh

    focusPositionLabel.setVisible(shouldBeVisible);
    focusPositionX.setVisible(shouldBeVisible);
    focusPositionY.setVisible(shouldBeVisible);
    lfoDepth.setVisible(shouldBeVisible);

    toggleModeButton.setVisible(shouldBeVisible);

    dahdsrEditor.setVisible(shouldBeVisible);

    volumeLabel.setVisible(shouldBeVisible);
    volumeSlider.setVisible(shouldBeVisible);

    pitchLabel.setVisible(shouldBeVisible);
    pitchSlider.setVisible(shouldBeVisible);
    pitchQuantisationLabel.setVisible(shouldBeVisible);
    pitchQuantisationChoice.setVisible(shouldBeVisible);

    midiChannelChoice.setVisible(shouldBeVisible);
    midiChannelLabel.setVisible(shouldBeVisible);
    midiNoteChoice.setVisible(shouldBeVisible);
    midiNoteLabel.setVisible(shouldBeVisible);

    lfoEditor.setVisible(shouldBeVisible);
}

void RegionEditor::copyRegionParameters()
{
    DBG("copying region parameters...");

    if (associatedRegion->getFileName() == "")
        selectedFileLabel.setText("Please select a file", juce::NotificationType::dontSendNotification);
    else
        selectedFileLabel.setText("Selected file: " + associatedRegion->getFileName(), juce::NotificationType::dontSendNotification);

    focusPositionX.setValue(associatedRegion->getFocusPoint().getX(), juce::NotificationType::dontSendNotification);
    focusPositionY.setValue(associatedRegion->getFocusPoint().getY(), juce::NotificationType::dontSendNotification);

    toggleModeButton.setToggleState(associatedRegion->getShouldBeToggleable(), juce::NotificationType::dontSendNotification);

    juce::Array<Voice*> voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());
    //Voice* voice = associatedRegion->getAssociatedVoice();
    if (voices.size() > 0) //(voice != nullptr)
    {
        Voice* voice = voices[0]; //all voices have the same parameters, so it's enough to always look at the first element

        volumeSlider.setValue(juce::Decibels::gainToDecibels<double>(voice->getBaseLevel(), -60.0), juce::NotificationType::dontSendNotification);
        pitchSlider.setValue(voice->getBasePitchShift(), juce::NotificationType::dontSendNotification);
        pitchQuantisationChoice.setEnabled(true);
        pitchQuantisationChoice.setSelectedId(static_cast<int>(voice->getPitchQuantisationMethod()) + 1);
    }
    else
    {
        pitchQuantisationChoice.setEnabled(false);
    }

    int midiChannel = associatedRegion->getMidiChannel();
    if (midiChannel < 0)
    {
        midiChannelChoice.setSelectedId(-1);
    }
    else
    {
        midiChannelChoice.setSelectedId(midiChannel + 1);
    }
    midiNoteChoice.setSelectedId(associatedRegion->getMidiNote());

    lfoEditor.updateAvailableVoices();
    lfoEditor.copyParameters();

    lfoDepth.setText("LFO depth: " + juce::String(associatedRegion->getAssociatedLfo()->getDepth()), juce::NotificationType::dontSendNotification);

    DBG("region parameters have been copied.");
}

void RegionEditor::selectFile()
{
    //shutdownAudio();

    fc = std::make_unique<juce::FileChooser>("Select a Wave file shorter than 2 seconds to play...",
        juce::File(R"(C:\Users\Aaron\Desktop\Musikproduktion\VSTs\Iris 2\Iris 2 Library\Samples)"),
        formatManager.getWildcardForAllFormats());
    auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;

    fc->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();

            if (file == juce::File{})
                return;

            std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file)); // [2]

            if (reader.get() != nullptr)
            {
                auto duration = (float)reader->lengthInSamples / reader->sampleRate;               // [3]

                associatedRegion->getAudioEngine()->suspendProcessing(true); //pause the audio engine to ensure that loading the file doesn't change the audio thread's priority

                //juce::AudioSampleBuffer tempBuffer();
                tempBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);  // [4]
                reader->read(&tempBuffer,                                                      // [5]
                    0,                                                                //  [5.1]
                    (int)reader->lengthInSamples,                                    //  [5.2]
                    0,                                                                //  [5.3]
                    true,                                                             //  [5.4]
                    true);                                                            //  [5.5]
                //position = 0;                                                                   // [6]
                //setAudioChannels(0, (int)reader->numChannels);                                // [7]

                associatedRegion->setBuffer(tempBuffer, file.getFileName(), reader->sampleRate);
                selectedFileLabel.setText(file.getFileName(), juce::NotificationType::dontSendNotification);

                lfoEditor.updateAvailableVoices();
                updateAllVoiceSettings(); //sets currently selected volume, pitch etc.

                juce::Array<DahdsrEnvelope*> associatedEnvelopes;
                juce::Array<Voice*> associatedVoices = associatedRegion->getAssociatedVoices();
                for (auto itVoice = associatedVoices.begin(); itVoice != associatedVoices.end(); itVoice++)
                {
                    associatedEnvelopes.add((*itVoice)->getEnvelope());
                }
                dahdsrEditor.setAssociatedEnvelopes(associatedEnvelopes);

                copyRegionParameters(); //updates pitch quantisation, makes sure nothing's missing

                associatedRegion->getAudioEngine()->suspendProcessing(false);
            }
            else
            {
                juce::NativeMessageBox::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "ImageINe - unknown format",
                    "The file couldn't be opened. This is most likely because the format isn't supported.",
                    this, nullptr);
            }
        });
}

void RegionEditor::updateFocusPosition()
{
    associatedRegion->setFocusPoint(juce::Point<float>(static_cast<float>(focusPositionX.getValue()), static_cast<float>(focusPositionY.getValue())));
}

void RegionEditor::renderLfoWaveform()
{
    associatedRegion->renderLfoWaveform();
    lfoDepth.setText("LFO depth: " + juce::String(associatedRegion->getAssociatedLfo()->getDepth()), juce::NotificationType::dontSendNotification);
}

void RegionEditor::updateToggleable()
{
    associatedRegion->setShouldBeToggleable(toggleModeButton.getToggleState());
}

void RegionEditor::updateAllVoiceSettings() //used after a voice has been first set or changed
{
    updateVolume();
    updatePitch();
    updatePlaybackPosition();
}
void RegionEditor::updateVolume()
{
    juce::Array<Voice*> voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        (*it)->setBaseLevel(juce::Decibels::decibelsToGain<double>(volumeSlider.getValue(), -60.0));
    }
}
void RegionEditor::updatePitch()
{
    juce::Array<Voice*> voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        (*it)->setBasePitchShift(pitchSlider.getValue());
    }
}
void RegionEditor::updatePlaybackPosition()
{
    juce::Array<Voice*> voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        //(*it)->setBasePlaybackPosition(playbackPositionSlider.getValue());
    }
}
