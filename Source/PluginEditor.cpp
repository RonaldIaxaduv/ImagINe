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
    modeBox.addItem("Init", State::Init);
    modeBox.addItem("Drawing", State::Drawing);
    modeBox.addItem("Editing", State::Editing);
    modeBox.addItem("Playing", State::Playing);
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
    case State::Init:
        loadImageButton.setBounds(area.removeFromTop(20).reduced(2, 2));

        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));
        break;

    case State::Drawing:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));

        image.setBounds(area.reduced(10, 10));
        break;

    case State::Editing:
        modeArea = area.removeFromTop(20);
        modeLabel.setBounds(modeArea.removeFromLeft(50).reduced(2, 2));
        modeBox.setBounds(modeArea.reduced(2, 2));

        image.setBounds(area.reduced(10, 10));
        break;

    case State::Playing:
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

    fc.reset(new juce::FileChooser("Choose an image to open...", juce::File::getCurrentWorkingDirectory(),
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
                modeBox.setItemEnabled(State::Drawing, true);
                modeBox.setSelectedId(State::Drawing);
                toFront(true);
            }
        },
        nullptr);
}

void ImageINeDemoAudioProcessorEditor::updateState()
{
    if (modeBox.getSelectedId() != 0 && modeBox.getSelectedId() != currentState) //check whether the state has actually changed
    {
        currentState = (State)modeBox.getSelectedId();
        DBG("set MainComponent's current state to " + juce::String(currentState));

        switch (currentState)
        {
        case State::Init:
            modeBox.setItemEnabled(State::Init, true);
            modeBox.setItemEnabled(State::Drawing, false);
            modeBox.setItemEnabled(State::Editing, false);
            modeBox.setItemEnabled(State::Playing, false);

            loadImageButton.setVisible(true);
            image.setVisible(false);

            image.setState(SegmentableImage::State::Init);

            break;

        case State::Drawing:
            modeBox.setItemEnabled(State::Init, true);
            modeBox.setItemEnabled(State::Drawing, true);
            modeBox.setItemEnabled(State::Editing, true);
            modeBox.setItemEnabled(State::Playing, true);

            loadImageButton.setVisible(false);
            image.setVisible(true);

            image.setState(SegmentableImage::State::Drawing);

            break;

        case State::Editing:
            modeBox.setItemEnabled(State::Init, true);
            modeBox.setItemEnabled(State::Drawing, true);
            modeBox.setItemEnabled(State::Editing, true);
            modeBox.setItemEnabled(State::Playing, true);

            loadImageButton.setVisible(false);
            image.setVisible(true);

            image.setState(SegmentableImage::State::Editing);

            break;

        case State::Playing:
            modeBox.setItemEnabled(State::Init, true);
            modeBox.setItemEnabled(State::Drawing, true);
            modeBox.setItemEnabled(State::Editing, true);
            modeBox.setItemEnabled(State::Playing, true);

            loadImageButton.setVisible(false);
            image.setVisible(true);

            image.setState(SegmentableImage::State::Playing);

            break;

        default:
            throw new std::exception("Invalid state.");
        }

        resized();
    }
}
