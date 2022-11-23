/*
  ==============================================================================

    PlayPathEditor.cpp
    Created: 7 Nov 2022 9:06:47am
    Author:  Aaron

  ==============================================================================
*/

#include "PlayPathEditor.h"

PlayPathEditor::PlayPathEditor(PlayPath* path) :
    upColourSelector(juce::ColourSelector::ColourSelectorOptions::editableColour | juce::ColourSelector::ColourSelectorOptions::showColourAtTop | juce::ColourSelector::ColourSelectorOptions::showColourspace),
    downColourSelector(juce::ColourSelector::ColourSelectorOptions::editableColour | juce::ColourSelector::ColourSelectorOptions::showColourAtTop | juce::ColourSelector::ColourSelectorOptions::showColourspace)
{
    associatedPath = path;

    //interval
    courierIntervalSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    courierIntervalSlider.setTextValueSuffix("s");
    courierIntervalSlider.setRange(0.1, 600.0, 0.1);
    courierIntervalSlider.setSkewFactorFromMidPoint(60.0);
    courierIntervalSlider.onValueChange = [this] { updateCourierInterval(); };
    courierIntervalSlider.setValue(10.0, juce::NotificationType::dontSendNotification);
    courierIntervalSlider.setPopupMenuEnabled(true);
    courierIntervalSlider.setTooltip("This slider changes the time that it takes the courier (the point traveling around the play path) to complete one lap around the play path.");
    addAndMakeVisible(courierIntervalSlider);

    courierIntervalLabel.setText("Courier Interval:", juce::NotificationType::dontSendNotification);
    courierIntervalLabel.attachToComponent(&courierIntervalSlider, true);
    addAndMakeVisible(courierIntervalLabel);

    //up colour selection
    upColourSelector.addChangeListener(this);
    addAndMakeVisible(upColourSelector);

    upColourLabel.setText("OFF Colour:", juce::NotificationType::dontSendNotification);
    upColourLabel.setTooltip("The colour selector on the right lets you choose the colour of the play path when it's inactive.");
    upColourLabel.attachToComponent(&upColourSelector, true);
    addAndMakeVisible(upColourLabel);

    //down colour selection
    downColourSelector.addChangeListener(this);
    addAndMakeVisible(downColourSelector);

    downColourLabel.setText("ON Colour:", juce::NotificationType::dontSendNotification);
    downColourLabel.setTooltip("The colour selector on the right lets you choose the colour of the play path when it's active.");
    downColourLabel.attachToComponent(&downColourSelector, true);
    addAndMakeVisible(downColourLabel);

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
            associatedPath->setMidiChannel(-1);
        }
        else if (midiChannelChoice.getSelectedId() > 0)
        {
            associatedPath->setMidiChannel(midiChannelChoice.getSelectedId() - 1);
        }
    };
    midiChannelChoice.setTooltip("If you want to play this play path via MIDI notes, you can select the channel that it should listen to here. Tip: there can be several play paths responding to the same channel and note!");
    addAndMakeVisible(midiChannelChoice);

    midiChannelLabel.setText("MIDI Channel: ", juce::NotificationType::dontSendNotification);
    midiChannelLabel.attachToComponent(&midiChannelChoice, true);
    addAndMakeVisible(midiChannelLabel);

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
        associatedPath->setMidiNote(midiNoteChoice.getSelectedId());
    };
    midiNoteChoice.setTooltip("If you want to play this play path via MIDI notes, you can select the note that it should listen to here. Tip: there can be several play paths responding to the same channel and note!");
    addAndMakeVisible(midiNoteChoice);

    midiNoteLabel.setText("MIDI Note: ", juce::NotificationType::dontSendNotification);
    midiNoteLabel.attachToComponent(&midiNoteChoice, true);
    addAndMakeVisible(midiNoteLabel);

    //randomise button
    randomiseButton.setButtonText("Randomise Region Parameters");
    randomiseButton.onClick = [this] { randomiseAllParameters(); };
    randomiseButton.setTooltip("Clicking this button will randomise most of the parameters in this editor. The randomness is biased in a useful way to achieve useful results. You can also press Ctrl + r while hovering above a parameter to randomise only that one value.");
    addAndMakeVisible(randomiseButton);

    //rest
    copyPathParameters();

    setSize(350, 700);
}
PlayPathEditor::~PlayPathEditor()
{
    DBG("destroying PlayPathEditor...");

    associatedPath = nullptr;

    DBG("PlayPathEditor destroyed.");
}

void PlayPathEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour(juce::Colours::lightgrey);
    g.drawRect(getLocalBounds(), 1);   // draw an outline around the component
}
void PlayPathEditor::resized()
{
    auto area = getLocalBounds();
    int hUnit = juce::jmin(50, juce::jmax(5, static_cast<int>(static_cast<float>(getHeight()) / 11.0))); //unit of height required to squeeze all elements into the window's area

    auto courierIntervalArea = area.removeFromTop(hUnit);
    courierIntervalSlider.setBounds(courierIntervalArea.removeFromRight(2 * courierIntervalArea.getWidth() / 3).reduced(1));
    courierIntervalSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, courierIntervalSlider.getWidth(), courierIntervalSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto upColourArea = area.removeFromTop(3 * hUnit);
    upColourSelector.setBounds(upColourArea.removeFromRight(2 * upColourArea.getWidth() / 3).reduced(1));

    auto downColourArea = area.removeFromTop(3 * hUnit);
    downColourSelector.setBounds(downColourArea.removeFromRight(2 * downColourArea.getWidth() / 3).reduced(1));

    auto midiArea = area.removeFromTop(hUnit);
    midiChannelChoice.setBounds(midiArea.removeFromRight(2 * midiArea.getWidth() / 3).reduced(2));
    midiArea = area.removeFromTop(hUnit);
    midiNoteChoice.setBounds(midiArea.removeFromRight(2 * midiArea.getWidth() / 3).reduced(2));

    area.removeFromTop(hUnit);

    randomiseButton.setBounds(area.removeFromBottom(hUnit).reduced(1));
}

bool PlayPathEditor::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::createFromDescription("ctrl + r"))
    {
        auto mousePos = getMouseXYRelative();
        juce::Component* target = getComponentAt(mousePos.getX(), mousePos.getY());

        if (courierIntervalSlider.getBounds().contains(mousePos))
        {
            randomiseCourierInterval();
            return true;
        }
        else if (upColourSelector.getBounds().contains(mousePos))
        {
            randomiseUpColour();
            return true;
        }
        else if (downColourSelector.getBounds().contains(mousePos))
        {
            randomiseDownColour();
            return true;
        }
    }

    return false;
}

void PlayPathEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (dynamic_cast<juce::ColourSelector*>(source) == &upColourSelector)
    {
        updateUpColour();
    }
    else if (dynamic_cast<juce::ColourSelector*>(source) == &downColourSelector)
    {
        updateDownColour();
    }
}



void PlayPathEditor::copyPathParameters()
{
    DBG("copying play path parameters...");

    courierIntervalSlider.setValue(associatedPath->getCourierInterval_seconds(), juce::NotificationType::dontSendNotification);

    upColourSelector.setCurrentColour(associatedPath->getFillColour(), juce::NotificationType::dontSendNotification);
    downColourSelector.setCurrentColour(associatedPath->getFillColourOn(), juce::NotificationType::dontSendNotification);

    int midiChannel = associatedPath->getMidiChannel();
    if (midiChannel < 0)
    {
        midiChannelChoice.setSelectedId(-1, juce::NotificationType::dontSendNotification);
    }
    else
    {
        midiChannelChoice.setSelectedId(midiChannel + 1, juce::NotificationType::dontSendNotification);
    }
    midiNoteChoice.setSelectedId(associatedPath->getMidiNote(), juce::NotificationType::dontSendNotification);

    DBG("play path parameters have been copied.");
}

void PlayPathEditor::updateCourierInterval()
{
    associatedPath->setCourierInterval_seconds(static_cast<float>(courierIntervalSlider.getValue()));
}
void PlayPathEditor::randomiseCourierInterval()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //some completely random value within the range is fine
    courierIntervalSlider.setValue(courierIntervalSlider.getMinimum() + rng.nextDouble() * (courierIntervalSlider.getMaximum() - courierIntervalSlider.getMinimum()), juce::NotificationType::sendNotification);
}

void PlayPathEditor::updateUpColour()
{
    associatedPath->setFillColour(upColourSelector.getCurrentColour());
}
void PlayPathEditor::randomiseUpColour()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    upColourSelector.setCurrentColour(juce::Colour(rng.nextInt(256), rng.nextInt(256), rng.nextInt(256)), juce::NotificationType::dontSendNotification);
}

void PlayPathEditor::updateDownColour()
{
    associatedPath->setFillColourOn(downColourSelector.getCurrentColour());
}
void PlayPathEditor::randomiseDownColour()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    downColourSelector.setCurrentColour(juce::Colour(rng.nextInt(256), rng.nextInt(256), rng.nextInt(256)), juce::NotificationType::dontSendNotification);
}

void PlayPathEditor::randomiseAllParameters()
{
    randomiseCourierInterval();

    //don't randomise the colours - they are more of a tool after all (and they can still be randomised manually by ctrl+r)
}
