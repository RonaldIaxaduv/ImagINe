/*
  ==============================================================================

    RegionEditor.cpp
    Created: 8 Jul 2022 10:00:28am
    Author:  Aaron

  ==============================================================================
*/

#include "RegionEditor.h"


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

    //focus position
    focusPositionX.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    focusPositionX.setIncDecButtonsMode(juce::Slider::IncDecButtonMode::incDecButtonsDraggable_Vertical);
    focusPositionX.setNumDecimalPlacesToDisplay(3);
    focusPositionX.setRange(0.0, 1.0, 0.001);
    focusPositionX.onValueChange = [this]
    {
        updateFocusPosition();
        if (focusPositionX.getThumbBeingDragged() < 0)
            renderLfoWaveform();
    };
    focusPositionX.onDragEnd = [this] { renderLfoWaveform(); };
    addChildComponent(focusPositionX);

    focusPositionY.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    focusPositionY.setIncDecButtonsMode(juce::Slider::IncDecButtonMode::incDecButtonsDraggable_Vertical);
    focusPositionY.setNumDecimalPlacesToDisplay(3);
    focusPositionY.setRange(0.0, 1.0, 0.001);
    focusPositionY.onValueChange = [this]
    {
        updateFocusPosition();
        if (focusPositionY.getThumbBeingDragged() < 0)
            renderLfoWaveform();
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

    //LFO editor
    addChildComponent(lfoEditor);

    //delete region
    deleteRegionButton.setButtonText("Delete Region");
    deleteRegionButton.onClick = [this] { deleteRegion(); };
    addChildComponent(deleteRegionButton);

    //other preparations
    copyRegionParameters();
    setChildVisibility(true);

    setSize(350, 700);

    formatManager.registerBasicFormats();
}

RegionEditor::~RegionEditor()
{
    associatedRegion = nullptr;
}

void RegionEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour(juce::Colours::lightgrey);
    g.drawRect(getLocalBounds(), 1);   // draw an outline around the component
}

void RegionEditor::resized()
{
    if (associatedRegion != nullptr)
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



        deleteRegionButton.setBounds(area.removeFromBottom(20));

        lfoEditor.setBounds(area); //fill rest with lfoEditor
    }
}

SegmentedRegion* RegionEditor::getAssociatedRegion()
{
    return associatedRegion;
}




//private

void RegionEditor::setChildVisibility(bool shouldBeVisible)
{
    selectedFileLabel.setVisible(shouldBeVisible);
    selectFileButton.setVisible(shouldBeVisible);

    colourPickerWIP.setVisible(shouldBeVisible);

    focusPositionLabel.setVisible(shouldBeVisible);
    focusPositionX.setVisible(shouldBeVisible);
    focusPositionY.setVisible(shouldBeVisible);

    toggleModeButton.setVisible(shouldBeVisible);

    dahdsrEditor.setVisible(shouldBeVisible);

    volumeLabel.setVisible(shouldBeVisible);
    volumeSlider.setVisible(shouldBeVisible);

    pitchLabel.setVisible(shouldBeVisible);
    pitchSlider.setVisible(shouldBeVisible);
    pitchQuantisationLabel.setVisible(shouldBeVisible);
    pitchQuantisationChoice.setVisible(shouldBeVisible);

    lfoEditor.setVisible(shouldBeVisible);

    deleteRegionButton.setVisible(shouldBeVisible);
}

void RegionEditor::copyRegionParameters()
{
    //ä //WIP
    DBG("[copy region parameters WIP]");

    if (associatedRegion->getFileName() == "")
        selectedFileLabel.setText("Please select a file", juce::NotificationType::dontSendNotification);
    else
        selectedFileLabel.setText("Selected file: " + associatedRegion->getFileName(), juce::NotificationType::dontSendNotification);

    focusPositionX.setValue(associatedRegion->focus.getX(), juce::NotificationType::dontSendNotification);
    focusPositionY.setValue(associatedRegion->focus.getY(), juce::NotificationType::dontSendNotification);

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

    lfoEditor.copyParameters();
}

void RegionEditor::selectFile()
{
    //shutdownAudio();

    fc = std::make_unique<juce::FileChooser>("Select a Wave file shorter than 2 seconds to play...",
        juce::File(R"(C:\Users\Aaron\Desktop\Musikproduktion\VSTs\Iris 2\Iris 2 Library\Samples)"),
        "*.wav");
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

                if (true)//(duration <= 7)
                {
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
                }
                else
                {
                    // handle the error that the file is 2 seconds or longer..
                    DBG("[invalid file - callback WIP]");

                    associatedRegion->setBuffer(juce::AudioSampleBuffer(), "", 0.0); //empty buffer
                }
            }
        });
}

void RegionEditor::updateFocusPosition()
{
    associatedRegion->focus.setX(static_cast<float>(focusPositionX.getValue()));
    associatedRegion->focus.setY(static_cast<float>(focusPositionY.getValue()));
    associatedRegion->repaint();
}

void RegionEditor::renderLfoWaveform()
{
    associatedRegion->renderLfoWaveform();
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

void RegionEditor::deleteRegion()
{
    //WIP: later, this will remove the associated Region from the canvas
}
