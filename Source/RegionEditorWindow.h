/*
  ==============================================================================

    RegionEditorWindow.h
    Created: 8 Jul 2022 10:01:38am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "RegionEditor.h"

#include "SegmentedRegion.h"
class SegmentedRegion;

class RegionEditorWindow : public juce::DocumentWindow
{
    //much of this is a copy-paste of the MainWindow class

public:
    RegionEditorWindow(juce::String name, SegmentedRegion* region);

    void refreshEditor();

    void closeButtonPressed() override;

    /* Note: Be careful if you override any DocumentWindow methods - the base
       class uses a lot of them, so by overriding you might break its functionality.
       It's best to do all your work in your content component instead, but if
       you really have to override any DocumentWindow methods, make sure your
       subclass also calls the superclass's method.
    */

private:
    //juce::SharedResourcePointer<juce::TooltipWindow> tooltipWindow; //used for displaying tooltips. automatically initialised

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegionEditorWindow)
};
