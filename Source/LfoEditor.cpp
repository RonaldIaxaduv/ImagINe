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

    lfoRateSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    lfoRateSlider.setTextValueSuffix("Hz");
    lfoRateSlider.setRange(0.01, 100.0, 0.01);
    lfoRateSlider.setSkewFactorFromMidPoint(1.0);
    lfoRateSlider.onValueChange = [this] { updateLfoRate(); };
    addAndMakeVisible(lfoRateSlider);

    lfoRateLabel.setText("LFO rate: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoRateLabel);
    lfoRateLabel.attachToComponent(&lfoRateSlider, true);

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

    /*auto lfoParamArea = area.removeFromTop(20);
    lfoParameterLabel.setBounds(lfoParamArea.removeFromLeft(lfoParamArea.getWidth() / 3));
    lfoParameterChoice.setBounds(lfoParamArea);*/

    lfoRegionsList.setBounds(area);
}

void LfoEditor::copyParameters()
{
    //copy rate
    lfoRateSlider.setValue(associatedLfo->getBaseFrequency(), juce::NotificationType::dontSendNotification);

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

void LfoEditor::updateLfoParameter(int targetRegionID, bool shouldBeModulated, LfoModulatableParameter modulatedParameter)
{
    if (!shouldBeModulated || static_cast<int>(modulatedParameter) <= 0)
    {
        associatedLfo->removeRegionModulation(targetRegionID); //removing modulation is the same for every region
        return;
    }
    
    //the remaining code is only for adding modulations

    std::function<void(float, float, float, Voice* v)> func;
    switch (modulatedParameter)
    {
    case LfoModulatableParameter::volume:
        func = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
        {
            v->modulateLevel(static_cast<double>(lfoValueUnipolar * depth));
        };
        break;
    case LfoModulatableParameter::volume_inverted:
        func = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
        {
            v->modulateLevel(static_cast<double>((1 - lfoValueUnipolar) * depth));
        };
        break;


    case LfoModulatableParameter::pitch:
        func = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
        {
            v->modulatePitchShift(static_cast<double>(lfoValueBipolar * depth) * 12.0);
        };
        break;
    case LfoModulatableParameter::pitch_inverted:
        func = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
        {
            v->modulatePitchShift(static_cast<double>(-lfoValueBipolar * depth) * 12.0);
        };
        break;


    case LfoModulatableParameter::lfoRate:
        func = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
        {
            v->getLfoFreqModFunction()(static_cast<double>(lfoValueBipolar * depth) * 24.0);
        };
        break;
    case LfoModulatableParameter::lfoRate_inverted:
        func = [](float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)
        {
            v->getLfoFreqModFunction()(static_cast<double>(-lfoValueBipolar * depth) * 24.0);
        };
        break;


    case LfoModulatableParameter::playbackPosition:
        //WIP
        break;
    case LfoModulatableParameter::playbackPosition_inverted:
        //WIP
        break;


    default:
        throw std::exception("invalid modulated parameter ID");
    }

    associatedLfo->addRegionModulation(audioEngine->getVoicesWithID(targetRegionID), func, modulatedParameter); //also overwrites existing modulation if this region had already been modulated
}

//void LfoEditor::updateLfoVoices(juce::Array<int> voiceIndices)
//{
//    juce::Array<Voice*> voices;
//    for (auto it = voiceIndices.begin(); it != voiceIndices.end(); ++it)
//    {
//        voices.addArray(audioEngine->getVoicesWithID(*it));
//    }
//    associatedLfo->setVoices(voices);
//
//    DBG("voices updated.");
//}
