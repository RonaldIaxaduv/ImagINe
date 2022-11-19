/*
  ==============================================================================

    PlayPathEditor.cpp
    Created: 7 Nov 2022 9:06:47am
    Author:  Aaron

  ==============================================================================
*/

#include "PlayPathEditor.h"

PlayPathEditor::PlayPathEditor(PlayPath* path)
{
    associatedPath = path;

    //interval
    courierIntervalSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    courierIntervalSlider.setTextValueSuffix("s");
    courierIntervalSlider.setRange(0.1, 600.0, 0.1);
    courierIntervalSlider.setSkewFactorFromMidPoint(60.0);
    courierIntervalSlider.onValueChange = [this] { updateCourierInterval(); };
    courierIntervalSlider.setValue(10.0, juce::NotificationType::dontSendNotification);
    addAndMakeVisible(courierIntervalSlider);

    courierIntervalLabel.setText("Courier Interval:", juce::NotificationType::dontSendNotification);
    courierIntervalLabel.attachToComponent(&courierIntervalSlider, true);
    addAndMakeVisible(courierIntervalLabel);

    //randomise button
    randomiseButton.setButtonText("Randomise Region Parameters");
    randomiseButton.onClick = [this] { randomiseAllParameters(); };
    addChildComponent(randomiseButton);

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

    auto courierIntervalArea = area.removeFromTop(20);
    courierIntervalSlider.setBounds(courierIntervalArea.removeFromRight(2 * courierIntervalArea.getWidth() / 3));
    courierIntervalLabel.setBounds(courierIntervalArea);
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
    }

    return false;
}



void PlayPathEditor::copyPathParameters()
{
    DBG("copying play path parameters...");

    courierIntervalSlider.setValue(associatedPath->getCourierInterval_seconds(), juce::NotificationType::dontSendNotification);

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

void PlayPathEditor::randomiseAllParameters()
{
    randomiseCourierInterval();
}
