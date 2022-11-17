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
    addAndMakeVisible(openPresetButton);
    savePresetButton.setButtonText("Save Preset");
    savePresetButton.onClick = [this] { showSavePresetDialogue(); };
    addAndMakeVisible(savePresetButton);
    
    //load image button
    addAndMakeVisible(openImageButton);
    openImageButton.setButtonText("Open Image");
    openImageButton.onClick = [this] { showOpenImageDialogue(); };

    //mode box
    addAndMakeVisible(modeBox);
    modeBox.addItem("Init", static_cast<int>(PluginEditorStateIndex::init));
    modeBox.addItem("Drawing Regions", static_cast<int>(PluginEditorStateIndex::drawingRegion));
    modeBox.addItem("Editing Regions", static_cast<int>(PluginEditorStateIndex::editingRegions));
    modeBox.addItem("Playing Regions", static_cast<int>(PluginEditorStateIndex::playingRegions));
    modeBox.addItem("Drawing Play Paths", static_cast<int>(PluginEditorStateIndex::drawingPlayPath));
    modeBox.addItem("Editing Play Paths", static_cast<int>(PluginEditorStateIndex::editingPlayPaths));
    modeBox.addItem("Playing Play Paths", static_cast<int>(PluginEditorStateIndex::playingPlayPaths));
    modeBox.onChange = [this] { updateState(); };
    addAndMakeVisible(modeLabel);
    modeLabel.setText("Mode: ", juce::NotificationType::dontSendNotification);
    modeLabel.attachToComponent(&modeBox, true);

    //segmentable image
    image.setImagePlacement(juce::RectanglePlacement::stretchToFit);
    addAndMakeVisible(image);
    addKeyListener(&image);

    //setResizable(true, true); //set during state changes
    setResizeLimits(100, 100, 4096, 2160); //maximum resolution: 4k
    setSize(600, 400);

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

void ImageINeDemoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    juce::Rectangle<int> presetArea = area.removeFromTop(20);
    int widthQuarter = presetArea.getWidth() / 4;
    openPresetButton.setBounds(presetArea.removeFromLeft(widthQuarter));
    savePresetButton.setBounds(presetArea.removeFromLeft(widthQuarter));

    //juce::Rectangle<int> midiArea = area.removeFromTop(20);
    //midiInputList.setBounds(midiArea.removeFromRight(2 * midiArea.getWidth() / 3));
    juce::Rectangle<int> midiArea = presetArea;
    midiInputList.setBounds(midiArea);

    juce::Rectangle<int> modeArea;
    juce::Rectangle<int> imageArea;
    switch (currentStateIndex)
    {
    case PluginEditorStateIndex::init:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));

        //openImageButton.setBounds(area.removeFromTop(20).reduced(2, 2));
        imageArea = juce::Rectangle<int>(area.getCentreX() - area.getWidth() / 3,
                                         area.getCentreY() - 12,
                                         2 * area.getWidth() / 3,
                                         24);
        openImageButton.setBounds(imageArea.reduced(2, 2));
        break;

    case PluginEditorStateIndex::drawingRegion:
    case PluginEditorStateIndex::editingRegions:
    case PluginEditorStateIndex::playingRegions:
    case PluginEditorStateIndex::drawingPlayPath:
    case PluginEditorStateIndex::editingPlayPaths:
    case PluginEditorStateIndex::playingPlayPaths:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));
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
    //image.setImage(juce::ImageFileFormat::loadFrom(juce::File("C:\\Users\\Aaron\\Desktop\\Programmierung\\GitHub\\ImageSegmentationTester\\Test Images\\Re_Legion_big+.jpg")));

    fc.reset(new juce::FileChooser("Choose an image to open...", juce::File(R"(C:\Users\Aaron\Desktop\Programmierung\GitHub\ImageSegmentationTester\Test Images)") /*juce::File::getCurrentWorkingDirectory()*/,
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
                    juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "ImageINe - Preset loaded.", "The preset has been loaded successfully.", this, nullptr);
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

void ImageINeDemoAudioProcessorEditor::updateState()
{
    if (modeBox.getSelectedId() != 0 && modeBox.getSelectedId() != static_cast<int>(currentStateIndex)) //check whether the state has actually changed
    {
        if (modeBox.getSelectedId() == static_cast<int>(PluginEditorStateIndex::init) &&            //when transitioning to init
            static_cast<int>(currentStateIndex) > static_cast<int>(PluginEditorStateIndex::init) && //from any state after init
            (image.hasAtLeastOneRegion() || image.hasAtLeastOnePlayPath())                          //while there are regions or play paths on the image
            )
        {
            //display warning prompt about deleting regions and/or play paths. allow the user to cancel the transition!

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
                "Are you sure that you would like to transition back to the initial state? All regions and play paths would be deleted!",
                this,
                cb);
        }
        else
        {
            transitionToState(static_cast<PluginEditorStateIndex>(modeBox.getSelectedId()));
        }
    }
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