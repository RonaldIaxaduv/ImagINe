/*
  ==============================================================================

    CheckBoxList.h
    Created: 1 Jul 2022 10:00:41am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "LfoModulatableParameter.h"


class CheckBoxListItem final : public juce::Component, public juce::ChangeBroadcaster, public juce::TooltipClient
{
public:
    CheckBoxListItem(juce::String text, int row, int regionID)
    {
        this->row = row;
        this->regionID = regionID;
        backgroundColour = juce::Colours::transparentBlack;

        setInterceptsMouseClicks(false, true);

        regionButton.setButtonText(text);
        regionButton.onClick = [this]
        {
            modulationChoice.setEnabled(regionButton.getToggleState());
            sendChangeMessage();
        };
        regionButton.setTooltip(getTooltip());
        addAndMakeVisible(regionButton);

        modulationChoice.addSectionHeading("Voice");
        modulationChoice.addItem("Volume", static_cast<int>(LfoModulatableParameter::volume));
        modulationChoice.addItem("Volume (inverted)", static_cast<int>(LfoModulatableParameter::volume_inverted));
        modulationChoice.addItem("Pitch", static_cast<int>(LfoModulatableParameter::pitch));
        modulationChoice.addItem("Pitch (inverted)", static_cast<int>(LfoModulatableParameter::pitch_inverted));
        //modulationChoice.addItem("Panning", static_cast<int>(LfoModulatableParameter::panning));
        //modulationChoice.addItem("Panning (inverted)", static_cast<int>(LfoModulatableParameter::panning_inverted));
        modulationChoice.addItem("Playback Starting Position", static_cast<int>(LfoModulatableParameter::playbackPositionStart));
        modulationChoice.addItem("Playback Starting Position (inverted)", static_cast<int>(LfoModulatableParameter::playbackPositionStart_inverted));
        modulationChoice.addItem("Playback Interval", static_cast<int>(LfoModulatableParameter::playbackPositionInterval));
        modulationChoice.addItem("Playback Interval (inverted)", static_cast<int>(LfoModulatableParameter::playbackPositionInterval_inverted));
        modulationChoice.addItem("Playback Current Position", static_cast<int>(LfoModulatableParameter::playbackPositionCurrent));
        modulationChoice.addItem("Playback Current Position (inverted)", static_cast<int>(LfoModulatableParameter::playbackPositionCurrent_inverted));
        modulationChoice.addItem("Filter Position", static_cast<int>(LfoModulatableParameter::filterPosition));
        modulationChoice.addItem("Filter Position (inverted)", static_cast<int>(LfoModulatableParameter::filterPosition_inverted));
        modulationChoice.addSeparator();
        modulationChoice.addSectionHeading("LFO");
        modulationChoice.addItem("LFO Rate", static_cast<int>(LfoModulatableParameter::lfoRate));
        modulationChoice.addItem("LFO Rate (inverted)", static_cast<int>(LfoModulatableParameter::lfoRate_inverted));
        modulationChoice.addItem("LFO Starting Phase", static_cast<int>(LfoModulatableParameter::lfoStartingPhase));
        modulationChoice.addItem("LFO Starting Phase (inverted)", static_cast<int>(LfoModulatableParameter::lfoStartingPhase_inverted));
        modulationChoice.addItem("LFO Phase Interval", static_cast<int>(LfoModulatableParameter::lfoPhaseInterval));
        modulationChoice.addItem("LFO Phase Interval (inverted)", static_cast<int>(LfoModulatableParameter::lfoPhaseInterval_inverted));
        modulationChoice.addItem("LFO Current Phase", static_cast<int>(LfoModulatableParameter::lfoCurrentPhase));
        modulationChoice.addItem("LFO Current Phase (inverted)", static_cast<int>(LfoModulatableParameter::lfoCurrentPhase_inverted));
        modulationChoice.addItem("LFO Update Interval", static_cast<int>(LfoModulatableParameter::lfoUpdateInterval));
        modulationChoice.addItem("LFO Update Interval (inverted)", static_cast<int>(LfoModulatableParameter::lfoUpdateInterval_inverted));
        //modulationChoice.addSeparator();
        //modulationChoice.addSectionHeading("Experimental");
        modulationChoice.onChange = [this] { sendChangeMessage(); }; //the LfoEditor does the actualy routing to the RegionLfo. that way, this class doesn't need any references to RegionLfo or the LfoEditor or any of that stuff, which is cleaner overall
        modulationChoice.setEnabled(false);
        modulationChoice.setTooltip(getTooltip());
        addAndMakeVisible(modulationChoice);
    }

    ~CheckBoxListItem()
    {
        removeAllChangeListeners();
    }

    int getRow()
    {
        return row;
    }
    void setRow(int newRow)
    {
        row = newRow;
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(backgroundColour);
        g.fillAll();

        //ToggleButton::paint(g);
    }

    void resized() override
    {
        setBoundsInset(juce::BorderSize<int>(2));

        auto area = getLocalBounds();
        int unit = area.getWidth() / 6;

        regionButton.setBounds(area.removeFromLeft(unit * 2).reduced(1));
        modulationChoice.setBounds(area.reduced(1)); //unit
    }

    juce::Colour getBackgroundColour()
    {
        return backgroundColour;
    }
    void setBackgroundColour(juce::Colour newBackgroundColour)
    {
        backgroundColour = newBackgroundColour;

        regionButton.setColour(juce::ToggleButton::textColourId, backgroundColour.contrasting().withAlpha(1.0f));
        regionButton.setColour(juce::ToggleButton::tickColourId, backgroundColour.contrasting().withAlpha(1.0f));
        regionButton.setColour(juce::ToggleButton::tickDisabledColourId, backgroundColour.contrasting().withAlpha(0.5f));

        modulationChoice.setColour(juce::ComboBox::ColourIds::textColourId, backgroundColour.contrasting().withAlpha(1.0f));
        modulationChoice.setColour(juce::ComboBox::ColourIds::backgroundColourId, backgroundColour);
        modulationChoice.setColour(juce::ComboBox::ColourIds::arrowColourId, backgroundColour.contrasting(0.4f).withAlpha(1.0f));
    }

    juce::String getName()
    {
        return regionButton.getButtonText();
    }

    int getRegionID()
    {
        return regionID;
    }
    bool isModulated()
    {
        return regionButton.getToggleState();
    }
    void setIsModulated(bool shouldBeModulated, juce::NotificationType notification)
    {
        modulationChoice.setEnabled(shouldBeModulated);
        regionButton.setToggleState(shouldBeModulated, notification);
    }
    LfoModulatableParameter getModulatedParameter()
    {
        return static_cast<LfoModulatableParameter>(modulationChoice.getSelectedId());
    }
    void setModulatedParameterID(LfoModulatableParameter id, juce::NotificationType notification)
    {
        modulationChoice.setSelectedId(static_cast<int>(id), notification);
    }
    void randomiseModulatedParameterID()
    {
        //assign weights to each parameter. the higher the weight, the more likely that parameter is to get chosen. in general, parameters that tend to yield noisy results should be chosen less often.
        const int volumeWeight              = 10; //volume
        const int pitchWeight               = 8; //pitch
        const int playbackPosStartWeight    = 1; //playback position start
        const int playbackPosIntervalWeight = 2; //playback position interval
        const int playbackPosCurrentWeight  = 1; //playback position current
        const int filterPosWeight           = 10; //filter position
        const int lfoRateWeight             = 8; //LFO rate
        const int lfoPhaseStartWeight       = 7; //LFO phase start
        const int lfoPhaseIntervalWeight    = 5; //LFO phase interval
        const int lfoPhaseCurrentWeight     = 3; //LFO phase current
        const int lfoUpdateIntervalWeight   = 5; //LFO update interval
        const int totalWeight = volumeWeight + pitchWeight + playbackPosStartWeight + playbackPosIntervalWeight + playbackPosCurrentWeight + filterPosWeight + lfoRateWeight + lfoPhaseStartWeight + lfoPhaseIntervalWeight + lfoPhaseCurrentWeight + lfoUpdateIntervalWeight;

        juce::Random& rng = juce::Random::getSystemRandom();
        int choice = rng.nextInt(totalWeight);

        if (choice < volumeWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::volume, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::volume_inverted, juce::NotificationType::sendNotification);
        }
        else if ((choice -= volumeWeight) < pitchWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::pitch, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::pitch_inverted, juce::NotificationType::sendNotification);
        }
        else if ((choice -= pitchWeight) < playbackPosStartWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::playbackPositionStart, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::playbackPositionStart_inverted, juce::NotificationType::sendNotification);
        }
        else if ((choice -= playbackPosStartWeight) < playbackPosIntervalWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::playbackPositionInterval, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::playbackPositionInterval_inverted, juce::NotificationType::sendNotification);
        }
        else if ((choice -= playbackPosIntervalWeight) < playbackPosCurrentWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::playbackPositionCurrent, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::playbackPositionCurrent_inverted, juce::NotificationType::sendNotification);
        }
        else if ((choice -= playbackPosCurrentWeight) < filterPosWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::filterPosition, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::filterPosition_inverted, juce::NotificationType::sendNotification);
        }
        else if ((choice -= filterPosWeight) < lfoPhaseStartWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::lfoStartingPhase, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::lfoStartingPhase_inverted, juce::NotificationType::sendNotification);
        }
        else if ((choice -= lfoPhaseStartWeight) < lfoPhaseIntervalWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::lfoPhaseInterval, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::lfoPhaseInterval_inverted, juce::NotificationType::sendNotification);
        }
        else if ((choice -= lfoPhaseIntervalWeight) < lfoPhaseCurrentWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::lfoCurrentPhase, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::lfoCurrentPhase_inverted, juce::NotificationType::sendNotification);
        }
        else if ((choice -= lfoPhaseCurrentWeight) < lfoUpdateIntervalWeight)
        {
            if (rng.nextBool())
                setModulatedParameterID(LfoModulatableParameter::lfoUpdateInterval, juce::NotificationType::sendNotification);
            else
                setModulatedParameterID(LfoModulatableParameter::lfoUpdateInterval_inverted, juce::NotificationType::sendNotification);
        }
    }

    void triggerClick(const juce::MouseEvent& e)
    {
        auto pos = e.getMouseDownPosition();
        if (regionButton.getBounds().contains(pos))
        {
            regionButton.triggerClick();
        }
        else if (modulationChoice.getBounds().contains(pos))
        {
            modulationChoice.showPopup();
        }

        regionButton.setTooltip(getTooltip());
        modulationChoice.setTooltip(getTooltip());
    }

    juce::String getTooltip() override
    {
        if (regionButton.getToggleState() == true)
        {
            //region is checked
            switch (modulationChoice.getSelectedId())
            {
            case static_cast<int>(LfoModulatableParameter::volume):
            case static_cast<int>(LfoModulatableParameter::volume_inverted):
                return "You are currently modulating region " + juce::String(regionID) + "'s volume, i.e. it grows louder the longer the LFO line is (or shorter if inverted). This is a multiplicative parameter.";

            case static_cast<int>(LfoModulatableParameter::pitch):
            case static_cast<int>(LfoModulatableParameter::pitch_inverted):
                return "You are currently modulating region " + juce::String(regionID) + "'s pitch, i.e. its pitch rises the longer the LFO line is (or shorter if inverted). This is an additive parameter.";

            case static_cast<int>(LfoModulatableParameter::playbackPositionStart):
            case static_cast<int>(LfoModulatableParameter::playbackPositionStart_inverted):
                return "You are currently modulating region " + juce::String(regionID) + "'s playback starting position, i.e. the position that you will hear first when you play the region. The longer the LFO line is, the further back it pushes the start (or pulls it closer if inverted). This is an additive parameter.";

            case static_cast<int>(LfoModulatableParameter::playbackPositionInterval):
            case static_cast<int>(LfoModulatableParameter::playbackPositionInterval_inverted):
                return "You are currently modulating region " + juce::String(regionID) + "'s playback interval. This shortens the section of audio that will be played, resulting in longer sections the longer the LFO line is (or shorter if inverted). This is a multiplicative parameter.";

            case static_cast<int>(LfoModulatableParameter::playbackPositionCurrent):
            case static_cast<int>(LfoModulatableParameter::playbackPositionCurrent_inverted):
                return "You are currently modulating region " + juce::String(regionID) + "'s current playback position. Whenever this region's LFO updates, it will force the target region's playback point to a certain position (from which it will keep on playing). When the LFO line is longer, the position to which it gets set is shifted further back (or forward if inverted). Note: for a less clicky sound, the region's DAHDSR envelope gets retriggered whenever the current playback position changes. This is an additive parameter.";

            case static_cast<int>(LfoModulatableParameter::filterPosition):
            case static_cast<int>(LfoModulatableParameter::filterPosition_inverted):
                return "You are currently modulating region " + juce::String(regionID) + "'s filter position, i.e. its filter position rises the longer the LFO line is (or shorter if inverted). This is a multiplicative parameter.";




            case static_cast<int>(LfoModulatableParameter::lfoRate):
            case static_cast<int>(LfoModulatableParameter::lfoRate_inverted):
                return "You are currently modulating the speed of region " + juce::String(regionID) + "'s LFO, i.e. the longer this LFO's line is, the faster that other LFO becomes (or slower if inverted). This is an additive parameter.";

            case static_cast<int>(LfoModulatableParameter::lfoStartingPhase):
            case static_cast<int>(LfoModulatableParameter::lfoStartingPhase_inverted):
                return "You are currently modulating the starting phase of region " + juce::String(regionID) + "'s LFO. The phase is essentially the angle at which the LFO line starts out, the original starting position being where the first point was drawn. When the LFO line is longer, the starting line gets shifted further back (or forward if inverted). This is an additive parameter.";

            case static_cast<int>(LfoModulatableParameter::lfoPhaseInterval):
            case static_cast<int>(LfoModulatableParameter::lfoPhaseInterval_inverted):
                return "You are currently modulating the phase interval of region " + juce::String(regionID) + "'s LFO. This is done by multiplying the maximum angle, which the LFO could travel, with a factor between 0 and 1, i.e. the section that the line can travel becomes larger the longer this LFO's line is (or shorter if inverted). This is a multiplicative parameter.";

            case static_cast<int>(LfoModulatableParameter::lfoCurrentPhase):
            case static_cast<int>(LfoModulatableParameter::lfoCurrentPhase_inverted):
                return "You are currently modulating the current phase of region " + juce::String(regionID) + "'s LFO. Whenever this region's LFO updates, it will force the target region's LFO to a certain phase (from which the other LFO will then keep moving). When the LFO line is longer, the position to which it gets set is shifted further back (or forward if inverted). This is an additive parameter.";

            case static_cast<int>(LfoModulatableParameter::lfoUpdateInterval):
            case static_cast<int>(LfoModulatableParameter::lfoUpdateInterval_inverted):
                return "You are currently modulating the update rate of region " + juce::String(regionID) + "'s LFO, i.e. the longer this LFO's line is, the slower that other LFO's update rate becomes (or faster if inverted). Note: the time until an LFO's next update is only adjusted during *its own* update, not whenever LFOs modulating it change. This is a multiplicative parameter.";




            default:
                return "You can modulate this region by selecting a parameter in the combo box on the right.";
            }
        }
        else
        {
            //region is not checked
            return "You can modulate this region by putting a check mark on the left and then selecting a parameter in the combo box on the right.";
        }
    }

    void copyValues(CheckBoxListItem* otherItem)
    {
        row = otherItem->row;
        regionID = otherItem->regionID;
        setBackgroundColour(otherItem->backgroundColour);
        regionButton.setButtonText(otherItem->regionButton.getButtonText());
        setIsModulated(otherItem->isModulated(), juce::NotificationType::dontSendNotification);
        modulationChoice.setSelectedId(otherItem->modulationChoice.getSelectedId(), juce::NotificationType::dontSendNotification);
    }

private:
    int row;
    int regionID;

    juce::Colour backgroundColour;

    juce::ToggleButton regionButton;
    juce::ComboBox modulationChoice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CheckBoxListItem)
};

//==============================================================================
/*
* A ListBox containing items which are all CheckBoxes
*/
class CheckBoxList final : public juce::Component, public juce::ListBoxModel, juce::ChangeListener //, public juce::TooltipClient
{
public:
    CheckBoxList() : juce::ListBoxModel()
    {
        list.setModel(this);
        list.setClickingTogglesRowSelection(false);
        list.setRowSelectedOnMouseDown(false);
        addAndMakeVisible(list);

        setInterceptsMouseClicks(false, true);
    }

    ~CheckBoxList() override
    {
        while (items.size() > 0)
        {
            delete items[items.size() - 1];
            items.removeLast();
        }

        list.updateContent(); //deletes all children
    }

    int getNumRows() override
    {
        return items.size();
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        //needs to be overwritten, but isn't required here since only components are used
    }

    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override
    {
        if (existingComponentToUpdate == nullptr)
        {
            //create new custom component
            if (rowNumber < getNumRows())
            {
                CheckBoxListItem* item = new CheckBoxListItem("", 0, 0);
                item->addChangeListener(this);
                item->copyValues(items[rowNumber]);
                return item;
            }
        }
        else
        {
            CheckBoxListItem* existingItem = static_cast<CheckBoxListItem*>(existingComponentToUpdate);

            if (existingItem->getRow() != rowNumber)
            {
                //juce prefers to re-use previously used components over creating new ones.
                if (rowNumber < getNumRows())
                {
                    CheckBoxListItem* item = static_cast<CheckBoxListItem*>(existingItem); //items[rowNumber];
                    static_cast<CheckBoxListItem*>(existingItem)->copyValues(items[rowNumber]);
                    return existingItem;
                }
                else
                {
                    static_cast<CheckBoxListItem*>(existingComponentToUpdate)->removeChangeListener(this);
                    delete existingComponentToUpdate;
                }
            }
            else
            {
                //existing component, same row number
                if (rowNumber < getNumRows())
                {
                    CheckBoxListItem* item = static_cast<CheckBoxListItem*>(existingItem); //items[rowNumber];
                    static_cast<CheckBoxListItem*>(existingItem)->copyValues(items[rowNumber]); //WIP: this might not be necessary
                    return existingItem;
                }
                else
                {
                    //item does not exist anymore in the array -> has been deleted -> delete item
                    static_cast<CheckBoxListItem*>(existingComponentToUpdate)->removeChangeListener(this);
                    delete existingComponentToUpdate;
                }
            }
        }

        return nullptr;
    }

    juce::String getNameForRow(int rowNumber) override
    {
        if (rowNumber < getNumRows())
        {
            return items[rowNumber]->getName();
        }
        else
        {
            return "";
        }
    }

    void listBoxItemClicked(int row, const juce::MouseEvent& e) override
    {
        if (row < getNumRows())
        {
            items[row]->triggerClick(e);
        }
    }

    juce::String getTooltipForRow(int row) override
    {
        if (row < getNumRows())
        {
            return items[row]->getTooltip();
        }
    }

    void resized() override
    {
        list.setBounds(getLocalBounds());
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::createFromDescription("ctrl + r"))
        {
            auto mousePos = getMouseXYRelative();
            //juce::Component* target = getComponentAt(mousePos.getX(), mousePos.getY());
            /*auto listItems = list.getChildren();

            for (auto itItem = listItems.begin(); itItem != listItems.end(); ++itItem)
            {
                if ((*itItem)->getBounds().reduced(2).contains(mousePos))
                {
                    randomiseItem(static_cast<CheckBoxListItem*>(*itItem));
                    return true;
                }
            }*/

            int targetRow = list.getRowContainingPosition(mousePos.getX(), mousePos.getY());
            if (targetRow >= 0 && targetRow < items.size())
            {
                auto rowBounds = list.getRowPosition(targetRow, true);
                
                if (rowBounds.reduced(2).contains(mousePos)) //leave some inbetween spaces -> looks better and allows for randomisation of the list as a whole (when pressing ctrl+r in those inbetween spaces)
                {
                    randomiseItem(items[targetRow]);
                    return true;
                }
            }
        }

        return false;
    }

    int addItem(juce::String text, int regionID, juce::ChangeListener* changeListener)
    {
        auto* newItem = new CheckBoxListItem(text, getNumRows(), regionID);
        newItem->setInterceptsMouseClicks(false, false); //it's probably cleaner to handle clicks through listBoxItemClicked since mouse click interception would mess with item selection
        newItem->addChangeListener(changeListener);
        items.add(newItem);

        //list.updateContent();

        return newItem->getRow();
    }
    void changeListenerCallback(juce::ChangeBroadcaster* source)
    {
        auto* item = static_cast<CheckBoxListItem*>(source);

        items[item->getRow()]->copyValues(item);
        items[item->getRow()]->sendChangeMessage();
    }
    
    void clearItems()
    {        
        while (items.size() > 0)
        {
            delete items[items.size() - 1];
            items.removeLast();
        }

        list.updateContent(); //deletes all children

        DBG("cleared items of the CheckBoxList.");
    }

    void setRowBackgroundColour(int rowNumber, juce::Colour colour)
    {
        if (rowNumber < getNumRows())
        {
            items[rowNumber]->setBackgroundColour(colour);
        }
        //list.updateContent();
    }

    /*void setClickFunction(int rowNumber, std::function<void ()> clickFunction)
    {
        if (rowNumber < getNumRows())
        {
            items[rowNumber]->onClick = clickFunction;
        }
    }*/

    juce::Array<int> getCheckedRegionIDs()
    {
        juce::Array<int> checkedRegionIndices;
        
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            if ((*it)->isModulated()) //region checkbox checked?
            {
                checkedRegionIndices.add((*it)->getRegionID());
            }
        }

        return checkedRegionIndices;
    }
    //void setCheckedRegions(juce::Array<int> regionIDs)
    //{
    //    for (auto it = items.begin(); it != items.end(); ++it)
    //    {
    //        (*it)->setIsModulated(regionIDs.contains((*it)->getRegionID())); //checked if contained in regionIDs
    //    }
    //}
    void copyRegionModulations(juce::Array<int> modulatedRegionIDs, juce::Array<LfoModulatableParameter> modulatedParameterIDs)
    {
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            int index = modulatedRegionIDs.indexOf((*it)->getRegionID());
            if (index >= 0) //modulatedRegionIDs contains this region
            {
                (*it)->setIsModulated(true, juce::NotificationType::dontSendNotification);
                (*it)->setModulatedParameterID(modulatedParameterIDs[index], juce::NotificationType::dontSendNotification);
            }
            else
            {
                (*it)->setIsModulated(false, juce::NotificationType::dontSendNotification);
            }
        }

        //list.updateContent();
    }

    void randomiseAllParameters()
    {
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            randomiseItem(*it);
        }
    }
    void randomiseItem(CheckBoxListItem* item)
    {
        juce::Random& rng = juce::Random::getSystemRandom();

        bool wasModulated = item->isModulated();
        bool shouldBeModulated = rng.nextInt(3) < 2; //66% chance to become modulated
        if (wasModulated != shouldBeModulated)
        {
            item->setIsModulated(shouldBeModulated, shouldBeModulated ? juce::NotificationType::dontSendNotification : juce::NotificationType::sendNotification); //if modulated, it will send a notification during the next if case
        }

        if (item->isModulated())
        {
            //set to a random parameter (excluding "no modulation")
            item->randomiseModulatedParameterID(); //this method uses weights to ensure that noise-inducing parameters are chosen less often
        }

        list.updateContent();
    }

    void setUnitOfHeight(int newHUnit)
    {
        list.setRowHeight(newHUnit);
    }

    //juce::String getTooltip() override
    //{
    //    return "In this list, there is one entry per region (incl. this one). For each region, you can select one parameter. That parameter will then be modulated by this region by interpreting the line from the region's focus point to its outline as an LFO.";
    //}

private:
    juce::ListBox list;
    juce::Array<CheckBoxListItem*> items; //not an OwnedArray! the items are automatically deleted by the ListBox when it's deleted, so using an OwnedArray would cause access violations!

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CheckBoxList)
};
