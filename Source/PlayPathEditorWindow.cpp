/*
  ==============================================================================

    PlayPathEditorWindow.cpp
    Created: 7 Nov 2022 9:07:01am
    Author:  Aaron

  ==============================================================================
*/

#include "PlayPathEditorWindow.h"

PlayPathEditorWindow::PlayPathEditorWindow(juce::String name, PlayPath* path)
    : DocumentWindow(name,
        juce::Desktop::getInstance().getDefaultLookAndFeel()
        .findColour(juce::ResizableWindow::backgroundColourId),
        DocumentWindow::minimiseButton | DocumentWindow::closeButton)
{
    setUsingNativeTitleBar(false);
    setContentOwned(new PlayPathEditor(path), true);

#if JUCE_IOS || JUCE_ANDROID
    setFullScreen(true);
#else
    setResizable(true, true);
    setResizeLimits(100, 200, 4096, 2160); //max resolution: 4k
    centreWithSize(400, 350);
#endif

    setVisible(true);
    setTitle("Play Path " + juce::String(path->getID()));

    addToDesktop();
}

void PlayPathEditorWindow::closeButtonPressed()
{
    delete this;
}
