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


class CheckBoxListItem : public juce::Component, public juce::ChangeBroadcaster
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
        addAndMakeVisible(regionButton);

        modulationChoice.addSectionHeading("Basic");
        modulationChoice.addItem("Volume", static_cast<int>(LfoModulatableParameter::volume));
        modulationChoice.addItem("Volume (inverted)", static_cast<int>(LfoModulatableParameter::volume_inverted));
        modulationChoice.addItem("Pitch", static_cast<int>(LfoModulatableParameter::pitch));
        modulationChoice.addItem("Pitch (inverted)", static_cast<int>(LfoModulatableParameter::pitch_inverted));
        //modulationChoice.addItem("Panning", static_cast<int>(LfoModulatableParameter::panning));
        //modulationChoice.addItem("Panning (inverted)", static_cast<int>(LfoModulatableParameter::panning_inverted));
        modulationChoice.addSeparator();
        modulationChoice.addSectionHeading("LFO");
        modulationChoice.addItem("LFO Rate", static_cast<int>(LfoModulatableParameter::lfoRate));
        modulationChoice.addItem("LFO Rate (inverted)", static_cast<int>(LfoModulatableParameter::lfoRate_inverted));
        modulationChoice.addSeparator();
        modulationChoice.addSectionHeading("Experimental");
        modulationChoice.addItem("Playback Position", static_cast<int>(LfoModulatableParameter::playbackPosition));
        modulationChoice.addItem("Playback Position (inverted)", static_cast<int>(LfoModulatableParameter::playbackPosition_inverted));
        modulationChoice.onChange = [this] { sendChangeMessage(); }; //the LfoEditor does the actualy routing to the RegionLfo. that way, this class doesn't need any references to RegionLfo or the LfoEditor or any of that stuff, which is cleaner overall
        modulationChoice.setEnabled(false);
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

        regionButton.setBounds(area.removeFromLeft(unit * 2));
        modulationChoice.setBounds(area); //unit
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
    void setIsModulated(bool shouldBeModulated)
    {
        regionButton.setToggleState(shouldBeModulated, juce::NotificationType::dontSendNotification);
        modulationChoice.setEnabled(shouldBeModulated);
    }
    LfoModulatableParameter getModulatedParameter()
    {
        return static_cast<LfoModulatableParameter>(modulationChoice.getSelectedId());
    }
    void setModulatedParameterID(LfoModulatableParameter id)
    {
        modulationChoice.setSelectedId(static_cast<int>(id), juce::NotificationType::dontSendNotification);
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
class CheckBoxList : public juce::Component, public juce::ListBoxModel
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
        items.clear();
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
            if (rowNumber < getNumRows())
            {
                auto item = items[rowNumber];


                return item;
            }
        }
        else
        {
            //WIP: row might've changed

            if (rowNumber < getNumRows())
            {
                auto item = items[rowNumber];

                return item;
            }
            else
            {
                //item does not exist anymore in the array -> has been deleted -> delete item
                delete existingComponentToUpdate;
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

    void resized() override
    {
        list.setBounds(getLocalBounds());
    }

    int addItem(juce::String text, int regionID, juce::ChangeListener* changeListener)
    {
        auto* newItem = new CheckBoxListItem(text, getNumRows(), regionID);
        newItem->setInterceptsMouseClicks(false, false); //it's probably cleaner to handle clicks through listBoxItemClicked since mouse click interception would mess with item selection
        newItem->addChangeListener(changeListener);
        items.add(newItem);

        list.updateContent();

        return newItem->getRow();
    }
    
    void clearItems()
    {
        items.clear();
        list.updateContent();
    }

    void setRowBackgroundColour(int rowNumber, juce::Colour colour)
    {
        if (rowNumber < getNumRows())
        {
            items[rowNumber]->setBackgroundColour(colour);
        }
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
    void setCheckedRegions(juce::Array<int> regionIDs)
    {
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            (*it)->setIsModulated(regionIDs.contains((*it)->getRegionID())); //checked if contained in regionIDs
        }
    }
    void copyRegionModulations(juce::Array<int> modulatedRegionIDs, juce::Array<LfoModulatableParameter> modulatedParameterIDs)
    {
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            int index = modulatedRegionIDs.indexOf((*it)->getRegionID());
            if (index >= 0) //modulatedRegionIDs contains this region
            {
                (*it)->setIsModulated(true);
                (*it)->setModulatedParameterID(modulatedParameterIDs[index]);
            }
            else
            {
                (*it)->setIsModulated(false);
            }

        }
    }

private:
    juce::ListBox list;
    juce::Array<CheckBoxListItem*> items; //not an OwnedArray! the items are automatically deleted by the ListBox when it's deleted, so using an OwnedArray would cause access violations!

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CheckBoxList)
};
