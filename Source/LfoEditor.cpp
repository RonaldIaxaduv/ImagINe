/*
  ==============================================================================

    LfoEditor.cpp
    Created: 8 Jul 2022 10:00:12am
    Author:  Aaron

  ==============================================================================
*/

#include "LfoEditor.h"


//public

LfoEditor::LfoEditor(AudioEngine* audioEngine, RegionLfo* associatedLfo /*SegmentedRegion* associatedRegion*/)
{
    //this->associatedRegion = associatedRegion;
    this->audioEngine = audioEngine;
    this->associatedLfo = associatedLfo;

    lfoRateSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    lfoRateSlider.setTextValueSuffix("Hz");
    lfoRateSlider.setRange(0.01, 100.0, 0.01);
    lfoRateSlider.setSkewFactorFromMidPoint(1.0);
    lfoRateSlider.onValueChange = [this] { updateLfoRate(); };
    addAndMakeVisible(lfoRateSlider);

    lfoRateLabel.setText("LFO rate: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoRateLabel);
    lfoRateLabel.attachToComponent(&lfoRateSlider, true);

    lfoParameterChoice.addSectionHeading("Basic");
    lfoParameterChoice.addItem("Volume", static_cast<int>(LfoModulatableParameter::volume));
    lfoParameterChoice.addItem("Pitch", static_cast<int>(LfoModulatableParameter::pitch));
    //lfoParameterChoice.addItem("Panning", static_cast<int>(LfoModulatableParameter::panning));
    lfoParameterChoice.addSeparator();
    lfoParameterChoice.addSectionHeading("LFO");
    lfoParameterChoice.addItem("LFO Rate", static_cast<int>(LfoModulatableParameter::lfoRate));
    lfoParameterChoice.addSeparator();
    lfoParameterChoice.addSectionHeading("Experimental");
    lfoParameterChoice.addItem("Playback Position", static_cast<int>(LfoModulatableParameter::playbackPosition));
    lfoParameterChoice.onChange = [this] { updateLfoParameter(); };
    addAndMakeVisible(lfoParameterChoice);

    lfoParameterLabel.setText("Modulated Parameter: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoParameterLabel);
    lfoParameterLabel.attachToComponent(&lfoParameterChoice, true);

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

    auto lfoParamArea = area.removeFromTop(20);
    lfoParameterLabel.setBounds(lfoParamArea.removeFromLeft(lfoParamArea.getWidth() / 3));
    lfoParameterChoice.setBounds(lfoParamArea);

    lfoRegionsList.setBounds(area);
}

void LfoEditor::copyParameters()
{
    //auto lfo = associatedRegion->getAssociatedLfo();

    //copy rate
    lfoRateSlider.setValue(associatedLfo->getBaseFrequency(), juce::NotificationType::dontSendNotification);

    //copy modulated parameter
    if (static_cast<int>(associatedLfo->getModulatedParameterID()) > 0) //parameter IDs start with 1 (0 isn't included because of how selected item IDs work with ComboBoxes in juce)
    {
        lfoParameterChoice.setSelectedId(static_cast<int>(associatedLfo->getModulatedParameterID()), juce::NotificationType::dontSendNotification);
    }

    //copy currently affected voices
    lfoRegionsList.setCheckedRegions(associatedLfo->getRegionIDs());
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
        auto newRowNumber = lfoRegionsList.addItem("Region " + juce::String(*it), *it);
        lfoRegionsList.setRowBackgroundColour(newRowNumber, audioEngine->getRegionColour(*it));
        lfoRegionsList.setClickFunction(newRowNumber, [this] { updateLfoVoices(lfoRegionsList.getCheckedRegionIDs()); });
    }
}




//private

void LfoEditor::updateLfoRate()
{
    associatedLfo->setBaseFrequency(lfoRateSlider.getValue());
}

void LfoEditor::updateLfoParameter()
{
    //note: the braces for the case statements are required here to allow for variable initialisations. see https://stackoverflow.com/questions/5136295/switch-transfer-of-control-bypasses-initialization-of-when-calling-a-function
    switch (static_cast<LfoModulatableParameter>(lfoParameterChoice.getSelectedId()))
    {
    case LfoModulatableParameter::volume:
    {
        associatedLfo->setPolarity(RegionLfo::Polarity::unipolar);
        std::function<void(float, Voice*)> func = [](float lfoValue, Voice* v) { v->modulateLevel(static_cast<double>(lfoValue)); };
        associatedLfo->setModulationFunction(func);
        associatedLfo->setModulatedParameterID(LfoModulatableParameter::volume);
        break;
    }

    case LfoModulatableParameter::pitch:
    {
        associatedLfo->setPolarity(RegionLfo::Polarity::bipolar);
        std::function<void(float, Voice*)> func = [](float lfoValue, Voice* v) { v->modulatePitchShift(static_cast<double>(lfoValue) * 12.0); };
        associatedLfo->setModulationFunction(func);
        associatedLfo->setModulatedParameterID(LfoModulatableParameter::pitch);
        break;
    }

    case LfoModulatableParameter::lfoRate:
    {
        associatedLfo->setPolarity(RegionLfo::Polarity::bipolar);
        std::function<void(float, Voice*)> func = [](float lfoValue, Voice* v) { v->getLfoFreqModFunction()(lfoValue * 24.0f); };
        associatedLfo->setModulationFunction(func);
        associatedLfo->setModulatedParameterID(LfoModulatableParameter::lfoRate);
        break;
    }

    case LfoModulatableParameter::playbackPosition:
    {
        associatedLfo->setPolarity(RegionLfo::Polarity::unipolar);
        //WIP
        break;
    }

    default:
        break;
    }
}

void LfoEditor::updateLfoVoices(juce::Array<int> voiceIndices)
{
    juce::Array<Voice*> voices;
    for (auto it = voiceIndices.begin(); it != voiceIndices.end(); ++it)
    {
        voices.addArray(audioEngine->getVoicesWithID(*it));
    }
    associatedLfo->setVoices(voices);

    DBG("voices updated.");
}
