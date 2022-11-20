/*
  ==============================================================================

    LfoEditor.cpp
    Created: 8 Jul 2022 10:00:12am
    Author:  Aaron

  ==============================================================================
*/

#include "LfoEditor.h"

#include "RegionEditor.h"


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
    lfoRateSlider.setPopupMenuEnabled(true);
    lfoRateSlider.setTooltip("This slider changes the speed at which the LFO line travels around the region.");
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
    lfoUpdateIntervalSlider.setPopupMenuEnabled(true);
    lfoUpdateIntervalSlider.setTooltip("This value determines how often the value of the LFO is evaluated. It's also reflected in how often the LFO line is updated. Slower update rates can be useful for some modulated parameters, e.g. pitch or LFO phase. They also reduce the CPU load.");
    addAndMakeVisible(lfoUpdateIntervalSlider);

    lfoUpdateIntervalLabel.setText("Update Interval: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoUpdateIntervalLabel);
    lfoUpdateIntervalLabel.attachToComponent(&lfoUpdateIntervalSlider, true);

    //modulation list
    updateAvailableVoices();
    addAndMakeVisible(lfoRegionsList);
}

LfoEditor::~LfoEditor()
{
    DBG("destroying LfoEditor...");

    audioEngine = nullptr;
    associatedLfo = nullptr;

    DBG("LfoEditor destroyed.");
}

void LfoEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));   // clear the background
}

void LfoEditor::resized()
{
    auto area = getLocalBounds();

    auto lfoRateArea = area.removeFromTop(20);
    lfoRateSlider.setBounds(lfoRateArea.removeFromRight(2 * lfoRateArea.getWidth() / 3).reduced(1));
    lfoRateSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, lfoRateSlider.getWidth(), lfoRateSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto lfoUpdateIntervalArea = area.removeFromTop(20);
    lfoUpdateIntervalSlider.setBounds(lfoUpdateIntervalArea.removeFromRight(2 * lfoUpdateIntervalArea.getWidth() / 3).reduced(1));
    lfoUpdateIntervalSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, lfoUpdateIntervalSlider.getWidth(), lfoUpdateIntervalSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    lfoRegionsList.setBounds(area.reduced(1));
}

bool LfoEditor::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::createFromDescription("ctrl + r"))
    {
        auto mousePos = getMouseXYRelative();
        juce::Component* target = getComponentAt(mousePos.getX(), mousePos.getY());

        if (lfoRateSlider.getBounds().contains(mousePos))
        {
            randomiseLfoRate();
            return true;
        }
        else if (lfoUpdateIntervalSlider.getBounds().contains(mousePos))
        {
            randomiseLfoUpdateInterval();
            return true;
        }
        else if (lfoRegionsList.getBounds().contains(mousePos))
        {
            return lfoRegionsList.keyPressed(key);
        }
    }
    
    return false;
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

void LfoEditor::randomiseAllParameters()
{
    randomiseLfoRate();
    randomiseLfoUpdateInterval();
    lfoRegionsList.randomiseAllParameters();
}




//private

void LfoEditor::updateLfoRate()
{
    associatedLfo->setBaseFrequency(lfoRateSlider.getValue());
}
void LfoEditor::randomiseLfoRate()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //prefer values closer to 0 (by dividing the result randomly by a value within [1, 0.5*max])
    lfoRateSlider.setValue((lfoRateSlider.getMinimum() + rng.nextDouble() * (lfoRateSlider.getMaximum() - lfoRateSlider.getMinimum())) / (1.0 + rng.nextDouble() * 0.5 * lfoRateSlider.getMaximum()), juce::NotificationType::sendNotification);
}

void LfoEditor::updateLfoUpdateInterval()
{
    associatedLfo->setUpdateInterval_Milliseconds(static_cast<float>(lfoUpdateIntervalSlider.getValue()));
    static_cast<RegionEditor*>(getParentComponent())->getAssociatedRegion()->setTimerInterval(static_cast<int>(lfoUpdateIntervalSlider.getValue()));
}
void LfoEditor::randomiseLfoUpdateInterval()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //choose a value depending on LFO frequency. the value should reflect a 1/16...2/1 rhythm = 2^(-4...2)
    lfoUpdateIntervalSlider.setValue((1.0 / lfoRateSlider.getValue()) * std::pow(2, rng.nextInt(juce::Range<int>(-4, 3))));
}

void LfoEditor::updateLfoParameter(int targetRegionID, bool shouldBeModulated, LfoModulatableParameter modulatedParameter)
{
    bool wasSuspended = audioEngine->isSuspended();

    if (!shouldBeModulated || static_cast<int>(modulatedParameter) <= 0)
    {
        audioEngine->suspendProcessing(true);
        associatedLfo->removeRegionModulation(targetRegionID); //removing modulation is the same for every region
        audioEngine->suspendProcessing(wasSuspended);
        return;
    }
    
    //there are different overloads for RegionLfo::addRegionModulation depending on whether a voice is modulated or an LFO
    audioEngine->suspendProcessing(true);
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
    audioEngine->suspendProcessing(wasSuspended);
}
