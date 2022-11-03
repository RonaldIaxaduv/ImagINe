/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ImageINeDemoAudioProcessorEditor::ImageINeDemoAudioProcessorEditor (ImageINeDemoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), image(&p.audioEngine)
{
    addAndMakeVisible(loadImageButton);
    loadImageButton.setButtonText("Load Image");
    loadImageButton.onClick = [this] { showLoadImageDialogue(); };

    addAndMakeVisible(modeBox);
    modeBox.addItem("Init", static_cast<int>(PluginEditorStateIndex::Init));
    modeBox.addItem("Drawing", static_cast<int>(PluginEditorStateIndex::Drawing));
    modeBox.addItem("Editing", static_cast<int>(PluginEditorStateIndex::Editing));
    modeBox.addItem("Playing", static_cast<int>(PluginEditorStateIndex::Playing));
    modeBox.onChange = [this] { updateState(); };
    addAndMakeVisible(modeLabel);
    modeLabel.setText("Mode: ", juce::NotificationType::dontSendNotification);
    modeLabel.attachToComponent(&modeBox, true);

    image.setImagePlacement(juce::RectanglePlacement::stretchToFit);
    addAndMakeVisible(image);
    addKeyListener(&image);

    //setResizable(true, true); //set during state changes
    setResizeLimits(100, 100, 4096, 2160); //maximum resolution: 4k
    setSize(600, 400);

    modeBox.setSelectedId(1); //set state to init
}

ImageINeDemoAudioProcessorEditor::~ImageINeDemoAudioProcessorEditor()
{
    fc = nullptr;
    //removeKeyListener(imagePtr.get());
    removeMouseListener(&image);
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
    juce::Rectangle<int> modeArea;

    switch (currentStateIndex)
    {
    case PluginEditorStateIndex::Init:
        loadImageButton.setBounds(area.removeFromTop(20).reduced(2, 2));

        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));
        break;

    case PluginEditorStateIndex::Drawing:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));
        break;

    case PluginEditorStateIndex::Editing:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));
        break;

    case PluginEditorStateIndex::Playing:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));
        break;

    default:
        break;
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
    case PluginEditorStateIndex::Init:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Drawing), false);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Editing), false);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Playing), false);

        loadImageButton.setVisible(true);
        image.setVisible(false);
        image.transitionToState(SegmentableImageStateIndex::empty);

        setResizable(true, true);

        break;

    case PluginEditorStateIndex::Drawing:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Drawing), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Editing), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Playing), true);

        loadImageButton.setVisible(false);
        image.setVisible(true);
        image.transitionToState(SegmentableImageStateIndex::withImage); //if its current state was drawingRegion, it will remain at that state

        setResizable(true, true);

        break;

    case PluginEditorStateIndex::Editing:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Drawing), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Editing), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Playing), true);

        loadImageButton.setVisible(false);
        image.setVisible(true);
        image.transitionToState(SegmentableImageStateIndex::editingRegions);

        setResizable(false, false);

        break;

    case PluginEditorStateIndex::Playing:
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Init), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Drawing), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Editing), true);
        modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Playing), true);

        loadImageButton.setVisible(false);
        image.setVisible(true);
        image.transitionToState(SegmentableImageStateIndex::playingRegions);

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

bool ImageINeDemoAudioProcessorEditor::serialise(juce::XmlElement* xmlProcessor, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("serialising PluginEditor...");
    bool serialisationSuccessful = true;

    juce::XmlElement* xmlEditor = xmlProcessor->createNewChildElement("PluginEditor");

    //define state that the editor should start up with
    PluginEditorStateIndex targetStateIndex = PluginEditorStateIndex::Null;
    if (image.getImage().isNull())
    {
        targetStateIndex = PluginEditorStateIndex::Init;
    }
    else
    {
        //valid image has been set
        if (!image.hasAtLeastOneRegion())
        {
            targetStateIndex = PluginEditorStateIndex::Drawing;
        }
        else
        {
            //at least one region has been set
            if (!image.hasAtLeastOneRegionWithAudio())
            {
                targetStateIndex = PluginEditorStateIndex::Editing;
            }
            else
            {
                //at least one region has audio
                targetStateIndex = PluginEditorStateIndex::Playing;
            }
        }
    }
    xmlEditor->setAttribute("targetStateIndex", static_cast<int>(targetStateIndex));

    serialisationSuccessful = image.serialise(xmlEditor, attachedData); //stores image and all its regions

    DBG(juce::String(serialisationSuccessful ? "PluginEditor has been serialised." : "PluginEditor could not be serialised."));
    return serialisationSuccessful;
}
bool ImageINeDemoAudioProcessorEditor::deserialise(juce::XmlElement* xmlProcessor, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("deserialising PluginEditor...");
    bool deserialisationSuccessful = true;

    juce::XmlElement* xmlEditor = xmlProcessor->getChildByName("PluginEditor");

    if (xmlEditor != nullptr)
    {
        deserialisationSuccessful = image.deserialise(xmlEditor, attachedData); //restores image and all its regions

        if (deserialisationSuccessful)
        {
            transitionToState(static_cast<PluginEditorStateIndex>(xmlEditor->getIntAttribute("targetStateIndex", static_cast<int>(PluginEditorStateIndex::Init)))); //transition to a fitting state
        }
        else
        {
            transitionToState(PluginEditorStateIndex::Init);
        }
    }
    else
    {
        DBG("no PluginEditor data found.");
        deserialisationSuccessful = false;
        transitionToState(PluginEditorStateIndex::Init);
    }

    DBG(juce::String(deserialisationSuccessful ? "PluginEditor has been deserialised." : "PluginEditor could not be deserialised."));
    return deserialisationSuccessful;
}




void ImageINeDemoAudioProcessorEditor::showLoadImageDialogue()
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
                modeBox.setItemEnabled(static_cast<int>(PluginEditorStateIndex::Drawing), true);
                modeBox.setSelectedId(static_cast<int>(PluginEditorStateIndex::Drawing));
                toFront(true);
            }
        },
        nullptr);
}

void ImageINeDemoAudioProcessorEditor::updateState()
{
    if (modeBox.getSelectedId() != 0 && modeBox.getSelectedId() != static_cast<int>(currentStateIndex)) //check whether the state has actually changed
    {
        transitionToState(static_cast<PluginEditorStateIndex>(modeBox.getSelectedId()));
    }
}
