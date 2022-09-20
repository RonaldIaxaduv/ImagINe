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
    modeBox.addItem("Init", static_cast<int>(SegmentableImageState::Init));
    modeBox.addItem("Drawing", static_cast<int>(SegmentableImageState::Drawing));
    modeBox.addItem("Editing", static_cast<int>(SegmentableImageState::Editing));
    modeBox.addItem("Playing", static_cast<int>(SegmentableImageState::Playing));
    modeBox.onChange = [this] { updateState(); };
    addAndMakeVisible(modeLabel);
    modeLabel.setText("Mode: ", juce::NotificationType::dontSendNotification);
    modeLabel.attachToComponent(&modeBox, true);

    image.setImagePlacement(juce::RectanglePlacement::stretchToFit);
    addAndMakeVisible(image);
    addKeyListener(&image);

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
    auto modeArea = area;

    switch (currentState)
    {
    case SegmentableImageState::Init:
        loadImageButton.setBounds(area.removeFromTop(20).reduced(2, 2));

        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));
        break;

    case SegmentableImageState::Drawing:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));

        image.setBounds(area.reduced(10, 10));
        break;

    case SegmentableImageState::Editing:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));

        image.setBounds(area.reduced(10, 10));
        break;

    case SegmentableImageState::Playing:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));

        image.setBounds(area.reduced(10, 10));
        break;

    default:
        break;
    }


    //^- WIP: make it so that the image's width-height-ratio is automatically preserved
    //(like when setting its ImagePlacement to centred, just with its actual bounds)
    //-> will make it a little nicer to look at
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
                modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Drawing), true);
                modeBox.setSelectedId(static_cast<int>(SegmentableImageState::Drawing));
                toFront(true);
            }
        },
        nullptr);
}

void ImageINeDemoAudioProcessorEditor::updateState()
{
    if (modeBox.getSelectedId() != 0 && modeBox.getSelectedId() != static_cast<int>(currentState)) //check whether the state has actually changed
    {
        currentState = static_cast<SegmentableImageState>(modeBox.getSelectedId());
        DBG("set MainComponent's current state to " + juce::String(static_cast<int>(currentState)));

        switch (currentState)
        {
        case SegmentableImageState::Init:
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Init), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Drawing), false);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Editing), false);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Playing), false);

            loadImageButton.setVisible(true);
            image.setVisible(false);

            image.setState(SegmentableImageState::Init);

            break;

        case SegmentableImageState::Drawing:
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Init), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Drawing), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Editing), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Playing), true);

            loadImageButton.setVisible(false);
            image.setVisible(true);

            image.setState(SegmentableImageState::Drawing);

            break;

        case SegmentableImageState::Editing:
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Init), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Drawing), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Editing), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Playing), true);

            loadImageButton.setVisible(false);
            image.setVisible(true);

            image.setState(SegmentableImageState::Editing);

            break;

        case SegmentableImageState::Playing:
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Init), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Drawing), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Editing), true);
            modeBox.setItemEnabled(static_cast<int>(SegmentableImageState::Playing), true);

            loadImageButton.setVisible(false);
            image.setVisible(true);

            image.setState(SegmentableImageState::Playing);

            break;

        default:
            throw new std::exception("Invalid state.");
        }

        resized();
    }
}
