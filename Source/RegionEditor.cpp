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
    selectFileButton.setTooltip("Click here to select an audio file for the region to play.");
    addChildComponent(selectFileButton);

    selectedFileLabel.setText("Please select a file", juce::NotificationType::dontSendNotification); //WIP: changes to "Selected file: (file name)" once a file has been selected
    addChildComponent(selectedFileLabel);
    selectedFileLabel.attachToComponent(&selectFileButton, false);

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
    focusPositionX.setPopupMenuEnabled(true);
    focusPositionX.setTooltip("This value changes the horizontal position of the focus point (the dot within a region from which the LFO line originates). The higher the maximum length of the LFO is, the higher its modulation depth becomes.");
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
    focusPositionY.setPopupMenuEnabled(true);
    focusPositionY.setTooltip("This value changes the vertical position of the focus point (the dot within a region from which the Lfo line originates). The higher the maximum length of the LFO is, the higher its modulation depth becomes.");
    addChildComponent(focusPositionY);

    focusPositionLabel.setText("Focus: ", juce::NotificationType::dontSendNotification);
    addChildComponent(focusPositionLabel);
    focusPositionLabel.attachToComponent(&focusPositionX, true);

    //toggle mode
    toggleModeButton.setButtonText("Toggle Mode");
    toggleModeButton.onClick = [this] { updateToggleable(); };
    toggleModeButton.setTooltip("When toggle mode is off, regions will try to stop playing as soon as clicks/notes/play path interactions stop. When it's on, they will turn on on the first interaction and off after the second.");
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
    dahdsrEditor.setTooltip("This is the DAHDSR editor.");
    addChildComponent(dahdsrEditor);

    //volume
    volumeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    volumeSlider.setTextValueSuffix("dB");
    volumeSlider.setRange(-60.0, 6.0, 0.01);
    volumeSlider.setSkewFactorFromMidPoint(-6.0);
    volumeSlider.onValueChange = [this] { updateVolume(); };
    volumeSlider.setValue(-6.0, juce::NotificationType::dontSendNotification);
    volumeSlider.setPopupMenuEnabled(true);
    volumeSlider.setTooltip("This slider sets the base volume of your selected audio file.");
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
    pitchSlider.setPopupMenuEnabled(true); 
    pitchSlider.setTooltip("This slider offsets the base pitch of your selected audio file, e.g. to adjust the tuning. It's achieved by playing the audio at different speeds (slower = lower).");
    addChildComponent(pitchSlider);

    pitchLabel.setText("Pitch: ", juce::NotificationType::dontSendNotification);
    addChildComponent(pitchLabel);
    pitchLabel.attachToComponent(&pitchSlider, true);

    pitchQuantisationChoice.addSectionHeading("Classical");
    pitchQuantisationChoice.addItem("Continuous (no quantisation)", static_cast<int>(PitchQuantisationMethod::continuous) + 1); //always adding 1 because 0 is not a valid ID (reserved for other purposes)
    pitchQuantisationChoice.addItem("Semitones [12]", static_cast<int>(PitchQuantisationMethod::semitones) + 1);
    pitchQuantisationChoice.addItem("Scale: Major [7]", static_cast<int>(PitchQuantisationMethod::scale_major) + 1);
    pitchQuantisationChoice.addItem("Scale: Minor [7]", static_cast<int>(PitchQuantisationMethod::scale_minor) + 1);
    pitchQuantisationChoice.addItem("Scale: Pentatonic Major [5]", static_cast<int>(PitchQuantisationMethod::scale_pentatonicMajor) + 1);
    pitchQuantisationChoice.addItem("Scale: Pentatonic Minor [5]", static_cast<int>(PitchQuantisationMethod::scale_pentatonicMinor) + 1);
    pitchQuantisationChoice.addItem("Scale: Whole Tone [6]", static_cast<int>(PitchQuantisationMethod::scale_wholeTone) + 1);
    pitchQuantisationChoice.addSeparator();
    pitchQuantisationChoice.addSectionHeading("Reversed");
    pitchQuantisationChoice.addItem("Semitones Reversed [12]", static_cast<int>(PitchQuantisationMethod::scale_semitonesReversed) + 1);
    pitchQuantisationChoice.addItem("Scale: Major Reversed [7]", static_cast<int>(PitchQuantisationMethod::scale_majorReversed) + 1);
    pitchQuantisationChoice.addItem("Scale: Minor Reversed [7]", static_cast<int>(PitchQuantisationMethod::scale_minorReversed) + 1);
    pitchQuantisationChoice.addSeparator();
    pitchQuantisationChoice.addSectionHeading("Japanese");
    pitchQuantisationChoice.addItem("Scale: Min'yo [5]", static_cast<int>(PitchQuantisationMethod::scale_minyo) + 1);
    pitchQuantisationChoice.addItem("Scale: Miyako Bushi [5]", static_cast<int>(PitchQuantisationMethod::scale_miyakoBushi) + 1);
    pitchQuantisationChoice.addItem("Scale: Ritsu [5]", static_cast<int>(PitchQuantisationMethod::scale_ritsu) + 1);
    pitchQuantisationChoice.addItem("Scale: Ryo [5]", static_cast<int>(PitchQuantisationMethod::scale_ryo) + 1);
    pitchQuantisationChoice.addSeparator();
    pitchQuantisationChoice.addSectionHeading("Blues");
    pitchQuantisationChoice.addItem("Scale: Blues [6]", static_cast<int>(PitchQuantisationMethod::scale_bluesHexa) + 1);
    pitchQuantisationChoice.addItem("Scale: Blues [7]", static_cast<int>(PitchQuantisationMethod::scale_bluesHepta) + 1);
    pitchQuantisationChoice.addItem("Scale: Blues [9]", static_cast<int>(PitchQuantisationMethod::scale_bluesNona) + 1);
    pitchQuantisationChoice.addSeparator();
    pitchQuantisationChoice.addSectionHeading("Other");
    pitchQuantisationChoice.addItem("Scale: Half Up [2]", static_cast<int>(PitchQuantisationMethod::scale_halfUp) + 1);
    pitchQuantisationChoice.addItem("Scale: Half Down [2]", static_cast<int>(PitchQuantisationMethod::scale_halfDown) + 1);
    pitchQuantisationChoice.addItem("Scale: Hijaz Kar [7]", static_cast<int>(PitchQuantisationMethod::scale_hijazKar) + 1);
    pitchQuantisationChoice.addItem("Scale: Hungarian Minor [7]", static_cast<int>(PitchQuantisationMethod::scale_hungarianMinor) + 1);
    pitchQuantisationChoice.addItem("Scale: Octaves [1]", static_cast<int>(PitchQuantisationMethod::scale_octaves) + 1);
    pitchQuantisationChoice.addItem("Scale: Persian [7]", static_cast<int>(PitchQuantisationMethod::scale_persian) + 1);
    pitchQuantisationChoice.onChange = [this]
    {
        auto voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());

        for (auto it = voices.begin(); it != voices.end(); it++)
        {
            (*it)->setPitchQuantisationMethod(static_cast<PitchQuantisationMethod>(pitchQuantisationChoice.getSelectedId() - 1)); //set the new pitch quantisation method for all associated voices
        }
    };
    pitchQuantisationChoice.setTooltip("If you want pitch shifts to only hit notes on a certain scale, you can choose so here. This does not affect the pitch slider, only pitch modulation. The value in the square brackets is the number of notes on the scale within one octave (which has a total of 12 notes).");
    addChildComponent(pitchQuantisationChoice);

    pitchQuantisationLabel.setText("Pitch Quantisation: ", juce::NotificationType::dontSendNotification);
    addChildComponent(pitchQuantisationLabel);
    pitchQuantisationLabel.attachToComponent(&pitchQuantisationChoice, true);

    //playback position start
    playbackPositionStartSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    playbackPositionStartSlider.setRange(0.0, 0.999, 0.001); //for a 5-minute audio file, this granularity amounts to 0.3s -> should be enough
    playbackPositionStartSlider.onValueChange = [this] { updatePlaybackPositionStart(); };
    playbackPositionStartSlider.setValue(0.0, juce::NotificationType::dontSendNotification);
    playbackPositionStartSlider.setPopupMenuEnabled(true);
    playbackPositionStartSlider.setTooltip("This slider offsets the starting point of your audio file. At 0.0, it uses the original starting position, at 0.5 it starts at the halfway point, et cetera.");
    addChildComponent(playbackPositionStartSlider);

    playbackPositionStartLabel.setText("Playback Starting Position: ", juce::NotificationType::dontSendNotification);
    addChildComponent(playbackPositionStartLabel);
    playbackPositionStartLabel.attachToComponent(&playbackPositionStartSlider, true);

    //playback position interval
    playbackPositionIntervalSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    playbackPositionIntervalSlider.setRange(0.001, 1.0, 0.001); //MUST NOT BE ZERO!; for a 5-minute audio file, this granularity amounts to 0.3s -> should be enough
    playbackPositionIntervalSlider.onValueChange = [this] { updatePlaybackPositionInterval(); };
    playbackPositionIntervalSlider.setValue(0.0, juce::NotificationType::dontSendNotification);
    playbackPositionIntervalSlider.setPopupMenuEnabled(true);
    playbackPositionIntervalSlider.setTooltip("This slider sets the length of your audio file. At 1.0, it uses the original file length, at 0.5 it only plays half, et cetera. This can be especially interesting if you also shift the playback starting position (above).");
    addChildComponent(playbackPositionIntervalSlider);

    playbackPositionIntervalLabel.setText("Playback Interval: ", juce::NotificationType::dontSendNotification);
    addChildComponent(playbackPositionIntervalLabel);
    playbackPositionIntervalLabel.attachToComponent(&playbackPositionIntervalSlider, true);

    //MIDI listening
    midiChannelChoice.addItem("None", -1); 
    midiChannelChoice.addItem("Any", 1); //this and all following IDs will be decreased by 1 when passed to the region
    for (int i = 1; i <= 16; ++i)
    {
        midiChannelChoice.addItem(juce::String(i), i + 1);
    }
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
    midiChannelChoice.setTooltip("If you want to play this region via MIDI notes, you can select the channel that it should listen to here. Tip: there can be several regions responding to the same channel and note!");
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
    midiNoteChoice.setTooltip("If you want to play this region via MIDI notes, you can select the note that it should listen to here. Tip: there can be several regions responding to the same channel and note!");
    addChildComponent(midiNoteChoice);

    midiNoteLabel.setText("MIDI Note: ", juce::NotificationType::dontSendNotification);
    midiNoteLabel.attachToComponent(&midiNoteChoice, true);
    addChildComponent(midiNoteLabel);


    //LFO editor
    lfoEditor.setTooltip("This is the LFO editor.");
    addChildComponent(lfoEditor);

    //randomise button
    randomiseButton.setButtonText("Randomise Region Parameters");
    randomiseButton.onClick = [this] { randomiseAllParameters(); };
    randomiseButton.setTooltip("Clicking this button will randomise most of the parameters in this editor. The randomness is biased in a useful way to achieve useful results. You can also press Ctrl + r while hovering above a parameter to randomise only that one value.");
    addChildComponent(randomiseButton);

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
    int hUnit = juce::jmin(50, juce::jmax(5, static_cast<int>(static_cast<float>(getHeight()) * 0.75 / 21.0))); //unit of height required to squeeze all elements into 75% of the window's area (the remaining 25% are used for the modulation table)

    area.removeFromTop(hUnit);
    selectFileButton.setBounds(area.removeFromTop(hUnit).reduced(2));

    area.removeFromTop(hUnit); //make space for the LFO depth label

    auto focusArea = area.removeFromTop(hUnit);
    focusArea.removeFromLeft(focusArea.getWidth() / 3);
    focusPositionX.setBounds(focusArea.removeFromLeft(focusArea.getWidth() / 2).reduced(1));
    focusPositionY.setBounds(focusArea.reduced(1));
    focusPositionX.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, false, focusPositionX.getWidth(), focusPositionX.getHeight());
    focusPositionY.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxRight, false, focusPositionY.getWidth(), focusPositionY.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    toggleModeButton.setBounds(area.removeFromTop(hUnit).reduced(1));

    dahdsrEditor.setUnitOfHeight(hUnit);
    dahdsrEditor.setBounds(area.removeFromTop(hUnit * 4).reduced(1));

    auto volumeArea = area.removeFromTop(hUnit);
    volumeSlider.setBounds(volumeArea.removeFromRight(2 * volumeArea.getWidth() / 3).reduced(1));
    volumeSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, volumeSlider.getWidth(), volumeSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto pitchArea = area.removeFromTop(hUnit);
    pitchSlider.setBounds(pitchArea.removeFromRight(2 * pitchArea.getWidth() / 3).reduced(1));
    pitchSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, pitchSlider.getWidth(), pitchSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto pitchQuantisationArea = area.removeFromTop(hUnit);
    pitchQuantisationChoice.setBounds(pitchQuantisationArea.removeFromRight(2 * pitchQuantisationArea.getWidth() / 3).reduced(2));

    auto playbackPosStartArea = area.removeFromTop(hUnit);
    playbackPositionStartSlider.setBounds(playbackPosStartArea.removeFromRight(2 * playbackPosStartArea.getWidth() / 3).reduced(1));
    playbackPositionStartSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, playbackPositionStartSlider.getWidth(), playbackPositionStartSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto playbackPosIntervalArea = area.removeFromTop(hUnit);
    playbackPositionIntervalSlider.setBounds(playbackPosIntervalArea.removeFromRight(2 * playbackPosIntervalArea.getWidth() / 3).reduced(1));
    playbackPositionIntervalSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, playbackPositionStartSlider.getWidth(), playbackPositionStartSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto midiArea = area.removeFromTop(hUnit);
    midiChannelChoice.setBounds(midiArea.removeFromRight(2 * midiArea.getWidth() / 3).reduced(2));
    midiArea = area.removeFromTop(hUnit);
    midiNoteChoice.setBounds(midiArea.removeFromRight(2 * midiArea.getWidth() / 3).reduced(2));

    randomiseButton.setBounds(area.removeFromBottom(hUnit).reduced(1));

    lfoEditor.setUnitOfHeight(hUnit);
    lfoEditor.setBounds(area.reduced(1)); //fill rest with lfoEditor
}

bool RegionEditor::keyPressed(const juce::KeyPress& key/*, Component* originatingComponent*/)
{
    if (key == juce::KeyPress::createFromDescription("ctrl + r"))
    {
        auto mousePos = getMouseXYRelative();
        juce::Component* target = getComponentAt(mousePos.getX(), mousePos.getY());

        if (focusPositionX.getBounds().contains(mousePos) || focusPositionY.getBounds().contains(mousePos))
        {
            randomiseFocusPosition();
            return true;
        }
        else if (dahdsrEditor.getBounds().contains(mousePos))
        {
            return dahdsrEditor.keyPressed(key);
        }
        else if (volumeSlider.getBounds().contains(mousePos))
        {
            randomiseVolume();
            return true;
        }
        else if (pitchSlider.getBounds().contains(mousePos))
        {
            randomisePitch();
            return true;
        }
        else if (pitchQuantisationChoice.getBounds().contains(mousePos))
        {
            randomisePitchQuantisation();
            return true;
        }
        else if (playbackPositionStartSlider.getBounds().contains(mousePos))
        {
            randomisePlaybackPositionStart();
            return true;
        }
        else if (playbackPositionIntervalSlider.getBounds().contains(mousePos))
        {
            randomisePlaybackPositionInterval();
            return true;
        }
        else if (lfoEditor.getBounds().contains(mousePos))
        {
            return lfoEditor.keyPressed(key);
        }
    }

    return false;
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

    playbackPositionStartLabel.setVisible(shouldBeVisible);
    playbackPositionStartSlider.setVisible(shouldBeVisible);

    playbackPositionIntervalLabel.setVisible(shouldBeVisible);
    playbackPositionIntervalSlider.setVisible(shouldBeVisible);

    midiChannelChoice.setVisible(shouldBeVisible);
    midiChannelLabel.setVisible(shouldBeVisible);
    midiNoteChoice.setVisible(shouldBeVisible);
    midiNoteLabel.setVisible(shouldBeVisible);

    lfoEditor.setVisible(shouldBeVisible);

    randomiseButton.setVisible(shouldBeVisible);
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
        pitchQuantisationChoice.setSelectedId(static_cast<int>(voice->getPitchQuantisationMethod()) + 1, juce::NotificationType::dontSendNotification);

        playbackPositionStartSlider.setValue(voice->getBasePlaybackPositionStart(), juce::NotificationType::dontSendNotification);
        playbackPositionIntervalSlider.setValue(voice->getBasePlaybackPositionInterval(), juce::NotificationType::dontSendNotification);
    }
    else
    {
        pitchQuantisationChoice.setEnabled(false);
    }

    int midiChannel = associatedRegion->getMidiChannel();
    if (midiChannel < 0)
    {
        midiChannelChoice.setSelectedId(-1, juce::NotificationType::dontSendNotification);
    }
    else
    {
        midiChannelChoice.setSelectedId(midiChannel + 1, juce::NotificationType::dontSendNotification);
    }
    midiNoteChoice.setSelectedId(associatedRegion->getMidiNote(), juce::NotificationType::dontSendNotification);

    lfoEditor.updateAvailableVoices();
    lfoEditor.copyParameters();

    lfoDepth.setText("LFO depth: " + juce::String(associatedRegion->getAssociatedLfo()->getDepth()), juce::NotificationType::dontSendNotification);

    DBG("region parameters have been copied.");
}

void RegionEditor::selectFile()
{
    //shutdownAudio();

    fc = std::make_unique<juce::FileChooser>("Select a Wave file shorter than 2 seconds to play...",
        juce::File(/*R"(C:\Users\Aaron\Desktop\Musikproduktion\VSTs\Iris 2\Iris 2 Library\Samples)"*/),
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
void RegionEditor::randomiseFocusPosition()
{
    juce::Random& rng = juce::Random::getSystemRandom();
    
    //some completely random position is fine
    focusPositionX.setValue(rng.nextDouble(), juce::NotificationType::dontSendNotification);
    focusPositionY.setValue(rng.nextDouble(), juce::NotificationType::sendNotification);
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
    updatePlaybackPositionStart();
    updatePlaybackPositionInterval();
}

void RegionEditor::updateVolume()
{
    juce::Array<Voice*> voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        (*it)->setBaseLevel(juce::Decibels::decibelsToGain<double>(volumeSlider.getValue(), -60.0));
    }
}
void RegionEditor::randomiseVolume()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //prefer values closer to 0 (by dividing the result randomly by a value within [1, 6] -> average value of -7,7dB)
    volumeSlider.setValue((volumeSlider.getMinimum() + rng.nextDouble() * (volumeSlider.getMaximum() - volumeSlider.getMinimum())) / (1.0 + 5 * rng.nextDouble()), juce::NotificationType::sendNotification);
}

void RegionEditor::updatePitch()
{
    juce::Array<Voice*> voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        (*it)->setBasePitchShift(pitchSlider.getValue());
    }
}
void RegionEditor::randomisePitch()
{
    juce::Random& rng = juce::Random::getSystemRandom();
    
    //prefer value closer to 0 (see volume, same divisor). values are rounded to the closest integer since that's usually cleaner
    pitchSlider.setValue(std::round((pitchSlider.getMinimum() + rng.nextDouble() * (pitchSlider.getMaximum() - pitchSlider.getMinimum())) / (1.0 + 5 * rng.nextDouble())), juce::NotificationType::sendNotification);
}
void RegionEditor::randomisePitchQuantisation()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //50% chance to be continuous. otherwise, choose a random entry
    if (rng.nextFloat() < 0.5f)
    {
        pitchQuantisationChoice.setSelectedItemIndex(0, juce::NotificationType::sendNotification);
    }
    else
    {
        pitchQuantisationChoice.setSelectedItemIndex(rng.nextInt(juce::Range<int>(1, pitchQuantisationChoice.getNumItems())), juce::NotificationType::sendNotification);
    }
}

void RegionEditor::updatePlaybackPositionStart()
{
    juce::Array<Voice*> voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        (*it)->setBasePlaybackPositionStart(playbackPositionStartSlider.getValue());
    }
}
void RegionEditor::randomisePlaybackPositionStart()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //prefer values closer to 0
    if (rng.nextInt(5) < 4)
    {
        //80% chance: remain 0
        playbackPositionStartSlider.setValue(0.0, juce::NotificationType::sendNotification);
    }
    else
    {
        //20% chance: randomise freely
        playbackPositionStartSlider.setValue((playbackPositionStartSlider.getMinimum() + rng.nextDouble() * (playbackPositionStartSlider.getMaximum() - playbackPositionStartSlider.getMinimum())), juce::NotificationType::sendNotification);
    }
}

void RegionEditor::updatePlaybackPositionInterval()
{
    juce::Array<Voice*> voices = associatedRegion->getAudioEngine()->getVoicesWithID(associatedRegion->getID());

    for (auto* it = voices.begin(); it != voices.end(); it++)
    {
        (*it)->setBasePlaybackPositionInterval(playbackPositionIntervalSlider.getValue());
    }
}
void RegionEditor::randomisePlaybackPositionInterval()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //prefer values closer to 0
    if (rng.nextInt(4) < 3)
    {
        //75% chance: remain 1
        playbackPositionIntervalSlider.setValue(1.0, juce::NotificationType::sendNotification);
    }
    else
    {
        //25% chance: randomise freely
        playbackPositionIntervalSlider.setValue((playbackPositionIntervalSlider.getMinimum() + rng.nextDouble() * (playbackPositionIntervalSlider.getMaximum() - playbackPositionIntervalSlider.getMinimum())), juce::NotificationType::sendNotification);
    }
}

void RegionEditor::randomiseAllParameters()
{
    randomiseFocusPosition();
    
    //don't randomise toggle mode - it's more of a coordination tool imo

    //DAHDSR parameters
    dahdsrEditor.randomiseAllParameters();

    randomiseVolume();
    randomisePitch();
    randomisePitchQuantisation();

    randomisePlaybackPositionStart();
    randomisePlaybackPositionInterval();

    //MIDI channel/note: no randomisation.

    //LFO editor
    lfoEditor.randomiseAllParameters();
}
