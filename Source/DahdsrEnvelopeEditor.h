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
    DahdsrEnvelopeEditor(DahdsrEnvelope* associatedEnvelope)
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
        setAssociatedEnvelope(associatedEnvelope);
    }

    ~DahdsrEnvelopeEditor() override
    {
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
            titleLabel.setText(name + " DAHDSR Envelope", juce::NotificationType::dontSendNotification);
        else
            titleLabel.setText("DAHDSR Envelope", juce::NotificationType::dontSendNotification);
    }

    void setAssociatedEnvelope(DahdsrEnvelope* newEnvelope)
    {
        associatedEnvelope = newEnvelope;
        setComponentsEnabled(associatedEnvelope != nullptr);
        copyEnvelopeParameters();
    }

private:
    DahdsrEnvelope* associatedEnvelope;

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
        if (associatedEnvelope != nullptr)
        {
            delaySlider.setValue(associatedEnvelope->getDelayTime(), juce::NotificationType::dontSendNotification);
            attackSlider.setValue(associatedEnvelope->getAttackTime(), juce::NotificationType::dontSendNotification);
            holdSlider.setValue(associatedEnvelope->getHoldTime(), juce::NotificationType::dontSendNotification);
            decaySlider.setValue(associatedEnvelope->getDecayTime(), juce::NotificationType::dontSendNotification);
            releaseSlider.setValue(associatedEnvelope->getReleaseTime(), juce::NotificationType::dontSendNotification);

            initialSlider.setValue(juce::Decibels::gainToDecibels<double>(associatedEnvelope->getInitialLevel()), juce::NotificationType::dontSendNotification);
            peakSlider.setValue(juce::Decibels::gainToDecibels<double>(associatedEnvelope->getPeakLevel()), juce::NotificationType::dontSendNotification);
            sustainSlider.setValue(juce::Decibels::gainToDecibels<double>(associatedEnvelope->getSustainLevel()), juce::NotificationType::dontSendNotification);
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
        associatedEnvelope->setDelayTime(delaySlider.getValue());
    }

    void updateInitialLevel()
    {
        associatedEnvelope->setInitialLevel(juce::Decibels::decibelsToGain<double>(initialSlider.getValue(), -60.0));
    }

    void updateAttackTime()
    {
        associatedEnvelope->setAttackTime(attackSlider.getValue());
    }

    void updatePeakLevel()
    {
        associatedEnvelope->setPeakLevel(juce::Decibels::decibelsToGain<double>(peakSlider.getValue(), -60.0));
    }

    void updateHoldTime()
    {
        associatedEnvelope->setHoldTime(holdSlider.getValue());
    }

    void updateDecayTime()
    {
        associatedEnvelope->setDecayTime(decaySlider.getValue());
    }

    void updateSustainLevel()
    {
        associatedEnvelope->setSustainLevel(juce::Decibels::decibelsToGain<double>(sustainSlider.getValue(), -60.0));
    }

    void updateReleaseTime()
    {
        associatedEnvelope->setReleaseTime(releaseSlider.getValue());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DahdsrEnvelopeEditor)
};
