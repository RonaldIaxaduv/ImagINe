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
    lfoRateSlider.setRange(0.001, 100.0, 0.001);
    lfoRateSlider.setSkewFactorFromMidPoint(10.0);
    lfoRateSlider.onValueChange = [this] { updateLfoRate(); };
    lfoRateSlider.setPopupMenuEnabled(true);
    lfoRateSlider.setTooltip("This slider changes the speed at which the LFO line travels around the region.");
    addAndMakeVisible(lfoRateSlider);

    lfoRateLabel.setText("LFO rate: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoRateLabel);
    lfoRateLabel.attachToComponent(&lfoRateSlider, true);

    //starting phase
    lfoStartingPhaseSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    lfoStartingPhaseSlider.setRange(0.0, 0.999, 0.001); //for a 5-minute track, this would be a granularity of 0.3s -> should be good enough
    //lfoStartingPhaseSlider.setSkewFactorFromMidPoint(0.5);
    lfoStartingPhaseSlider.onValueChange = [this] { updateLfoStartingPhase(); };
    lfoStartingPhaseSlider.setPopupMenuEnabled(true);
    lfoStartingPhaseSlider.setTooltip("This slider changes the starting phase of the LFO, i.e. the angle at which the LFO line starts.");
    addAndMakeVisible(lfoStartingPhaseSlider);

    lfoStartingPhaseLabel.setText("LFO starting phase: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoStartingPhaseLabel);
    lfoStartingPhaseLabel.attachToComponent(&lfoStartingPhaseSlider, true);

    //phase interval
    lfoPhaseIntervalSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    lfoPhaseIntervalSlider.setRange(0.001, 1.0, 0.001); //MUST NOT BE ZERO!; for a 5-minute track, this would be a granularity of 0.3s -> should be good enough
    //lfoPhaseIntervalSlider.setSkewFactorFromMidPoint(0.5);
    lfoPhaseIntervalSlider.onValueChange = [this] { updateLfoPhaseInterval(); };
    lfoPhaseIntervalSlider.setPopupMenuEnabled(true);
    lfoPhaseIntervalSlider.setTooltip("This slider changes the phase interval, i.e. the range along which the LFO will travel. At value 1.0, it will go all the way around; at 0.5, it will only go halfway before resetting; when set to 0.0, the line will be completely static.");
    addAndMakeVisible(lfoPhaseIntervalSlider);

    lfoPhaseIntervalLabel.setText("LFO phase interval: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoPhaseIntervalLabel);
    lfoPhaseIntervalLabel.attachToComponent(&lfoPhaseIntervalSlider, true);

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

    lfoUpdateQuantisationChoice.addItem("1/1T", static_cast<int>(UpdateRateQuantisationMethod::full_triole) + 1); //always adding 1 because 0 is not a valid ID (reserved for other purposes)
    lfoUpdateQuantisationChoice.addItem("1/2.", static_cast<int>(UpdateRateQuantisationMethod::half_dotted) + 1);
    lfoUpdateQuantisationChoice.addItem("1/2", static_cast<int>(UpdateRateQuantisationMethod::half) + 1);
    lfoUpdateQuantisationChoice.addItem("1/2T", static_cast<int>(UpdateRateQuantisationMethod::half_triole) + 1);
    lfoUpdateQuantisationChoice.addItem("1/4.", static_cast<int>(UpdateRateQuantisationMethod::quarter_dotted) + 1);
    lfoUpdateQuantisationChoice.addItem("1/4", static_cast<int>(UpdateRateQuantisationMethod::quarter) + 1);
    lfoUpdateQuantisationChoice.addItem("1/4T", static_cast<int>(UpdateRateQuantisationMethod::quarter_triole) + 1);
    lfoUpdateQuantisationChoice.addItem("1/8.", static_cast<int>(UpdateRateQuantisationMethod::eighth_dotted) + 1);
    lfoUpdateQuantisationChoice.addItem("1/8", static_cast<int>(UpdateRateQuantisationMethod::eighth) + 1);
    lfoUpdateQuantisationChoice.addItem("1/8T", static_cast<int>(UpdateRateQuantisationMethod::eighth_triole) + 1);
    lfoUpdateQuantisationChoice.addItem("1/16.", static_cast<int>(UpdateRateQuantisationMethod::sixteenth_dotted) + 1);
    lfoUpdateQuantisationChoice.addItem("1/16", static_cast<int>(UpdateRateQuantisationMethod::sixteenth) + 1);
    lfoUpdateQuantisationChoice.addItem("1/16T", static_cast<int>(UpdateRateQuantisationMethod::sixteenth_triole) + 1);
    lfoUpdateQuantisationChoice.addItem("1/32.", static_cast<int>(UpdateRateQuantisationMethod::thirtysecond_dotted) + 1);
    lfoUpdateQuantisationChoice.addItem("1/32", static_cast<int>(UpdateRateQuantisationMethod::thirtysecond) + 1);
    lfoUpdateQuantisationChoice.addItem("1/32T", static_cast<int>(UpdateRateQuantisationMethod::thirtysecond_triole) + 1);
    lfoUpdateQuantisationChoice.addItem("1/64.", static_cast<int>(UpdateRateQuantisationMethod::sixtyfourth_dotted) + 1);
    lfoUpdateQuantisationChoice.addItem("1/64", static_cast<int>(UpdateRateQuantisationMethod::sixtyfourth) + 1);
    lfoUpdateQuantisationChoice.addItem("1/64T", static_cast<int>(UpdateRateQuantisationMethod::sixtyfourth_triole) + 1);
    lfoUpdateQuantisationChoice.addItem("continuous (no quantisation)", static_cast<int>(UpdateRateQuantisationMethod::continuous) + 1);
    lfoUpdateQuantisationChoice.onChange = [this]
    {
        updateLfoUpdateQuantisation();
    };
    lfoUpdateQuantisationChoice.setTooltip("If you want updates to only occur in a certain relation to the base value, you can choose so here. This does not affect the update rate slider itself, only update rate modulation. Its main use is to create musical rhythms.");
    addAndMakeVisible(lfoUpdateQuantisationChoice);

    lfoUpdateQuantisationLabel.setText("Update Rate Quantisation: ", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(lfoUpdateQuantisationLabel);
    lfoUpdateQuantisationLabel.attachToComponent(&lfoUpdateQuantisationChoice, true);

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

void LfoEditor::setUnitOfHeight(int newHUnit)
{
    hUnit = newHUnit;
}
void LfoEditor::resized()
{
    auto area = getLocalBounds();

    auto lfoRateArea = area.removeFromTop(hUnit);
    lfoRateSlider.setBounds(lfoRateArea.removeFromRight(2 * lfoRateArea.getWidth() / 3).reduced(1));
    lfoRateSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, lfoRateSlider.getWidth(), lfoRateSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto lfoStartingPhaseArea = area.removeFromTop(hUnit);
    lfoStartingPhaseSlider.setBounds(lfoStartingPhaseArea.removeFromRight(2 * lfoStartingPhaseArea.getWidth() / 3).reduced(1));
    lfoStartingPhaseSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, lfoStartingPhaseSlider.getWidth(), lfoStartingPhaseSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto lfoPhaseIntervalArea = area.removeFromTop(hUnit);
    lfoPhaseIntervalSlider.setBounds(lfoPhaseIntervalArea.removeFromRight(2 * lfoPhaseIntervalArea.getWidth() / 3).reduced(1));
    lfoPhaseIntervalSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, lfoStartingPhaseSlider.getWidth(), lfoStartingPhaseSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto lfoUpdateIntervalArea = area.removeFromTop(hUnit);
    lfoUpdateIntervalSlider.setBounds(lfoUpdateIntervalArea.removeFromRight(2 * lfoUpdateIntervalArea.getWidth() / 3).reduced(1));
    lfoUpdateIntervalSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxAbove, false, lfoUpdateIntervalSlider.getWidth(), lfoUpdateIntervalSlider.getHeight()); //this may look redundant, but the tooltip won't display unless this is done...

    auto lfoUpdateQuantisationArea = area.removeFromTop(hUnit);
    lfoUpdateQuantisationChoice.setBounds(lfoUpdateQuantisationArea.removeFromRight(2 * lfoUpdateQuantisationArea.getWidth() / 3).reduced(1));

    lfoRegionsList.setUnitOfHeight(hUnit);
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
        else if (lfoStartingPhaseSlider.getBounds().contains(mousePos))
        {
            randomiseLfoStartingPhase();
            return true;
        }
        else if (lfoPhaseIntervalSlider.getBounds().contains(mousePos))
        {
            randomiseLfoPhaseInterval();
            return true;
        }
        else if (lfoUpdateIntervalSlider.getBounds().contains(mousePos))
        {
            randomiseLfoUpdateInterval();
            return true;
        }
        else if (lfoUpdateQuantisationChoice.getBounds().contains(mousePos))
        {
            randomiseLfoUpdateQuantisation();
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
    lfoRateSlider.setValue(associatedLfo->getBaseFrequency(), juce::NotificationType::dontSendNotification);

    lfoStartingPhaseSlider.setValue(associatedLfo->getBaseStartingPhase(), juce::NotificationType::dontSendNotification);
    lfoPhaseIntervalSlider.setValue(associatedLfo->getBasePhaseInterval(), juce::NotificationType::dontSendNotification);

    lfoUpdateIntervalSlider.setValue(associatedLfo->getUpdateInterval_Milliseconds(), juce::NotificationType::dontSendNotification);
    lfoUpdateQuantisationChoice.setSelectedId(static_cast<int>(associatedLfo->getUpdateRateQuantisationMethod()) + 1, juce::NotificationType::dontSendNotification);

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
    randomiseLfoStartingPhase();
    randomiseLfoPhaseInterval();
    randomiseLfoUpdateInterval();
    randomiseLfoUpdateQuantisation();
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
    lfoRateSlider.setValue((lfoRateSlider.getMinimum() + rng.nextDouble() * (lfoRateSlider.getMaximum() - lfoRateSlider.getMinimum())) / (1.0 + rng.nextDouble() * 0.75 * lfoRateSlider.getMaximum()), juce::NotificationType::sendNotification);
}

void LfoEditor::updateLfoStartingPhase()
{
    associatedLfo->setBaseStartingPhase(lfoStartingPhaseSlider.getValue());
    associatedLfo->resetPhase(); //better for visual feedback
    static_cast<RegionEditor*>(getParentComponent())->getAssociatedRegion()->timerCallback(); //repaint LFO line
}
void LfoEditor::randomiseLfoStartingPhase()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //completely random values within the range are fine
    lfoStartingPhaseSlider.setValue(lfoStartingPhaseSlider.getMinimum() + rng.nextDouble() * (lfoStartingPhaseSlider.getMaximum() - lfoStartingPhaseSlider.getMinimum()), juce::NotificationType::sendNotification);
    associatedLfo->resetPhase(); //better for visual feedback
    static_cast<RegionEditor*>(getParentComponent())->getAssociatedRegion()->timerCallback(); //repaint LFO line
}

void LfoEditor::updateLfoPhaseInterval()
{
    associatedLfo->setBasePhaseInterval(lfoPhaseIntervalSlider.getValue());
}
void LfoEditor::randomiseLfoPhaseInterval()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //prefer values closer to 1.0 (using sqrt)
    if (rng.nextInt(3) < 2)
    {
        //66%: set to 1
        lfoPhaseIntervalSlider.setValue(lfoPhaseIntervalSlider.getMaximum(), juce::NotificationType::sendNotification);
    }
    else
    {
        //33%: random value, but prefer values closer to 1.0 (by using fractional powers)
        lfoPhaseIntervalSlider.setValue(std::pow(lfoPhaseIntervalSlider.getMinimum() + rng.nextDouble() * (lfoPhaseIntervalSlider.getMaximum() - lfoPhaseIntervalSlider.getMinimum()), 0.25), juce::NotificationType::sendNotification);
    }
}

void LfoEditor::updateLfoUpdateInterval()
{
    associatedLfo->setUpdateInterval_Milliseconds(static_cast<float>(lfoUpdateIntervalSlider.getValue()));
    static_cast<RegionEditor*>(getParentComponent())->getAssociatedRegion()->setTimerInterval(static_cast<int>(lfoUpdateIntervalSlider.getValue()));
}
void LfoEditor::randomiseLfoUpdateInterval()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    ////choose a value depending on LFO frequency. the value should reflect a 1/16...2/1 rhythm = 2^(-4...2)
    //lfoUpdateIntervalSlider.setValue((1.0 / lfoRateSlider.getValue()) * std::pow(2, rng.nextInt(juce::Range<int>(-4, 3))), juce::NotificationType::sendNotification);

    //either prefer a value close to 0 or any value
    if (rng.nextInt(2) < 1)
    {
        //50% chance: value close to 0
        lfoUpdateIntervalSlider.setValue((lfoUpdateIntervalSlider.getMinimum() + rng.nextDouble() * (lfoUpdateIntervalSlider.getMaximum() - lfoUpdateIntervalSlider.getMinimum())) / (1.0 + rng.nextDouble() * 9.0), juce::NotificationType::sendNotification);
    }
    else
    {
        //50% chance: any value
        lfoUpdateIntervalSlider.setValue(lfoUpdateIntervalSlider.getMinimum() + rng.nextDouble() * (lfoUpdateIntervalSlider.getMaximum() - lfoUpdateIntervalSlider.getMinimum()), juce::NotificationType::sendNotification);
    }
}

void LfoEditor::updateLfoUpdateQuantisation()
{
    associatedLfo->setUpdateRateQuantisationMethod(static_cast<UpdateRateQuantisationMethod>(lfoUpdateQuantisationChoice.getSelectedId() - 1));
}
void LfoEditor::randomiseLfoUpdateQuantisation()
{
    juce::Random& rng = juce::Random::getSystemRandom();

    //50% chance to be continuous. otherwise, choose a random entry
    if (rng.nextFloat() < 0.5f)
    {
        lfoUpdateQuantisationChoice.setSelectedItemIndex(0, juce::NotificationType::sendNotification);
    }
    else
    {
        lfoUpdateQuantisationChoice.setSelectedItemIndex(rng.nextInt(juce::Range<int>(1, lfoUpdateQuantisationChoice.getNumItems())), juce::NotificationType::sendNotification);
    }
}

void LfoEditor::updateLfoParameter(int targetRegionID, bool shouldBeModulated, LfoModulatableParameter modulatedParameter)
{
    audioEngine->updateLfoParameter(associatedLfo->getRegionID(), targetRegionID, shouldBeModulated, modulatedParameter);
}
