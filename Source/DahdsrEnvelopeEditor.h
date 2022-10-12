/*
  ==============================================================================

    DahdsrEnvelopeEditor.h
    Created: 7 Jul 2022 4:34:34pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DahdsrEnvelope.h"

//==============================================================================
/*
*/
class DahdsrEnvelopeEditor  : public juce::Component
{
public:
    DahdsrEnvelopeEditor(juce::Array<DahdsrEnvelope*> associatedEnvelopes)
    {
        //initialise components
        titleLabel.setText("DAHDSR Envelope", juce::NotificationType::dontSendNotification);
        addAndMakeVisible(titleLabel);

        initialiseTimeSlider(&delaySlider, [this] { updateDelayTime(); });
        initialiseSliderLabel(&delayLabel, "Delay", &delaySlider);

        initialiseTimeSlider(&attackSlider, [this] { updateAttackTime(); });
        initialiseSliderLabel(&attackLabel, "Attack", &attackSlider);

        initialiseTimeSlider(&holdSlider, [this] { updateHoldTime(); });
        initialiseSliderLabel(&holdLabel, "Hold", &holdSlider);

        initialiseTimeSlider(&decaySlider, [this] { updateDecayTime(); });
        initialiseSliderLabel(&decayLabel, "Decay", &decaySlider);

        initialiseTimeSlider(&releaseSlider, [this] { updateReleaseTime(); });
        initialiseSliderLabel(&releaseLabel, "Release", &releaseSlider);


        initialiseLevelSlider(&initialSlider, [this] { updateInitialLevel(); });
        initialiseSliderLabel(&initialLabel, "Initial", &initialSlider);

        initialiseLevelSlider(&peakSlider, [this] { updatePeakLevel(); });
        initialiseSliderLabel(&peakLabel, "Peak", &peakSlider);

        initialiseLevelSlider(&sustainSlider, [this] { updateSustainLevel(); });
        initialiseSliderLabel(&sustainLabel, "Sustain", &sustainSlider);

        //copy current parameter values
        setAssociatedEnvelopes(associatedEnvelopes);
    }

    ~DahdsrEnvelopeEditor() override
    {
        associatedEnvelopes.clear();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background
    }

    void resized() override
    {
        auto area = getLocalBounds();

        titleLabel.setBounds(area.removeFromTop(20));

        //2 rows, 5 columns. the first row contains times, the second one levels
        auto timeArea = area.removeFromTop(area.getHeight() / 2);
        auto levelArea = area;

        int widthFifth = timeArea.getWidth() / 5;
        int rowHeightTwoThirds = 2 * timeArea.getHeight() / 3; //leaves space for labels above the sliders

        delaySlider.setBounds(timeArea.removeFromLeft(widthFifth).removeFromBottom(rowHeightTwoThirds).reduced(2));
        attackSlider.setBounds(timeArea.removeFromLeft(widthFifth).removeFromBottom(rowHeightTwoThirds).reduced(2));
        holdSlider.setBounds(timeArea.removeFromLeft(widthFifth).removeFromBottom(rowHeightTwoThirds).reduced(2));
        decaySlider.setBounds(timeArea.removeFromLeft(widthFifth).removeFromBottom(rowHeightTwoThirds).reduced(2));
        releaseSlider.setBounds(timeArea.removeFromBottom(rowHeightTwoThirds).reduced(2));

        levelArea.removeFromLeft(widthFifth); //no level associated with delay
        initialSlider.setBounds(levelArea.removeFromLeft(widthFifth).removeFromBottom(rowHeightTwoThirds).reduced(2));
        peakSlider.setBounds(levelArea.removeFromLeft(widthFifth).removeFromBottom(rowHeightTwoThirds).reduced(2));
        sustainSlider.setBounds(levelArea.removeFromLeft(widthFifth).removeFromBottom(rowHeightTwoThirds).reduced(2));
        //levelArea //final level of release is always 0.0 in this case
    }

    /// <summary>
    /// Sets the text of the label at the top of the component to "[name] DAHDSR Envelope"
    /// </summary>
    /// <param name="name">specifies what the envelope modulates (e.g. Amp or LFO)</param>
    void setDisplayedName(juce::String name)
    {
        if (name != "")
            titleLabel.setText(name + " DAHDSR Envelope:", juce::NotificationType::dontSendNotification);
        else
            titleLabel.setText("DAHDSR Envelope:", juce::NotificationType::dontSendNotification);
    }

    void setAssociatedEnvelopes(juce::Array<DahdsrEnvelope*> newEnvelopes)
    {
        associatedEnvelopes = newEnvelopes;
        setComponentsEnabled(associatedEnvelopes.size() > 0);
        copyEnvelopeParameters();
    }

private:
    juce::Array<DahdsrEnvelope*> associatedEnvelopes;

    juce::Label titleLabel; //will read "[referenced sound] DAHDSR Envelope"

    juce::Label delayLabel;
    juce::Slider delaySlider;

    juce::Label attackLabel;
    juce::Slider attackSlider;

    juce::Label initialLabel;
    juce::Slider initialSlider;

    juce::Label holdLabel;
    juce::Slider holdSlider;

    juce::Label peakLabel;
    juce::Slider peakSlider;

    juce::Label decayLabel;
    juce::Slider decaySlider;

    juce::Label sustainLabel;
    juce::Slider sustainSlider;

    juce::Label releaseLabel;
    juce::Slider releaseSlider;

    //================================================================

    void initialiseTimeSlider(juce::Slider* sliderToInitialise, std::function<void ()> valueChangeFunction)
    {
        sliderToInitialise->setSliderStyle(juce::Slider::SliderStyle::LinearBar);
        sliderToInitialise->setTextValueSuffix("s");
        sliderToInitialise->setRange(0.0, 10.0, 0.0001); //in seconds (minimum interval: 0.1ms)
        sliderToInitialise->setSkewFactorFromMidPoint(1.0);
        sliderToInitialise->setNumDecimalPlacesToDisplay(4);
        sliderToInitialise->onValueChange = valueChangeFunction;
        addAndMakeVisible(sliderToInitialise);
    }

    void initialiseLevelSlider(juce::Slider* sliderToInitialise, std::function<void()> valueChangeFunction)
    {
        sliderToInitialise->setSliderStyle(juce::Slider::SliderStyle::LinearBar);
        sliderToInitialise->setTextValueSuffix("dB");
        sliderToInitialise->setRange(-60.0, 6.0, 0.01);
        sliderToInitialise->setSkewFactorFromMidPoint(-6.0);
        sliderToInitialise->setNumDecimalPlacesToDisplay(2);
        sliderToInitialise->onValueChange = valueChangeFunction;
        addAndMakeVisible(sliderToInitialise);
    }

    void initialiseSliderLabel(juce::Label* labelToInitialise, juce::String text, juce::Slider* sliderToAttachTo)
    {
        labelToInitialise->setFont(juce::Font(9.0f)); //very small font
        labelToInitialise->setJustificationType(juce::Justification::centredBottom);
        labelToInitialise->setText(text, juce::NotificationType::dontSendNotification);
        labelToInitialise->attachToComponent(sliderToAttachTo, false); //on top of slider
        addAndMakeVisible(labelToInitialise);
    }


    void copyEnvelopeParameters()
    {
        if (associatedEnvelopes.size() > 0)
        {
            delaySlider.setValue(associatedEnvelopes[0]->getDelayTime(), juce::NotificationType::dontSendNotification);
            attackSlider.setValue(associatedEnvelopes[0]->getAttackTime(), juce::NotificationType::dontSendNotification);
            holdSlider.setValue(associatedEnvelopes[0]->getHoldTime(), juce::NotificationType::dontSendNotification);
            decaySlider.setValue(associatedEnvelopes[0]->getDecayTime(), juce::NotificationType::dontSendNotification);
            releaseSlider.setValue(associatedEnvelopes[0]->getReleaseTime(), juce::NotificationType::dontSendNotification);

            initialSlider.setValue(juce::Decibels::gainToDecibels<double>(associatedEnvelopes[0]->getInitialLevel()), juce::NotificationType::dontSendNotification);
            peakSlider.setValue(juce::Decibels::gainToDecibels<double>(associatedEnvelopes[0]->getPeakLevel()), juce::NotificationType::dontSendNotification);
            sustainSlider.setValue(juce::Decibels::gainToDecibels<double>(associatedEnvelopes[0]->getSustainLevel()), juce::NotificationType::dontSendNotification);
        }
    }

    void setComponentsEnabled(bool shouldBeEnabled)
    {
        delayLabel.setEnabled(shouldBeEnabled);
        delaySlider.setEnabled(shouldBeEnabled);

        initialLabel.setEnabled(shouldBeEnabled);
        initialSlider.setEnabled(shouldBeEnabled);

        attackLabel.setEnabled(shouldBeEnabled);
        attackSlider.setEnabled(shouldBeEnabled);

        peakLabel.setEnabled(shouldBeEnabled);
        peakSlider.setEnabled(shouldBeEnabled);

        holdLabel.setEnabled(shouldBeEnabled);
        holdSlider.setEnabled(shouldBeEnabled);

        decayLabel.setEnabled(shouldBeEnabled);
        decaySlider.setEnabled(shouldBeEnabled);

        sustainLabel.setEnabled(shouldBeEnabled);
        sustainSlider.setEnabled(shouldBeEnabled);

        releaseLabel.setEnabled(shouldBeEnabled);
        releaseSlider.setEnabled(shouldBeEnabled);
    }


    void updateDelayTime()
    {
        for (auto it = associatedEnvelopes.begin(); it != associatedEnvelopes.end(); it++)
        {
            (*it)->setDelayTime(delaySlider.getValue());
        }
    }

    void updateInitialLevel()
    {
        for (auto it = associatedEnvelopes.begin(); it != associatedEnvelopes.end(); it++)
        {
            (*it)->setInitialLevel(juce::Decibels::decibelsToGain<double>(initialSlider.getValue(), -60.0));
        }
    }

    void updateAttackTime()
    {
        for (auto it = associatedEnvelopes.begin(); it != associatedEnvelopes.end(); it++)
        {
            (*it)->setAttackTime(attackSlider.getValue());
        }
    }

    void updatePeakLevel()
    {
        for (auto it = associatedEnvelopes.begin(); it != associatedEnvelopes.end(); it++)
        {
            (*it)->setPeakLevel(juce::Decibels::decibelsToGain<double>(peakSlider.getValue(), -60.0));
        }
    }

    void updateHoldTime()
    {
        for (auto it = associatedEnvelopes.begin(); it != associatedEnvelopes.end(); it++)
        {
            (*it)->setHoldTime(holdSlider.getValue());
        }
    }

    void updateDecayTime()
    {
        for (auto it = associatedEnvelopes.begin(); it != associatedEnvelopes.end(); it++)
        {
            (*it)->setDecayTime(decaySlider.getValue());
        }
    }

    void updateSustainLevel()
    {
        for (auto it = associatedEnvelopes.begin(); it != associatedEnvelopes.end(); it++)
        {
            (*it)->setSustainLevel(juce::Decibels::decibelsToGain<double>(sustainSlider.getValue(), -60.0));
        }
    }

    void updateReleaseTime()
    {
        for (auto it = associatedEnvelopes.begin(); it != associatedEnvelopes.end(); it++)
        {
            (*it)->setReleaseTime(releaseSlider.getValue());
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DahdsrEnvelopeEditor)
};
