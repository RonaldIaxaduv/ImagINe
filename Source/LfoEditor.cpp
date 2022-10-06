/*
  ==============================================================================

    LfoEditor.cpp
    Created: 8 Jul 2022 10:00:12am
    Author:  Aaron

  ==============================================================================
*/

#include "LfoEditor.h"


//public

LfoEditor::LfoEditor(AudioEngine* audioEngine, RegionLfo* associatedLfo)
{
    this->audioEngine = audioEngine;
    this->associatedLfo = associatedLfo;

    //rate
    lfoRateSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    lfoRateSlider.setTextValueSuffix("Hz");
    lfoRateSlider.setRange(0.01, 100.0, 0.01);
    lfoRateSlider.setSkewFactorFromMidPoint(1.0);
    lfoRateSlider.onValueChange = [this] { updateLfoRate(); };
    addAndMakeVisible(lfoRateSlider);

    lfoRateLabel.setText("LFO rate: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoRateLabel);
    lfoRateLabel.attachToComponent(&lfoRateSlider, true);

    //update interval
    lfoUpdateIntervalSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    lfoUpdateIntervalSlider.setTextValueSuffix("ms");
    lfoUpdateIntervalSlider.setRange(0.0, 10000.0, 0.01);
    lfoUpdateIntervalSlider.setSkewFactorFromMidPoint(100.0);
    lfoUpdateIntervalSlider.onValueChange = [this] { updateLfoUpdateInterval(); };
    addAndMakeVisible(lfoUpdateIntervalSlider);

    lfoUpdateIntervalLabel.setText("Update Interval: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoUpdateIntervalLabel);
    lfoUpdateIntervalLabel.attachToComponent(&lfoUpdateIntervalSlider, true);

    //lfoParameterChoice.addSectionHeading("Basic");
    //lfoParameterChoice.addItem("Volume", static_cast<int>(LfoModulatableParameter::volume));
    //lfoParameterChoice.addItem("Pitch", static_cast<int>(LfoModulatableParameter::pitch));
    ////lfoParameterChoice.addItem("Panning", static_cast<int>(LfoModulatableParameter::panning));
    //lfoParameterChoice.addSeparator();
    //lfoParameterChoice.addSectionHeading("LFO");
    //lfoParameterChoice.addItem("LFO Rate", static_cast<int>(LfoModulatableParameter::lfoRate));
    //lfoParameterChoice.addSeparator();
    //lfoParameterChoice.addSectionHeading("Experimental");
    //lfoParameterChoice.addItem("Playback Position", static_cast<int>(LfoModulatableParameter::playbackPosition));
    //lfoParameterChoice.onChange = [this] { updateLfoParameter(); };
    //addAndMakeVisible(lfoParameterChoice);

    //lfoParameterLabel.setText("Modulated Parameter: ", juce::NotificationType::dontSendNotification);
    //addAndMakeVisible(lfoParameterLabel);
    //lfoParameterLabel.attachToComponent(&lfoParameterChoice, true);

    updateAvailableVoices();
    addAndMakeVisible(lfoRegionsList);
}

LfoEditor::~LfoEditor()
{
    audioEngine = nullptr;
    associatedLfo = nullptr;
}

void LfoEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));   // clear the background
}

void LfoEditor::resized()
{
    auto area = getLocalBounds();

    auto lfoRateArea = area.removeFromTop(20);
    lfoRateLabel.setBounds(lfoRateArea.removeFromLeft(lfoRateArea.getWidth() / 3));
    lfoRateSlider.setBounds(lfoRateArea);

    auto lfoUpdateIntervalArea = area.removeFromTop(20);
    lfoUpdateIntervalLabel.setBounds(lfoUpdateIntervalArea.removeFromLeft(lfoUpdateIntervalArea.getWidth() / 3));
    lfoUpdateIntervalSlider.setBounds(lfoUpdateIntervalArea);

    /*auto lfoParamArea = area.removeFromTop(20);
    lfoParameterLabel.setBounds(lfoParamArea.removeFromLeft(lfoParamArea.getWidth() / 3));
    lfoParameterChoice.setBounds(lfoParamArea);*/

    lfoRegionsList.setBounds(area);
}

void LfoEditor::copyParameters()
{
    //copy rate
    lfoRateSlider.setValue(associatedLfo->getBaseFrequency(), juce::NotificationType::dontSendNotification);

    lfoUpdateIntervalSlider.setValue(associatedLfo->getUpdateInterval_Milliseconds(), juce::NotificationType::dontSendNotification);

    //copy modulated parameters
    //if (static_cast<int>(associatedLfo->getModulatedParameterID()) > 0) //parameter IDs start with 1 (0 isn't included because of how selected item IDs work with ComboBoxes in juce)
    //{
    //    lfoParameterChoice.setSelectedId(static_cast<int>(associatedLfo->getModulatedParameterID()), juce::NotificationType::dontSendNotification);
    //}

    //copy currently affected voices and their modulated parameters
    lfoRegionsList.copyRegionModulations(associatedLfo->getAffectedRegionIDs(), associatedLfo->getModulatedParameterIDs());
}

void LfoEditor::updateAvailableVoices()
{
    lfoRegionsList.clearItems();
    juce::Array<int> regionIdList;

    for (int i = 0; i <= audioEngine->getLastRegionID(); ++i)
    {
        if (audioEngine->checkRegionHasVoice(i) == true)
        {
            regionIdList.add(i);
        }
    }

    for (auto it = regionIdList.begin(); it != regionIdList.end(); ++it)
    {
        auto newRowNumber = lfoRegionsList.addItem("Region " + juce::String(*it), *it, this); //automatically adds the LfoEditor as a ChangeListener for the row
        lfoRegionsList.setRowBackgroundColour(newRowNumber, audioEngine->getRegionColour(*it));
        //lfoRegionsList.setClickFunction(newRowNumber, [this] { updateLfoVoices(lfoRegionsList.getCheckedRegionIDs()); });
    }
}

void LfoEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    auto* cbListItem = dynamic_cast<CheckBoxListItem*>(source);
    updateLfoParameter(cbListItem->getRegionID(), cbListItem->isModulated(), cbListItem->getModulatedParameter());
}




//private

void LfoEditor::updateLfoRate()
{
    associatedLfo->setBaseFrequency(lfoRateSlider.getValue());
}
void LfoEditor::updateLfoUpdateInterval()
{
    associatedLfo->setUpdateInterval_Milliseconds(static_cast<float>(lfoUpdateIntervalSlider.getValue()));
}

void LfoEditor::updateLfoParameter(int targetRegionID, bool shouldBeModulated, LfoModulatableParameter modulatedParameter)
{
    if (!shouldBeModulated || static_cast<int>(modulatedParameter) <= 0)
    {
        associatedLfo->removeRegionModulation(targetRegionID); //removing modulation is the same for every region
        return;
    }
    
    //there are different overloads for RegionLfo::addRegionModulation depending on whether a voice is modulated or an LFO
    switch (modulatedParameter)
    {
    case LfoModulatableParameter::volume:
    case LfoModulatableParameter::volume_inverted:
        associatedLfo->addRegionModulation(modulatedParameter, targetRegionID, audioEngine->getParameterOfRegion_Volume(targetRegionID));
        break;

    case LfoModulatableParameter::pitch:
    case LfoModulatableParameter::pitch_inverted:
        associatedLfo->addRegionModulation(modulatedParameter, targetRegionID, audioEngine->getParameterOfRegion_Pitch(targetRegionID));
        break;

    case LfoModulatableParameter::playbackPosition:
    case LfoModulatableParameter::playbackPosition_inverted:
        associatedLfo->addRegionModulation(modulatedParameter, targetRegionID, audioEngine->getParameterOfRegion_PlaybackPosition(targetRegionID));
        break;




    case LfoModulatableParameter::lfoRate:
    case LfoModulatableParameter::lfoRate_inverted:
        associatedLfo->addRegionModulation(modulatedParameter, targetRegionID, audioEngine->getParameterOfRegion_LfoRate(targetRegionID));
        break;

    case LfoModulatableParameter::lfoPhase:
    case LfoModulatableParameter::lfoPhase_inverted:
        associatedLfo->addRegionModulation(modulatedParameter, targetRegionID, audioEngine->getParameterOfRegion_LfoPhase(targetRegionID));
        break;

    case LfoModulatableParameter::lfoUpdateInterval:
    case LfoModulatableParameter::lfoUpdateInterval_inverted:
        associatedLfo->addRegionModulation(modulatedParameter, targetRegionID, audioEngine->getParameterOfRegion_LfoUpdateInterval(targetRegionID));
        break;




    default:
        throw std::exception("Unknown or unimplemented region modulation");
    }
}
