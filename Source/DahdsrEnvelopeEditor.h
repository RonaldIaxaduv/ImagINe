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
class DahdsrEnvelopeEditor final : public juce::Component, public juce::SettableTooltipClient
{
public:
    DahdsrEnvelopeEditor(juce::Array<DahdsrEnvelope*> associatedEnvelopes)
    {
        //initialise components
        titleLabel.setText("DAHDSR Envelope", juce::NotificationType::dontSendNotification);
        addAndMakeVisible(titleLabel);

        initialiseTimeSlider(&delaySlider, [this] { updateDelayTime(); });
        delaySlider.setPopupMenuEnabled(true);
        delaySlider.setTooltip("This slider changes how much time passes until the audio becomes audible after a region has started to be played.");
        initialiseSliderLabel(&delayLabel, "Delay", &delaySlider);

        initialiseTimeSlider(&attackSlider, [this] { updateAttackTime(); });
        attackSlider.setPopupMenuEnabled(true);
        attackSlider.setTooltip("This slider changes how long it takes to get from the initial volume to the peak volume of the audio.");
        initialiseSliderLabel(&attackLabel, "Attack", &attackSlider);

        initialiseTimeSlider(&holdSlider, [this] { updateHoldTime(); });
        holdSlider.setPopupMenuEnabled(true);
        holdSlider.setTooltip("This slider changes how long the peak volume is kept up.");
        initialiseSliderLabel(&holdLabel, "Hold", &holdSlider);

        initialiseTimeSlider(&decaySlider, [this] { updateDecayTime(); });
        decaySlider.setPopupMenuEnabled(true);
        decaySlider.setTooltip("This slider changes how long it takes to get from the peak volume to the sustain volume of the audio.");
        initialiseSliderLabel(&decayLabel, "Decay", &decaySlider);

        initialiseTimeSlider(&releaseSlider, [this] { updateReleaseTime(); });
        releaseSlider.setPopupMenuEnabled(true);
        releaseSlider.setTooltip("This slider changes how long it takes for the audio to fade out after a region has requested to stop playing.");
        initialiseSliderLabel(&releaseLabel, "Release", &releaseSlider);


        initialiseLevelSlider(&initialSlider, [this] { updateInitialLevel(); });
        initialSlider.setPopupMenuEnabled(true);
        initialSlider.setTooltip("This slider changes the initial volume of the audio that's used after the delay stage, at the beginning of the attack stage.");
        initialiseSliderLabel(&initialLabel, "Initial", &initialSlider);

        initialiseLevelSlider(&peakSlider, [this] { updatePeakLevel(); });
        peakSlider.setPopupMenuEnabled(true);
        peakSlider.setTooltip("This slider changes the peak volume of the audio that's used after the attack stage, during the hold stage and at the beginning of the decay stage.");
        initialiseSliderLabel(&peakLabel, "Peak", &peakSlider);

        initialiseLevelSlider(&sustainSlider, [this] { updateSustainLevel(); });
        sustainSlider.setPopupMenuEnabled(true);
        sustainSlider.setTooltip("This slider changes the sustain volume of the audio that's used after the decay stage, during the sustain stage and at the beginning of the release stage.");
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

        //update textbox styles - this may look redundant, but the tooltip won't display unless this is done...
        delaySlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, delaySlider.getWidth(), delaySlider.getHeight());
        attackSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, attackSlider.getWidth(), attackSlider.getHeight());
        holdSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, holdSlider.getWidth(), holdSlider.getHeight());
        decaySlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, decaySlider.getWidth(), decaySlider.getHeight());
        releaseSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, releaseSlider.getWidth(), releaseSlider.getHeight());
        initialSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, initialSlider.getWidth(), initialSlider.getHeight());
        peakSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, peakSlider.getWidth(), peakSlider.getHeight());
        sustainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, delaySlider.getWidth(), sustainSlider.getHeight());
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::createFromDescription("ctrl + r"))
        {
            auto mousePos = getMouseXYRelative();
            juce::Component* target = getComponentAt(mousePos.getX(), mousePos.getY());

            //time sliders
            if (delaySlider.getBounds().contains(mousePos))
            {
                randomiseTimeSlider(&delaySlider);
                return true;
            }
            else if(attackSlider.getBounds().contains(mousePos))
            {
                randomiseTimeSlider(&attackSlider);
                return true;
            }
            else if (holdSlider.getBounds().contains(mousePos))
            {
                randomiseTimeSlider(&holdSlider);
                return true;
            }
            else if (decaySlider.getBounds().contains(mousePos))
            {
                randomiseTimeSlider(&decaySlider);
                return true;
            }
            else if (releaseSlider.getBounds().contains(mousePos))
            {
                randomiseTimeSlider(&releaseSlider);
                return true;
            }

            //level sliders
            else if (initialSlider.getBounds().contains(mousePos))
            {
                randomiseInitialSlider();
                return true;
            }
            else if (peakSlider.getBounds().contains(mousePos))
            {
                randomisePeakSlider();
                return true;
            }
            else if (sustainSlider.getBounds().contains(mousePos))
            {
                randomiseSustainSlider();
                return true;
            }
        }

        return false;
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

    void randomiseAllParameters()
    {
        //time sliders
        //randomiseTimeSlider(&delaySlider); //no delay time - the delay is probably more of a coordination tool, so it's better not to randomise it.
        randomiseTimeSlider(&attackSlider);
        randomiseTimeSlider(&holdSlider);
        randomiseTimeSlider(&decaySlider);
        randomiseTimeSlider(&releaseSlider);

        //level sliders
        randomiseInitialSlider();
        randomisePeakSlider();
        randomiseSustainSlider();
    }

    void setUnitOfHeight(int newHUnit)
    {
        hUnit = newHUnit;
    }

private:
    const double sliderMinimumLevel = -60.0;
    const double sliderMaximumLevel = 6.0;
    const double sliderMaximumTime = 60.0;

    int hUnit = 20;

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
        sliderToInitialise->setRange(0.0, sliderMaximumTime, 0.0001); //in seconds (minimum interval: 0.1ms)
        sliderToInitialise->setSkewFactorFromMidPoint(1.0);
        sliderToInitialise->setNumDecimalPlacesToDisplay(4);
        sliderToInitialise->onValueChange = valueChangeFunction;
        addAndMakeVisible(sliderToInitialise);
    }
    void randomiseTimeSlider(juce::Slider* sliderToRandomise)
    {
        juce::Random& rng = juce::Random::getSystemRandom();

        //prefer values closer to 0. since the min value is 0, the calculation is slightly simplified.
        sliderToRandomise->setValue((rng.nextDouble() * sliderMaximumTime) / (1.0 + rng.nextDouble() * 0.5 * sliderMaximumTime), juce::NotificationType::sendNotification);
    }

    void initialiseLevelSlider(juce::Slider* sliderToInitialise, std::function<void()> valueChangeFunction)
    {
        sliderToInitialise->setSliderStyle(juce::Slider::SliderStyle::LinearBar);
        sliderToInitialise->setTextValueSuffix("dB");
        sliderToInitialise->setRange(sliderMinimumLevel, sliderMaximumLevel, 0.01);
        sliderToInitialise->setSkewFactorFromMidPoint(-6.0);
        sliderToInitialise->setNumDecimalPlacesToDisplay(2);
        sliderToInitialise->onValueChange = valueChangeFunction;
        addAndMakeVisible(sliderToInitialise);
    }
    //void randomiseLevelSlider(juce::Slider* sliderToRandomise)
    //{
    //    juce::Random& rng = juce::Random::getSystemRandom();

    //    //level sliders -> prefer values closer to 0 (by dividing the result randomly by a value within [1, 6] -> average value of -7,7dB)
    //    sliderToRandomise->setValue((sliderMinimumLevel + rng.nextDouble() * (sliderMaximumLevel - sliderMinimumLevel)) / (1.0 + 5.0 * rng.nextDouble()), juce::NotificationType::sendNotification);
    //}
    void randomiseInitialSlider()
    {
        juce::Random& rng = juce::Random::getSystemRandom();

        //prefer either a value close to silence (minimum value) or 0dB
        if (rng.nextInt(10) < 7) //70% chance
        {
            //prefer value close to silence
            initialSlider.setValue(sliderMinimumLevel + rng.nextDouble() * (-30.0 - sliderMinimumLevel), juce::NotificationType::sendNotification); //values between -60 and -30
        }
        else //30% chance
        {
            //prefer value close to 0dB
            initialSlider.setValue((-30.0 + rng.nextDouble() * (sliderMaximumLevel - -30.0)) / (1.0 + 5.0 * rng.nextDouble()), juce::NotificationType::sendNotification); //values between -30 and 6
        }
    }
    void randomisePeakSlider()
    {
        juce::Random& rng = juce::Random::getSystemRandom();

        //level sliders -> prefer values closer to 0, but allow any value in the possible range
        peakSlider.setValue((sliderMinimumLevel + rng.nextDouble() * (sliderMaximumLevel - sliderMinimumLevel)) / (1.0 + 5.0 * rng.nextDouble()), juce::NotificationType::sendNotification);
    }
    void randomiseSustainSlider()
    {
        juce::Random& rng = juce::Random::getSystemRandom();

        //prefer either a value close to silence (minimum value) or 0dB. if the peak level or initial level are on the louder side, prefer values closer to silence more.
        int silenceThreshold = 1 + (initialSlider.getValue() < 0.75 * sliderMinimumLevel ? 1 : 0) + (peakSlider.getValue() < 0.75 * sliderMinimumLevel ? 1 : 0);
        int zeroThreshold = 1 + (initialSlider.getValue() > 0.25 * sliderMinimumLevel ? 1 : 0) + (peakSlider.getValue() > 0.25 * sliderMinimumLevel ? 1 : 0);
        if (rng.nextInt(silenceThreshold + zeroThreshold) < silenceThreshold) //chance of silenceThreshold/zeroThreshold
        {
            //prefer value close to silence
            sustainSlider.setValue(sliderMinimumLevel / (1.0 + rng.nextDouble()), juce::NotificationType::sendNotification); //values between -60 and -30
        }
        else //chance of 1 - silenceThreshold/zeroThreshold
        {
            //prefer value close to 0dB
            sustainSlider.setValue((0.5 * sliderMinimumLevel + rng.nextDouble() * (sliderMaximumLevel - sliderMinimumLevel * 0.5)) / (1.0 + 5.0 * rng.nextDouble()), juce::NotificationType::sendNotification); //values between -30 and 6
        }
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
