/*
  ==============================================================================

    PlayPathEditorWindow.h
    Created: 7 Nov 2022 9:07:01am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PlayPathEditor.h"

#include "PlayPath.h"
class PlayPath;

class PlayPathEditorWindow final : public juce::DocumentWindow
{
public:
    PlayPathEditorWindow(juce::String name, PlayPath* path);

    void closeButtonPressed() override;

    /* Note: Be careful if you override any DocumentWindow methods - the base
       class uses a lot of them, so by overriding you might break its functionality.
       It's best to do all your work in your content component instead, but if
       you really have to override any DocumentWindow methods, make sure your
       subclass also calls the superclass's method.
    */

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayPathEditorWindow)
};