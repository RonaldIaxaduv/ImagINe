/*
  ==============================================================================

    RegionEditorWindow.cpp
    Created: 8 Jul 2022 10:01:38am
    Author:  Aaron

  ==============================================================================
*/

#include "RegionEditorWindow.h"

RegionEditorWindow::RegionEditorWindow(juce::String name, SegmentedRegion* region)
    : DocumentWindow(name,
        juce::Desktop::getInstance().getDefaultLookAndFeel()
        .findColour(juce::ResizableWindow::backgroundColourId),
        DocumentWindow::minimiseButton | DocumentWindow::closeButton)
{
    setUsingNativeTitleBar(false);
    setContentOwned(new RegionEditor(region), true);

#if JUCE_IOS || JUCE_ANDROID
    setFullScreen(true);
#else
    setResizable(true, true);
    setResizeLimits(100, 200, 4096, 2160); //max resolution: 4k
    centreWithSize(400, 700);
#endif

    setVisible(true);
    setTitle("Region " + juce::String(region->getID()));

    //addKeyListener(static_cast<RegionEditor*>(getContentComponent()));
    
    addToDesktop();
}

void RegionEditorWindow::refreshEditor()
{
    static_cast<RegionEditor*>(getContentComponent())->refreshParameters();
}

void RegionEditorWindow::closeButtonPressed() 
{
    //setVisible(false);
    //this->removeFromDesktop();

    //removeKeyListener(static_cast<RegionEditor*>(getContentComponent()));

    giveAwayKeyboardFocus();

    delete this;
}
