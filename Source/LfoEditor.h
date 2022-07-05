/*
  ==============================================================================

    LfoEditor.h
    Created: 30 Jun 2022 2:21:13pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
//#include "SegmentedRegion.h"
//#include "RegionLfo.h"
//#include "CheckBoxList.h"

//==============================================================================
/*
*/
//class LfoEditor  : public juce::Component
//{
//public:
//    LfoEditor(SegmentedRegion* associatedRegion)
//    {
//        this->associatedRegion = associatedRegion;
//
//        lfoRateSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
//        lfoRateSlider.setRange(0.01, 100.0, 0.01);
//        lfoRateSlider.setSkewFactorFromMidPoint(1.0);
//        lfoRateSlider.onValueChange = [this] { updateLfoRate(); };
//        addAndMakeVisible(lfoRateSlider);
//
//        lfoRateLabel.setText("LFO rate: ", juce::NotificationType::dontSendNotification);
//        addAndMakeVisible(lfoRateLabel);
//        lfoRateLabel.attachToComponent(&lfoRateSlider, true);
//
//        lfoParameterChoice.addSectionHeading("Basic");
//        lfoParameterChoice.addItem("Volume", volume);
//        lfoParameterChoice.addItem("Pitch", pitch);
//        //lfoParameterChoice.addItem("Panning", panning);
//        lfoParameterChoice.addSeparator();
//        lfoParameterChoice.addSectionHeading("LFO");
//        lfoParameterChoice.addItem("LFO Rate", lfoRate);
//        lfoParameterChoice.addSeparator();
//        lfoParameterChoice.addSectionHeading("Experimental");
//        lfoParameterChoice.addItem("Playback Position", playbackPosition);
//        lfoParameterChoice.onChange = [this] { updateLfoParameter(); };
//        addAndMakeVisible(lfoParameterChoice);
//
//        lfoParameterLabel.setText("Modulated Parameter: ", juce::NotificationType::dontSendNotification);
//        addAndMakeVisible(lfoParameterLabel);
//        lfoParameterLabel.attachToComponent(&lfoParameterChoice, true);
//
//        updateAvailableVoices();
//        addAndMakeVisible(lfoVoicesList);
//    }
//
//    ~LfoEditor() override
//    {
//    }
//
//    void paint (juce::Graphics& g) override
//    {
//        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background
//    }
//
//    void resized() override
//    {
//        auto area = getLocalBounds();
//
//        auto lfoRateArea = area.removeFromTop(20);
//        lfoRateLabel.setBounds(lfoRateArea.removeFromLeft(lfoRateArea.getWidth() / 3));
//        lfoRateSlider.setBounds(lfoRateArea);
//
//        auto lfoParamArea = area.removeFromTop(20);
//        lfoParameterLabel.setBounds(lfoParamArea.removeFromLeft(lfoParamArea.getWidth() / 3));
//        lfoParameterChoice.setBounds(lfoParamArea);
//
//        lfoVoicesList.setBounds(area);
//    }
//
//    void copyParameters()
//    {
//        auto lfo = associatedRegion->getAssociatedLfo();
//
//        //copy rate
//        lfoRateSlider.setValue(lfo->getFrequency(), juce::NotificationType::dontSendNotification);
//
//        //copy modulated parameter
//        if (lfo->getModulatedParameterID() > 0) //parameter IDs start with 1 (0 isn't included because of how selected item IDs work with ComboBoxes in juce)
//        {
//            lfoParameterChoice.setSelectedId(lfo->getModulatedParameterID(), juce::NotificationType::dontSendNotification);
//        }
//
//        //copy currently affected voices
//        auto voices = lfo->getVoices();
//        juce::Array<int> regionIDs;
//        for (auto it = voices.begin(); it != voices.end(); ++it)
//        {
//            regionIDs.add((*it)->getID());
//        }
//        lfoVoicesList.setCheckedRegions(regionIDs);
//    }
//
//private:
//    SegmentedRegion* associatedRegion;
//
//    juce::Label lfoRateLabel;
//    juce::Slider lfoRateSlider;
//
//    juce::Label lfoParameterLabel;
//    juce::ComboBox lfoParameterChoice;
//
//    CheckBoxList lfoVoicesList;
//
//    enum ModulatableParameter
//    {
//        volume = 1,
//        pitch,
//        //Panning,
//
//        lfoRate,
//        //lfoDepth
//
//        playbackPosition
//    };
//
//    void updateLfoRate()
//    {
//        associatedRegion->getAssociatedLfo()->setFrequency(lfoRateSlider.getValue());
//    }
//
//    void updateLfoParameter()
//    {
//        //note: the braces for the case statements are required here to allow for variable initialisations. see https://stackoverflow.com/questions/5136295/switch-transfer-of-control-bypasses-initialization-of-when-calling-a-function
//        switch (lfoParameterChoice.getSelectedId())
//        {
//        case volume:
//        {
//            std::function<void(float, Voice*)> func = [](float lfoValue, Voice* v) { v->modulateLevel(static_cast<double>(lfoValue)); };
//            associatedRegion->getAssociatedLfo()->setModulationFunction(func);
//            associatedRegion->getAssociatedLfo()->setModulatedParameterID(volume);
//            break;
//        }
//
//        case pitch:
//        {
//            //WIP
//            break;
//        }
//
//        case lfoRate:
//        {
//            //WIP
//            break;
//        }
//
//        case playbackPosition:
//        {
//            //WIP
//            break;
//        }
//
//        default:
//            break;
//        }
//    }
//
//    void updateLfoVoices(juce::Array<int> voiceIndices)
//    {
//        juce::Array<Voice*> voices;
//        
//        for (auto it = voiceIndices.begin(); it != voiceIndices.end(); ++it)
//        {
//            voices.addArray(associatedRegion->getAudioEngine()->getVoicesWithID(*it));
//        }
//
//        associatedRegion->getAssociatedLfo()->setVoices(voices);
//    }
//
//    void updateAvailableVoices()
//    {
//        lfoVoicesList.clearItems();
//        juce::Array<int> regionIdList;
//        
//        for (int i = 0; i < associatedRegion->getAudioEngine()->getLastRegionID(); ++i)
//        {
//            if (associatedRegion->getAudioEngine()->checkRegionHasVoice(i) == true)
//            {
//                regionIdList.add(i);
//            }
//        }
//
//        for (auto it = regionIdList.begin(); it != regionIdList.end(); ++it)
//        {
//            auto newRowNumber = lfoVoicesList.addItem("Region " + juce::String(*it), *it);
//            lfoVoicesList.setRowBackgroundColour(newRowNumber, juce::Colours::black); //WIP: how to get other regions' colours from here? maybe make a list/dictionary in AudioEngine...?
//            lfoVoicesList.setClickFunction(newRowNumber, [this] { updateLfoVoices(lfoVoicesList.getCheckedRegionIDs()); });
//        }
//    }
//
//    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoEditor)
//};
