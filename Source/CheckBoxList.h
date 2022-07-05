/*
  ==============================================================================

    CheckBoxList.h
    Created: 1 Jul 2022 10:00:41am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CheckBoxListItem : public juce::ToggleButton
{
public:
    CheckBoxListItem(juce::String text, int row, int regionID) : juce::ToggleButton(text)
    {
        this->row = row;
        this->regionID = regionID;
        backgroundColour = juce::Colours::transparentBlack;
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

        ToggleButton::paint(g);
    }

    void resized() override
    {
        setBoundsInset(juce::BorderSize<int>(2));
    }

    juce::Colour getBackgroundColour()
    {
        return backgroundColour;
    }
    void setBackgroundColour(juce::Colour newBackgroundColour)
    {
        backgroundColour = newBackgroundColour;
        setColour(textColourId, backgroundColour.contrasting().withAlpha(1.0f));
    }

    int getRegionID()
    {
        return regionID;
    }

private:
    int row;
    int regionID;
    juce::Colour backgroundColour;

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
        addAndMakeVisible(list);
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

                //if (isRowSelected)
                //    item->setColour(juce::ToggleButton::ColourIds::textColourId, juce::Colours::lightseagreen);
                //else
                    item->setColour(juce::ToggleButton::textColourId, juce::Colours::black);

                return item;
            }
        }
        else
        {
            //WIP: row might've changed

            if (rowNumber < getNumRows())
            {
                auto item = items[rowNumber];

                //if (isRowSelected)
                //    item->setColour(juce::ToggleButton::ColourIds::textColourId, juce::Colours::lightseagreen);
                //else
                    item->setColour(juce::ToggleButton::textColourId, juce::Colours::black);

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
            return items[rowNumber]->getButtonText();
        }
        else
        {
            return "";
        }
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        if (row < getNumRows())
        {
            items[row]->triggerClick();
        }
    }

    void resized() override
    {
        list.setBounds(getLocalBounds());
    }

    int addItem(juce::String text, int regionID)
    {
        auto* newItem = new CheckBoxListItem(text, getNumRows(), regionID);
        newItem->setInterceptsMouseClicks(false, false); //it's probably cleaner to handle clicks through listBoxItemClicked since mouse click interception would mess with item selection
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

    void setClickFunction(int rowNumber, std::function<void ()> clickFunction)
    {
        if (rowNumber < getNumRows())
        {
            items[rowNumber]->onClick = clickFunction;
        }
    }

    juce::Array<int> getCheckedRegionIDs()
    {
        juce::Array<int> checkedRegionIndices;
        
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            if ((*it)->getToggleState() == true) //checkbox checked?
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
            (*it)->setToggleState(regionIDs.contains((*it)->getRegionID()), juce::NotificationType::dontSendNotification); //checked if contained in regionIDs
        }
    }

private:
    juce::ListBox list;
    juce::Array<CheckBoxListItem*> items; //not an OwnedArray! the items are automatically deleted by the ListBox when it's deleted, so using an OwnedArray would cause access violations!

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CheckBoxList)
};
