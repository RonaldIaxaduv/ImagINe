/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ImageINeDemoAudioProcessorEditor::ImageINeDemoAudioProcessorEditor (ImageINeDemoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), image(*p.audioEngine.getImage())
{
    //code for the MIDI input box: see https://docs.juce.com/master/tutorial_synth_using_midi_input.html
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs Enabled");
    midiInputList.setTooltip("If you would like to play ImageINe using MIDI notes, you can select the MIDI input to listen to here.");
    addAndMakeVisible(midiInputList);

    juce::StringArray midiInputNames;
    for (auto input : midiInputs)
        midiInputNames.add(input.name);

    midiInputList.addItemList(midiInputNames, 1);
    midiInputList.onChange = [this] { setMidiInput(midiInputList.getSelectedItemIndex()); };

    for (auto input : midiInputs)
    {
        if (audioProcessor.deviceManager.isMidiInputDeviceEnabled(input.identifier))
        {
            setMidiInput(midiInputs.indexOf(input));
            break;
        }
    }

    if (midiInputList.getSelectedId() == 0)
        setMidiInput(0);

    midiInputListLabel.setText("MIDI Input:", juce::NotificationType::dontSendNotification);
    midiInputListLabel.attachToComponent(&midiInputList, true);
    addAndMakeVisible(midiInputListLabel);




    //preset buttons
    openPresetButton.setButtonText("Open Preset");
    openPresetButton.onClick = [this] { showOpenPresetDialogue(); };
    openPresetButton.setTooltip("Click this button to load a preset for ImageINe. Presets can contain a background image as well as regions (including their audio), play paths and all their settings and modulations.");
    addAndMakeVisible(openPresetButton);
    savePresetButton.setButtonText("Save Preset");
    savePresetButton.onClick = [this] { showSavePresetDialogue(); };
    savePresetButton.setTooltip("Click this button to save the current program state as a preset. The preset will contain the current background image as well as any regions (incl. their audio), play paths and all their settings and modulations.");
    addAndMakeVisible(savePresetButton);

    //information button
    informationButton.setButtonText("?");
    informationButton.onClick = [this] { displayModeInformation(); };
    informationButton.setTooltip("Click here to display some information about the current program mode, useful keybindings et cetera.");
    addAndMakeVisible(informationButton);
    
    //load image button
    openImageButton.setButtonText("Open Image");
    openImageButton.onClick = [this] { showOpenImageDialogue(); };
    openImageButton.setTooltip("Click this button to load an image into ImageINe. After doing so, you will be able to draw regions and play paths on it.");
    addAndMakeVisible(openImageButton);

    //mode box
    modeBox.addItem("Init", static_cast<int>(PluginEditorStateIndex::init));
    modeBox.addItem("Drawing Regions", static_cast<int>(PluginEditorStateIndex::drawingRegion));
    modeBox.addItem("Editing Regions", static_cast<int>(PluginEditorStateIndex::editingRegions));
    modeBox.addItem("Drawing Play Paths", static_cast<int>(PluginEditorStateIndex::drawingPlayPath));
    modeBox.addItem("Editing Play Paths", static_cast<int>(PluginEditorStateIndex::editingPlayPaths));
    modeBox.addItem("Playing Regions", static_cast<int>(PluginEditorStateIndex::playingRegions));
    modeBox.addItem("Playing Play Paths", static_cast<int>(PluginEditorStateIndex::playingPlayPaths));
    modeBox.onChange = [this] { updateState(); };
    addAndMakeVisible(modeLabel);
    modeLabel.setText("Mode: ", juce::NotificationType::dontSendNotification);
    modeLabel.attachToComponent(&modeBox, true);
    modeBox.setTooltip("Here you can select the current mode of the program. To go beyond Init, you first have to load an image. Afterwards, you can freely switch between states. Note especially that any editors remain open when switching to play mode, and any regions and play paths keep playing when switching to another play or editing mode!");
    addAndMakeVisible(modeBox);

    //panic button
    panicButton.setButtonText("PANIC!");
    panicButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::red);
    panicButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::darkred);
    panicButton.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colours::black);
    panicButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::white);
    panicButton.onClick = [this] { panic(); };
    panicButton.setTooltip("Clicking this button will immediately stop all regions and play paths without any release times. This is useful in case you accidentally run into some noisy program configuration et cetera.");
    addAndMakeVisible(panicButton);

    //segmentable image
    image.setImagePlacement(juce::RectanglePlacement::stretchToFit);
    addAndMakeVisible(image);
    addKeyListener(&image);

    //setResizable(true, true); //set during state changes
    setResizeLimits(100, 100, 4096, 2160); //maximum resolution: 4k
    setSize(600, 400);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);

    //tooltips
    tooltipWindow->setMillisecondsBeforeTipAppears(2000);

    //modeBox.setSelectedId(static_cast<int>(PluginEditorStateIndex::Init));
    setStateAccordingToImage();
}

ImageINeDemoAudioProcessorEditor::~ImageINeDemoAudioProcessorEditor()
{
    fc = nullptr;
  
    //removeKeyListener(imagePtr.get());
    removeMouseListener(&image);
    removeChildComponent(&image); //the image is not a member of the editor. it's contained in the AudioEngine, so it needs to be released.
}

//==============================================================================
void ImageINeDemoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    /*g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);*/
}
void ImageINeDemoAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);

    int textHeight = juce::jmax(2, juce::jmin(20, getHeight() / 100));
    int textWidth = getWidth(); //juce::jmax(100, juce::jmin(getWidth(), textHeight * 25));
    //float textHeight = juce::jmax(2.0f, juce::jmin(20.0f, static_cast<float>(getHeight()) / 100.0f));
    //float textWidth = juce::jmax(50.0f, juce::jmin(static_cast<float>(getWidth()) / 2.0f, textHeight * 25.0f));
    g.drawText("ImageINe V" + juce::String(JucePlugin_VersionString), juce::Rectangle<int>(0, getHeight() - textHeight, textWidth, textHeight), juce::Justification::bottomLeft, false);
    //g.drawFittedText("ImageINe V" + juce::String(JucePlugin_VersionString), juce::Rectangle<int>(0, getHeight() - textHeight, textWidth, textHeight), juce::Justification::bottomLeft, 1, 0.5f);
}

void ImageINeDemoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    juce::Rectangle<int> headerArea = area.removeFromTop(20);
    int widthQuarter = headerArea.getWidth() / 4;
    openPresetButton.setBounds(headerArea.removeFromLeft(widthQuarter).reduced(2));
    savePresetButton.setBounds(headerArea.removeFromLeft(widthQuarter).reduced(2));
    informationButton.setBounds(headerArea.removeFromRight(20).reduced(2));
    midiInputList.setBounds(headerArea.reduced(2));

    juce::Rectangle<int> modeArea;
    juce::Rectangle<int> imageArea;
    switch (currentStateIndex)
    {
    case PluginEditorStateIndex::init:
        modeArea = area.removeFromTop(20);
        modeArea.removeFromLeft(50);
        panicButton.setBounds(modeArea.removeFromRight(60).reduced(1));
        modeBox.setBounds(modeArea.reduced(1));

        //openImageButton.setBounds(area.removeFromTop(20).reduced(2, 2));
        imageArea = juce::Rectangle<int>(area.getCentreX() - area.getWidth() / 3,
                                         area.getCentreY() - 12,
                                         2 * area.getWidth() / 3,
                                         24);
        openImageButton.setBounds(imageArea.reduced(2));
        break;

    case PluginEditorStateIndex::drawingRegion:
    case PluginEditorStateIndex::editingRegions:
    case PluginEditorStateIndex::playingRegions:
    case PluginEditorStateIndex::drawingPlayPath:
    case PluginEditorStateIndex::editingPlayPaths:
    case PluginEditorStateIndex::playingPlayPaths:
        modeArea = area.removeFromTop(20);
        modeArea.removeFromLeft(50);
        panicButton.setBounds(modeArea.removeFromRight(60).reduced(1));
        modeBox.setBounds(modeArea.reduced(1));
        break;

    default:
        break; //don't throw an exception here! the code will sometimes get here when starting up the editor!
    }

    area = area.reduced(5, 5); //leave some space towards the sides

    auto origImgBounds = image.getImage().getBounds();

    if (origImgBounds.getWidth() == 0 || origImgBounds.getHeight() == 0)
    {
        //no image set yet
        image.setBounds(area);
        return;
    }

    float origImgAspect = static_cast<float>(origImgBounds.getWidth()) / static_cast<float>(origImgBounds.getHeight());
    float currentAspect = static_cast<float>(area.getWidth()) / static_cast<float>(area.getHeight());

    //preserve aspect ratio of the original image
    if (origImgAspect > currentAspect)
    {
        //image is narrower vertically than the window area
        image.setBounds(area.getX(),
                        area.getY() + static_cast<int>(0.5f * (static_cast<float>(area.getHeight()) - static_cast<float>(area.getWidth()) / origImgAspect)),
                        juce::jmax(1, area.getWidth()),
                        juce::jmax(1, static_cast<int>(static_cast<float>(area.getWidth()) / origImgAspect)));
    }
    else
    {
        //window area is narrower vertically than the image
        image.setBounds(area.getX() + static_cast<int>(0.5f * (static_cast<float>(area.getWidth()) - static_cast<float>(area.getHeight()) * origImgAspect)),
                        area.getY(),
                        juce::jmax(1, static_cast<int>(static_cast<float>(area.getHeight()) * origImgAspect)),
                        juce::jmax(1, area.getHeight()));
    }
}

bool ImageINeDemoAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    DBG("editor detected key press.");

    //state-specific key bindings:
    switch (currentStateIndex)
    {
    case PluginEditorStateIndex::init:
        if (key == juce::KeyPress::createFromDescription("ctrl + i"))
        {
            openImageButton.triggerClick();
            return true;
        }
    }

    //(largely) state-unrelated key-bindings:
    if (static_cast<int>(currentStateIndex) > static_cast<int>(PluginEditorStateIndex::init))
    {
        //switch state
        if (key == juce::KeyPress::createFromDescription("w") || key == juce::KeyPress::createFromDescription("a") || key == juce::KeyPress::upKey || key == juce::KeyPress::leftKey)
        {
            /*if (modeBox.getSelectedId() > 1)
            {
                modeBox.setSelectedId(modeBox.getSelectedId() - 1);
                return true;
            }*/
            if (modeBox.getSelectedItemIndex() > 0)
            {
                modeBox.setSelectedItemIndex(modeBox.getSelectedItemIndex() - 1);
            }
        }
        if (key == juce::KeyPress::createFromDescription("s") || key == juce::KeyPress::createFromDescription("d") || key == juce::KeyPress::downKey || key == juce::KeyPress::rightKey)
        {
            /*if (modeBox.getSelectedId() < static_cast<int>(PluginEditorStateIndex::StateIndexCount) - 1)
            {
                modeBox.setSelectedId(modeBox.getSelectedId() + 1);
                return true;
            }*/
            if (modeBox.getSelectedItemIndex() < modeBox.getNumItems() - 1)
            {
                modeBox.setSelectedItemIndex(modeBox.getSelectedItemIndex() + 1);
            }
        }

        for (int i = 0; i < modeBox.getNumItems(); ++i)
        {
            if (key == juce::KeyPress::createFromDescription(juce::String(i)))
            {
                //modeBox.setSelectedId(i + 1);
                modeBox.setSelectedItemIndex(i);
                return true;
            }
        }
    }

    if (key == juce::KeyPress::createFromDescription("ctrl + o"))
    {
        openPresetButton.triggerClick();
        return true;
    }
    else if (key == juce::KeyPress::createFromDescription("ctrl + s"))
    {
        savePresetButton.triggerClick();
        return true;
    }

    return false;
}

void ImageINeDemoAudioProcessorEditor::transitionToState(PluginEditorStateIndex stateToTransitionTo)
{
    currentStateIndex = stateToTransitionTo;
    DBG("set PluginEditor's current state to " + juce::String(static_cast<int>(currentStateIndex)));

    switch (currentStateIndex)
    {
    case PluginEditorStateIndex::init:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingRegion), false);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingRegions), false);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingRegions), false);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingPlayPath), false);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingPlayPaths), false);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingPlayPaths), false);

        openImageButton.setVisible(true);
        image.setVisible(false);
        image.transitionToState(SegmentableImageStateIndex::empty);

        setResizable(true, true);

        break;




    case PluginEditorStateIndex::drawingRegion:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingRegion), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingPlayPath), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingPlayPaths), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingPlayPaths), true);

        openImageButton.setVisible(false);
        image.setVisible(true);
        image.transitionToState(SegmentableImageStateIndex::drawingRegion);

        setResizable(true, true);

        break;

    case PluginEditorStateIndex::editingRegions:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingRegion), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingPlayPath), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingPlayPaths), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingPlayPaths), true);

        openImageButton.setVisible(false);
        image.setVisible(true);
        image.transitionToState(SegmentableImageStateIndex::editingRegions);

        setResizable(false, false);

        break;

    case PluginEditorStateIndex::playingRegions:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingRegion), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingPlayPath), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingPlayPaths), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingPlayPaths), true);

        openImageButton.setVisible(false);
        image.setVisible(true);
        image.transitionToState(SegmentableImageStateIndex::playingRegions);

        setResizable(false, false);

        break;




    case PluginEditorStateIndex::drawingPlayPath:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingRegion), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingPlayPath), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingPlayPaths), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingPlayPaths), true);

        openImageButton.setVisible(false);
        image.setVisible(true);
        image.transitionToState(SegmentableImageStateIndex::drawingPlayPath);

        setResizable(true, true);

        break;

    case PluginEditorStateIndex::editingPlayPaths:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingRegion), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingPlayPath), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingPlayPaths), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingPlayPaths), true);

        openImageButton.setVisible(false);
        image.setVisible(true);
        image.transitionToState(SegmentableImageStateIndex::editingPlayPaths);

        setResizable(false, false);

        break;

    case PluginEditorStateIndex::playingPlayPaths:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingRegion), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingRegions), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingPlayPath), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingPlayPaths), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingPlayPaths), true);

        openImageButton.setVisible(false);
        image.setVisible(true);
        image.transitionToState(SegmentableImageStateIndex::playingPlayPaths);

        setResizable(false, false);

        break;




    default:
        throw new std::exception("Invalid state.");
    }

    if (modeBox.getSelectedId() != static_cast<int>(currentStateIndex))
    {
        modeBox.setSelectedId(static_cast<int>(currentStateIndex), juce::NotificationType::dontSendNotification);
    }
    resized();
}
void ImageINeDemoAudioProcessorEditor::setStateAccordingToImage()
{
    //allow to transition to any state -> mode boxes all need to be activated!
    modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::init), true);
    modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingRegion), true);
    modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingRegions), true);
    modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingRegions), true);
    modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingPlayPath), true);
    modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::editingPlayPaths), true);
    modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::playingPlayPaths), true);

    switch (image.getCurrentStateIndex())
    {
    case SegmentableImageStateIndex::empty:
        transitionToState(PluginEditorStateIndex::init);
        break;




    case SegmentableImageStateIndex::withImage:
    case SegmentableImageStateIndex::drawingRegion:
        transitionToState(PluginEditorStateIndex::drawingRegion);
        break;

    case SegmentableImageStateIndex::editingRegions:
        transitionToState(PluginEditorStateIndex::editingRegions);
        break;

    case SegmentableImageStateIndex::playingRegions:
        transitionToState(PluginEditorStateIndex::playingRegions);
        break;




    case SegmentableImageStateIndex::drawingPlayPath:
        transitionToState(PluginEditorStateIndex::drawingPlayPath);
        break;

    case SegmentableImageStateIndex::editingPlayPaths:
        transitionToState(PluginEditorStateIndex::editingPlayPaths);
        break;

    case SegmentableImageStateIndex::playingPlayPaths:
        transitionToState(PluginEditorStateIndex::playingPlayPaths);
        break;




    default:
        throw std::exception("unhandled SegmentableImageStateIndex.");
    }
}

void ImageINeDemoAudioProcessorEditor::restorePreviousModeBoxSelection()
{
    modeBox.setSelectedId(static_cast<int>(currentStateIndex), juce::NotificationType::dontSendNotification);
}




void ImageINeDemoAudioProcessorEditor::showOpenImageDialogue()
{
    fc.reset(new juce::FileChooser("Choose an image to open...", juce::File(/*R"(C:\Users\Aaron\Desktop\Programmierung\GitHub\ImageSegmentationTester\Test Images)"*/) /*juce::File::getCurrentWorkingDirectory()*/,
        "*.jpg;*.jpeg;*.png;*.gif;*.bmp", true));

    fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            juce::String chosen;

            auto result = chooser.getURLResult();
            if (result.isLocalFile())
            {
                chosen = result.getLocalFile().getFullPathName();
                juce::Image img = juce::ImageFileFormat::loadFrom(result.getLocalFile());
                image.setImage(img);
                modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::drawingRegion), true);
                modeBox.setSelectedId(static_cast<int>(PluginEditorStateIndex::drawingRegion));
                toFront(true);
            }
        },
        nullptr);
}

void ImageINeDemoAudioProcessorEditor::showOpenPresetDialogue()
{
    fc.reset(new juce::FileChooser("Choose a preset to open...", juce::File(), "*.imageine_preset", false, false, nullptr));

    fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto result = chooser.getURLResult();
            if (result.isLocalFile())
            {
                juce::File chosenFile = result.getLocalFile();
                juce::MemoryBlock fileBlock;

                //load file to RAM
                bool shouldBeSuspended = processor.isSuspended();
                processor.suspendProcessing(true);
                chosenFile.loadFileAsData(fileBlock);

                //deserialise file
                try
                {
                    processor.setStateInformation(fileBlock.getData(), fileBlock.getSize());
                    juce::Timer::callAfterDelay(100, [this]
                        {
                            juce::Rectangle<int> prevBounds = image.getBounds();
                            image.setBounds(prevBounds.expanded(100, 100));
                            image.setBounds(prevBounds);
                        }); //hacky, but the LFO lines don't redraw properly otherwise. don't question it :x

                    if (currentStateIndex != PluginEditorStateIndex::init)
                    {
                        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "ImageINe - Preset loaded.", "The preset has been loaded successfully.", this, nullptr);
                    }
                    else
                    {
                        DBG("the editor has returned to its init state. this can happen if an error occurs while loading the preset.");
                    }
                    processor.suspendProcessing(shouldBeSuspended);
                }
                catch (std::exception ex)
                {
                    juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "ImageINe.", "The preset could not be loaded. Reason: " + juce::String(ex.what()), this, nullptr);
                    processor.suspendProcessing(shouldBeSuspended);
                }       
            }
        },
        nullptr);
}
void ImageINeDemoAudioProcessorEditor::showSavePresetDialogue()
{
    fc.reset(new juce::FileChooser("Choose a directory and name for the new preset...", juce::File(), "*.imageine_preset", false, false, nullptr));

    fc->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& chooser)
        {
            auto result = chooser.getURLResult();
            if (result.isLocalFile())
            {
                juce::File chosenFile = result.getLocalFile();
                
                //ensure correct file extension
                DBG("detected file extension: " + chosenFile.getFileExtension());
                if (chosenFile.getFileExtension().compareIgnoreCase(".imageine_preset") != 0)
                {
                    chosenFile = chosenFile.withFileExtension("imageine_preset");
                    DBG("new file extension: " + chosenFile.getFileExtension());
                }

                if (chosenFile.hasWriteAccess())
                {
                    //serialise current state
                    juce::MemoryBlock fileBlock;
                    processor.getStateInformation(fileBlock);

                    //save to file
                    bool shouldBeSuspended = processor.isSuspended();
                    processor.suspendProcessing(true);
                    try
                    {
                        chosenFile.replaceWithData(fileBlock.getData(), fileBlock.getSize());
                        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "ImageINe - Preset saved.", "The preset has been saved successfully.", this, nullptr);
                        processor.suspendProcessing(shouldBeSuspended);
                    }
                    catch (std::exception ex)
                    {
                        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "ImageINe.", "The preset could not be saved. Reason: " + juce::String(ex.what()), this, nullptr);
                        processor.suspendProcessing(shouldBeSuspended);
                    }
                }
            }
        },
        nullptr);
}

void ImageINeDemoAudioProcessorEditor::displayModeInformation()
{
    juce::String header = "";
    juce::String body = "";

    switch (currentStateIndex)
    {
    case PluginEditorStateIndex::init:
        header = "About: ImageINe - " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::init)));
        body = juce::String("Welcome to ImageINe!\n") +
               "In this menu, you will find information about each program mode including key bindings and so on. You can hover your cursor over any component to give you more information on what it does (note that there is a short delay before tooltips appear).\n\n" +

               "Before you can draw any regions or play paths, you first need to select a background image. You can do so by pressing the '" + openImageButton.getButtonText() + "' button. The original aspect ratio of the image will always be maintained even when you resize this window.\n\n" +

               "Key Bindings:\n" +
               "Ctrl + i: open image\n\n" +
               
               "Ctrl + o: open preset\n" +
               "Ctrl + s: save current state as preset";
        break;

    case PluginEditorStateIndex::drawingRegion:
        header = "About: ImageINe - " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::drawingRegion)));
        body = juce::String("To create a region, a path needs to be traced. You can do so by left-clicking the image, which will make points appear. The points will be connected in the order they were created. To finish a region, press 'o'. This will connect the last point you've created to the first. You need at least 3 points to create a region. When you're happy with the regions you've created, you can switch to " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::editingRegions))) + " to assign sounds to them.\n\n") +

               "Key Bindings:\n" +
               "o: complete region (because 'o' is a closed circle)\n" +
               "Backspace: delete last point\n" +
               "Esc: delete all currently drawn points\n" +
               "Del: delete the (completed) region that the cursor points at\n\n";

        for (int i = 0; i < modeBox.getNumItems(); ++i)
        {
            if (modeBox.getItemId(i) != static_cast<int>(PluginEditorStateIndex::drawingRegion))
            {
                body += juce::String(i) + ": switch to " + modeBox.getItemText(i) + "\n";
            }
        }
        body += juce::String("up/left/w/a: switch to previous mode\n") +
                "down/right/s/d: switch to next mode\n\n" +
                "Ctrl + o: open preset\n" +
                "Ctrl + s: save current state as preset";
        break;
        
    case PluginEditorStateIndex::editingRegions:
        header = "About: ImageINe - " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::editingRegions)));
        body = juce::String("To open a region's editor window, simply click on the region. In the editor, you can select the audio file that a region should play, and you have a range of other options, including how regions can influence each others' sounds through their LFO lines (the line going from the dot within the region towards its outline). As always, you can hover your cursor over any component to give you more information on what it does. Editors are resizable and will remain open when switching to playing mode to ensure a smoother workflow.\n\n") +

               "Key Bindings:\n" +
               "Del: delete the region that the cursor points at\n\n";

        for (int i = 0; i < modeBox.getNumItems(); ++i)
        {
            if (modeBox.getItemId(i) != static_cast<int>(PluginEditorStateIndex::editingRegions))
            {
                body += juce::String(i) + ": switch to " + modeBox.getItemText(i) + "\n";
            }
        }
        body += juce::String("up/left/w/a: switch to previous mode\n") +
                "down/right/s/d: switch to next mode\n\n" +
                "Ctrl + o: open preset\n" +
                "Ctrl + s: save current state as preset";
        break;

    case PluginEditorStateIndex::playingRegions:
        header = "About: ImageINe - " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::playingRegions)));
        body = juce::String("You can play a region by simply clicking on it. Depending on the region's settings, it may keep playing until you press it again. It may also respond to MIDI messages and to interactions with play paths. The region will play if *any* of the three cases is true and will only stop playing if *all* three conditions are no longer met.\n\n") +

               "Key Bindings:\n" +
               "p: play all regions (treated as clicks)\n" +
               "Alt + p: stop all regions (treated as clicks)\n" +
               "t: toggle all regions (treated as clicks)\n" +
               "Ctrl + p: play all play paths (treated as clicks)\n" +
               "Ctrl + Alt + p: stop all play paths (treated as clicks)\n" +
               "Ctrl + t: toggle all play paths (treated as clicks)\n\n";

        for (int i = 0; i < modeBox.getNumItems(); ++i)
        {
            if (modeBox.getItemId(i) != static_cast<int>(PluginEditorStateIndex::playingRegions))
            {
                body += juce::String(i) + ": switch to " + modeBox.getItemText(i) + "\n";
            }
        }
        body += juce::String("up/left/w/a: switch to previous mode\n") +
                "down/right/s/d: switch to next mode\n\n" +
                "Ctrl + o: open preset\n" +
                "Ctrl + s: save current state as preset";
        break;

    case PluginEditorStateIndex::drawingPlayPath:
        header = "About: ImageINe - " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::drawingPlayPath)));
        body = juce::String("To create a play path, its outline needs to be traced. You can do so, as with regions, by left-clicking the image, which will make points appear. The points will be connected in the order they were created. To finish a play path, press 'o'. This will connect the last point you've created to the first. You need at least 3 points to create a play path. When you're happy with the play paths you've created, you can switch to " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::editingPlayPaths))) + " to assign timings to them.\n\n") +

               "Key Bindings:\n" +
               "o: complete play path (because 'o' is a closed circle)\n" +
               "Backspace: delete last point\n" +
               "Esc: delete all currently drawn points\n" +
               "Del: delete the (completed) play path that the cursor points at\n\n";

        for (int i = 0; i < modeBox.getNumItems(); ++i)
        {
            if (modeBox.getItemId(i) != static_cast<int>(PluginEditorStateIndex::drawingPlayPath))
            {
                body += juce::String(i) + ": switch to " + modeBox.getItemText(i) + "\n";
            }
        }
        body += juce::String("up/left/w/a: switch to previous mode\n") +
                "down/right/s/d: switch to next mode\n\n" +
                "Ctrl + o: open preset\n" +
                "Ctrl + s: save current state as preset";
        break;

    case PluginEditorStateIndex::editingPlayPaths:
        header = "About: ImageINe - " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::editingPlayPaths)));
        body = juce::String("To open a play path's editor window, simply click on the play path. In the editor, you can select how fast the courier (the point which travels along the play path) moves. As always, you can hover your cursor over any component to give you more information on what it does. Editors are resizable and will remain open when switching to playing mode to ensure a smoother workflow.\n\n") +

               "Key Bindings:\n" +
               "Del: delete the play path that the cursor points at\n\n";

        for (int i = 0; i < modeBox.getNumItems(); ++i)
        {
            if (modeBox.getItemId(i) != static_cast<int>(PluginEditorStateIndex::editingPlayPaths))
            {
                body += juce::String(i) + ": switch to " + modeBox.getItemText(i) + "\n";
            }
        }
        body += juce::String("up/left/w/a: switch to previous mode\n") +
                "down/right/s/d: switch to next mode\n\n" +
                "Ctrl + o: open preset\n" +
                "Ctrl + s: save current state as preset";
        break;

    case PluginEditorStateIndex::playingPlayPaths:
        header = "About: ImageINe - " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::playingPlayPaths)));
        body = juce::String("You can play a play path by simply clicking on it. This will cause its courier (the point on it) to start moving according to the play path's settings. When the courier enters any region, that region will start playing, and when there are no longer any couriers within a region, it will stop playing. (There are some exceptions to this - see the " + modeBox.getItemText(modeBox.indexOfItemId(static_cast<int>(PluginEditorStateIndex::playingRegions))) + " info box for more information.)\n\n") +

               "Key Bindings:\n" +
               "p: play all play paths (treated as clicks)\n" +
               "Alt + p: stop all play paths (treated as clicks)\n" + 
               "t: toggle all play paths (treated as clicks)\n" +
               "Ctrl + p: play all regions (treated as clicks)\n" +
               "Ctrl + Alt + p: stop all regions (treated as clicks)\n" +
               "Ctrl + t: toggle all regions (treated as clicks)\n\n";

        for (int i = 0; i < modeBox.getNumItems(); ++i)
        {
            if (modeBox.getItemId(i) != static_cast<int>(PluginEditorStateIndex::playingPlayPaths))
            {
                body += juce::String(i) + ": switch to " + modeBox.getItemText(i) + "\n";
            }
        }
        body += juce::String("up/left/w/a: switch to previous mode\n") +
                "down/right/s/d: switch to next mode\n\n" +
                "Ctrl + o: open preset\n" +
                "Ctrl + s: save current state as preset";
        break;

    default:
        return;
    }

    body += juce::String("\n\n") + 
            "ImageINe was created by Aaron David Lux for his bachelor's thesis at the Friedrich Schiller Universitaet in Jena in 2022/2023. The thesis also includes a feedback survey for this program.\n" +
            "Feedback survey (Google Forms): https://forms.gle/d4r9Am5yja1J8myV6 \n" + //unshortened link: https://docs.google.com/forms/d/e/1FAIpQLSf9b_CzSsGovvz-4tWIfTTzq30DrUtfBDxkgTaGoAuCOHTLFA/viewform?usp=sf_link
            "ImageINe Discord server: https://discord.gg/NYW8bYmkEb";

    //juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, header, body, this);
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, header, body, "I see!", this);
}

void ImageINeDemoAudioProcessorEditor::updateState()
{
    if (modeBox.getSelectedId() != 0 && modeBox.getSelectedId() != static_cast<int>(currentStateIndex)) //check whether the state has actually changed
    {
        if (modeBox.getSelectedId() == static_cast<int>(PluginEditorStateIndex::init) &&                //when transitioning to init
            static_cast<int>(currentStateIndex) > static_cast<int>(PluginEditorStateIndex::init) &&     //from any state after init
            (image.hasAtLeastOneRegion() || image.hasAtLeastOnePlayPath() || image.hasStartedDrawing()) //while there are regions, play paths or drawn points on the image
            )
        {
            //display warning prompt about deleting regions, play paths and/or drawn points. allow the user to cancel the transition!

            //define message box and its callback
            void(*f)(int, ImageINeDemoAudioProcessorEditor*, PluginEditorStateIndex) = [](int result, ImageINeDemoAudioProcessorEditor* editor, PluginEditorStateIndex stateID)
            {
                if (result > 0)
                {
                    editor->transitionToState(stateID);
                }
                else
                {
                    editor->restorePreviousModeBoxSelection();
                }
            };
            juce::ModalComponentManager::Callback* cb = juce::ModalCallbackFunction::withParam(f, this, static_cast<PluginEditorStateIndex>(modeBox.getSelectedId()));

            auto result = juce::NativeMessageBox::showYesNoBox(juce::MessageBoxIconType::WarningIcon,
                "Transition back to Init?",
                "Are you sure that you would like to transition back to the initial state? All regions, play paths and drawn points would be deleted!",
                this,
                cb);
        }
        else if ((currentStateIndex == PluginEditorStateIndex::drawingRegion || currentStateIndex == PluginEditorStateIndex::drawingPlayPath) &&                                                    //when transitioning away from init
                 (modeBox.getSelectedId() != static_cast<int>(PluginEditorStateIndex::drawingRegion) && modeBox.getSelectedId() != static_cast<int>(PluginEditorStateIndex::drawingPlayPath)) &&
                 image.hasStartedDrawing()                                                                                                                                                          //while the user has already started drawing
                )
        {
            //display warning prompt about deleting drawn points. allow the user to cancel the transition!

            //define message box and its callback
            void(*f)(int, ImageINeDemoAudioProcessorEditor*, PluginEditorStateIndex) = [](int result, ImageINeDemoAudioProcessorEditor* editor, PluginEditorStateIndex stateID)
            {
                if (result > 0)
                {
                    editor->transitionToState(stateID);
                }
                else
                {
                    editor->restorePreviousModeBoxSelection();
                }
            };
            juce::ModalComponentManager::Callback* cb = juce::ModalCallbackFunction::withParam(f, this, static_cast<PluginEditorStateIndex>(modeBox.getSelectedId()));

            auto result = juce::NativeMessageBox::showYesNoBox(juce::MessageBoxIconType::WarningIcon,
                "Exit Drawing Mode?",
                "Are you sure that you would like to exit drawing mode? All currently drawn points would be deleted!",
                this,
                cb);
        }
        else
        {
            transitionToState(static_cast<PluginEditorStateIndex>(modeBox.getSelectedId()));
        }
    }
}

void ImageINeDemoAudioProcessorEditor::panic()
{
    DBG("PANIC! (PluginEditor)");
    image.panic();
    audioProcessor.audioEngine.panic();
}

void ImageINeDemoAudioProcessorEditor::setMidiInput(int index)
{
    auto list = juce::MidiInput::getAvailableDevices();

    audioProcessor.deviceManager.removeMidiInputDeviceCallback(list[lastInputIndex].identifier, &audioProcessor.midiCollector);

    auto newInput = list[index];

    if (!audioProcessor.deviceManager.isMidiInputDeviceEnabled(newInput.identifier))
        audioProcessor.deviceManager.setMidiInputDeviceEnabled(newInput.identifier, true);

    audioProcessor.deviceManager.addMidiInputDeviceCallback(newInput.identifier, &audioProcessor.midiCollector);
    midiInputList.setSelectedId(index + 1, juce::dontSendNotification);

    lastInputIndex = index;
}