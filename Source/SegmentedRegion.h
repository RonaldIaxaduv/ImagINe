/*
  ==============================================================================

    SegmentedRegion.h
    Created: 9 Jun 2022 9:03:01am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AudioEngine.h"
#include "Voice.h"
//#include "RegionLfo.h"
//#include "LfoEditor.h"
#include "CheckBoxList.h"

//==============================================================================
/*
*/
class SegmentedRegion : public juce::DrawableButton, juce::Timer
{
public:
    SegmentedRegion(const juce::Path& outline, const juce::Rectangle<float>& relativeBounds, AudioEngine* audioEngine) :
        juce::DrawableButton("", ButtonStyle::ImageStretched),
        p(outline),
        relativeBounds(relativeBounds)
    {
        regionEditorWindow = nullptr;

        initialiseImages();

        this->audioEngine = audioEngine;
        ID = audioEngine->addNewRegion(fillColour);

        //setButtonStyle(ButtonStyle::ImageStretched);

        //default: set focus in the centre of the shape
        focus = p.getBounds().getCentre();
        focus.setXY(focus.getX() / p.getBounds().getWidth(), focus.getY() / p.getBounds().getHeight()); //relative centre
        
        //LFO
        renderLfoWaveform(); //also initialises LFO

        /*this->onStateChange += [this] { handleButtonStateChanged(); };*/

        setBuffer(juce::AudioSampleBuffer(), "", 0.0); //empty buffer
        DBG(juce::String(p.getLength()));
    }

    ~SegmentedRegion() override
    {
        DBG("destroying SegmentedRegion");

        //WIP maybe the close button of the editor window will have to be pressed before dropping the pointer
        if (regionEditorWindow)
        {
            DBG("RegionEditorWindow still alive -> deleting");
            regionEditorWindow.deleteAndZero();
        }

        //release LFO
        /*if (associatedLfo != nullptr && lfoIndex >= 0)
            audioEngine->lfosim.remove(lfoIndex, true);*/
        audioEngine->removeLfo(getID());
        associatedLfo = nullptr;

        //release Voice(s)
        /*if (associatedVoice != nullptr && voiceIndex >= 0)
            audioEngine->removeVoice(voiceIndex);*/
        audioEngine->removeVoicesWithID(getID());
        associatedVoice = nullptr;

        //release audio engine
        audioEngine = nullptr;

        //~DrawableButton(); //this caused dangling pointers in the past -> try not to call if at all possible!
    }

    void initialiseImages()
    {
        setSize(static_cast<int>(500.0f * relativeBounds.getWidth()),
                static_cast<int>(500.0f * relativeBounds.getHeight())); //must be set before the images are drawn, because drawables require bounds to be displayed! 
        
        DBG("initial size of segmented region: " + getBounds().toString());
        DBG("bounds of the passed path: " + p.getBounds().toString());

        juce::Random rng;
        fillColour = juce::Colour::fromHSV(rng.nextFloat(), 0.6f + 0.4f * rng.nextFloat(), 0.6f + 0.4f * rng.nextFloat(), 1.0f); //juce::Colours::maroon;

        normalImage.setPath(p);
        normalImage.setFill(juce::FillType(fillColour));
        normalImage.setBounds(getBounds());

        overImage.setPath(p);
        overImage.setFill(juce::FillType(fillColour.brighter(0.2f)));
        overImage.setBounds(getBounds());

        downImage.setPath(p);
        downImage.setFill(juce::FillType(fillColour.darker(0.2f)));
        downImage.setBounds(getBounds());

        disabledImage.setPath(p);
        disabledImage.setFill(juce::FillType(fillColour.withAlpha(0.5f)));
        disabledImage.setBounds(getBounds());


        normalImageOn.setPath(p);
        normalImageOn.setFill(juce::FillType(fillColour.darker(0.4f)));
        normalImageOn.setBounds(getBounds());

        overImageOn.setPath(p);
        overImageOn.setFill(juce::FillType(fillColour.darker(0.2f)));
        overImageOn.setBounds(getBounds());

        downImageOn.setPath(p);
        downImageOn.setFill(juce::FillType(fillColour.darker(0.6f)));
        downImageOn.setBounds(getBounds());

        disabledImageOn.setPath(p);
        disabledImageOn.setFill(juce::FillType(fillColour.darker(0.4f).withAlpha(0.5f)));
        disabledImageOn.setBounds(getBounds());

        setImages(&normalImage, &overImage, &downImage, &disabledImage, &normalImageOn, &overImageOn, &downImageOn, &disabledImageOn);

        setColour(ColourIds::backgroundOnColourId, juce::Colours::transparentBlack); //otherwise, the button has a grey background colour while the button is toggled on
    }

    void timerCallback() override
    {
        repaint(); //could the redrawn area be reduced?
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        //if the normal paint method were used, the button images would be in front of whatever is drawn there.
        //with this method, everything is drawn in front of the button images.

        juce::Point<float> focusPt(focus.x * getBounds().getWidth(),
                                   focus.y * getBounds().getHeight());

        //draw focus point
        g.setColour(fillColour.contrasting());
        float focusRadius = 2.5f;
        g.fillEllipse(focusPt.x - focusRadius,
                      focusPt.y - focusRadius,
                      2 * focusRadius,
                      2 * focusRadius);

        if (associatedLfo != nullptr)
        {
            //draw LFO line
            float curLfoPhase = associatedLfo->getPhase();

            if (isPlaying)
                g.setColour(fillColour.brighter());
            else
                g.setColour(fillColour.brighter().withAlpha(0.5f)); //faded when not playing

            //draw line from focus point to point on the outline that corresponds to the associated LFO's current phase
            juce::Point<float> outlinePt = p.getPointAlongPath(curLfoPhase * p.getLength(), juce::AffineTransform(), juce::Path::defaultToleranceForMeasurement);
            g.drawLine(focusPt.x, focusPt.y,
                       outlinePt.x, outlinePt.y,
                       3.0f);
        }
    }

    void resized() override //WIP: for some reason, this is called when hovering over the button
    {
        //recalculate hitbox
        p.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), false);

        juce::DrawableButton::resized();
    }

    bool hitTest(int x, int y) override
    {
        return p.contains((float)x, (float)y);
    }

    enum State
    {
        Null,
        Init, //all components have been initialised
        Standby, //shape cannot be clicked yet
        Editing, //region is ready to be edited
        Playable
    };
    void setState(State newState)
    {
        if (currentState != newState)
        {
            currentState = newState; //the state mainly changes how the click method behaves
            DBG("set SegmentedRegion's current state to " + juce::String(currentState));

            //WIP: when switching to Playable, preparing the audio will probably be necessary

            //resized();
        }
    }

    void clicked(const juce::ModifierKeys& modifiers) override
    {
        juce::Button::buttonStateChanged();

        //clicked(modifiers); //base class stuff

        //modifiers.isRightButtonDown() -> differentiate between left and right mouse button like this

        switch (currentState)
        {
        case State::Standby:
            DBG("*stands by*");
            break;

        case State::Editing:
            DBG("*shows editor window*");
            if (regionEditorWindow != nullptr)
            {
                regionEditorWindow->toFront(true);
            }
            else
            {
                regionEditorWindow = juce::Component::SafePointer<RegionEditorWindow>(new RegionEditorWindow("Region Editor", this));
            }
            break;

        case State::Playable:
            //handled though buttonStateChanged
            break;
        }
    }

    void setBuffer(juce::AudioSampleBuffer newBuffer, juce::String fileName, double origSampleRate)
    {
        if (audioFileName != "")
        {
            //delete old voice
            /*if (voiceIndex >= 0)
                audioEngine->removeVoice(voiceIndex);*/
            audioEngine->removeVoicesWithID(getID());
            associatedVoice = nullptr;
        }

        buffer = newBuffer;
        audioFileName = fileName;

        if (audioFileName != "")
        {
            associatedVoice = new Voice(newBuffer, origSampleRate, getID());
            if (associatedLfo != nullptr)
            {
                //make LFO modulate that voice
                //std::function<void(float lfoValue)> newModulationFunction = [this](float lfoValue) { associatedVoice->modulateLevel(static_cast<double>(lfoValue)); };
                //associatedLfo->setModulationFunction(newModulationFunction);

                //associatedVoice->setLfo(associatedLfo);
            }
            auto voiceIndex = audioEngine->addVoice(associatedVoice);
            DBG("successfully added voice #" + juce::String(voiceIndex) + ". associated region: " + juce::String(getID()));

        }
    }

    void renderLfoWaveform()
    {
        DBG("rendering LFO's waveform...");

        juce::AudioBuffer<float> waveform(1, juce::jmax<int>(2, (int)p.getLength()) + 1); //minimum size of 2 samples (should always be the case since a minimum of 3 points are necessary to define a region)
        auto samples = waveform.getWritePointer(0);

        //the for-loop will iterate from 0.0 to the end point (length) of the path.
        //to get the desired number of samples, the path must be divided into sections with the following distance:
        float stepDistance = p.getLength() / ((float)(waveform.getNumSamples() - 2));

        for (int i = 0; i < waveform.getNumSamples() - 1; ++i)
        {
            samples[i] = p.getPointAlongPath((float)i * stepDistance, //this will usually just iterate between each pixel
                                             juce::AffineTransform(), //no transform
                                             juce::Path::defaultToleranceForMeasurement //?
                                            ).getDistanceSquaredFrom(focus); //use distance from focus point to create a 1D value
        }
        //the last sample will be set to the first one after normalisation (see there)

        //normalise to -1...1 (can be set to 0...1 later via RegionLfo.setPolarity)
        auto range = waveform.findMinMax(0, 0, waveform.getNumSamples() - 1);
        float mid = range.getStart() + range.getLength() * 0.5f;
        float mult = 1.0f / (range.getEnd() - mid); //= 1.0f / (mid - range.getStart()) = -1.0f / (range.getStart() - mid)
        //float mult = 1.0f / range.getLength(); //when normalising to 0...1 instead

        for (int i = 0; i < waveform.getNumSamples() - 1; ++i)
        {
            samples[i] = (samples[i] - mid) * mult;
            //samples[i] = (samples[i] - range.getStart()) * mult; //when normalising to 0...1 instead
        }
        samples[waveform.getNumSamples() - 1] = waveform.getSample(0, 0); //the last sample is equal to the first -> makes wrapping simpler and faster

        //apply to LFO
        if (associatedLfo == nullptr) //lfo not yet initialised
        {
            associatedLfo = new RegionLfo(waveform, [] (float, Voice*) {; }, getID()); //no modulation until the voice has been initialised
            jassert(associatedLfo->getPolarity() == RegionLfo::Polarity::bipolar); //default should be bipolar
            audioEngine->addLfo(associatedLfo);
        }
        else
        {
            associatedLfo->setPolarity(RegionLfo::Polarity::bipolar);
            associatedLfo->setWaveTable(waveform);
        }

        associatedLfo->setBaseFrequency(0.2f);

        DBG("LFO's waveform has been rendered.");
    }

    int getID()
    {
        return ID;
    }

    RegionLfo* getAssociatedLfo()
    {
        return associatedLfo;
    }

    AudioEngine* getAudioEngine()
    {
        return audioEngine;
    }

    juce::Point<float> focus;
    juce::Rectangle<float> relativeBounds;

protected:
    void buttonStateChanged() override //void handleButtonStateChanged() //void clicked(const juce::ModifierKeys& modifiers) override
    {
        juce::DrawableButton::buttonStateChanged();

        //clicked(modifiers); //base class stuff

        //modifiers.isRightButtonDown() -> differentiate between left and right mouse button like this

        switch (currentState)
        {
        case State::Playable:
            switch (getState())
            {
            case juce::Button::ButtonState::buttonDown:
                if (isToggleable() && getToggleState() == true) //when toggleable, toggle music on or off. turning it on is handled in the other case
                    stopPlaying();
                else //not in toggle mode or toggling on
                    startPlaying();
                break;
            default:
                if (!isToggleable())
                    stopPlaying();
                break;
            }

            break;
        }
    }

private:
    int ID;

    State currentState;
    bool isPlaying = false;

    juce::Path p; //also acts as a hitbox
    juce::Colour fillColour;
    juce::DrawablePath normalImage; //normal image when not toggleable or toggled off
    juce::DrawablePath overImage; //image when hovering over the button when not toggleable or toggled off
    juce::DrawablePath downImage; //image when clicking the button when not toggleable or toggled off
    juce::DrawablePath disabledImage; //image when disabled when not toggleable or toggled off
    juce::DrawablePath normalImageOn; //normal image when toggleable and toggled on
    juce::DrawablePath overImageOn; //image when hovering over the button when toggleable and toggled on
    juce::DrawablePath downImageOn; //image when clicking the button when toggleable and toggled on
    juce::DrawablePath disabledImageOn; //image when disabled when toggleable and toggled on

    AudioEngine* audioEngine;
    juce::AudioSampleBuffer buffer;
    juce::String audioFileName = "";
    Voice* associatedVoice = nullptr;

    RegionLfo* associatedLfo = nullptr;

    void startPlaying()
    {
        if (audioFileName != "" && !isPlaying)
        {
            DBG("*plays music*");
            isPlaying = true;
            associatedVoice->startNote(0, 1.0f, audioEngine->getSynth()->getSound(0).get(), 64);
            //audioEngine->getSynth()->noteOn(1, 64, 1.0f); //might be worth a thought for later because of polyphony, but since voices will then be chosen automatically, adjustments to the voices class would have to be made
            //audioEngine->getSynth()->getVoice(voiceIndex)->startNote(0, 1.0f, audioEngine->getSynth()->getSound(0).get(), 64);
            
            startTimerHz(20); //24 fps for now; animates the LFO line
        }
    }
    void stopPlaying()
    {
        if (audioFileName != "" && isPlaying)
        {
            DBG("*stops music*");
            associatedVoice->stopNote(1.0f, true);
            //audioEngine->getSynth()->noteOff(1, 64, 1.0f, true);
            //audioEngine->getSynth()->getVoice(voiceIndex)->stopNote(1.0f, true);
            isPlaying = false;
            
            stopTimer();
        }
    }

    class RegionEditorWindow : public juce::DocumentWindow
    {
        //much of this is a copy-paste of the MainWindow class

    public:
        RegionEditorWindow(juce::String name, SegmentedRegion* region)
            : DocumentWindow(name,
                juce::Desktop::getInstance().getDefaultLookAndFeel()
                .findColour(juce::ResizableWindow::backgroundColourId),
                DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(false);
            setContentOwned(new RegionEditor(region), true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
#else
            setResizable(true, true);
            centreWithSize(300, 600);
#endif

            setVisible(true);
            setTitle("Region " + juce::String(region->getID()));
            addToDesktop();
        }

        void closeButtonPressed() override
        {
            //setVisible(false);
            //this->removeFromDesktop();

            delete this;
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
        class RegionEditor : public juce::Component
        {
        public:
            RegionEditor(SegmentedRegion* region) : lfoEditor(region)
            {
                associatedRegion = region;

                //file selection
                selectFileButton.setButtonText("Select Sound File");
                selectFileButton.onClick = [this] { selectFile(); };
                addChildComponent(selectFileButton);

                selectedFileLabel.setText("Please select a file", juce::NotificationType::dontSendNotification); //WIP: changes to "Selected file: (file name)" once a file has been selected
                addChildComponent(selectedFileLabel);
                selectedFileLabel.attachToComponent(&selectFileButton, false);

                //colour picker
                colourPickerWIP.setText("[colour picker WIP]", juce::NotificationType::dontSendNotification);
                //colourPickerWIP.setColour(...); //when a region has actually been selected, the background colour of this component will change
                addChildComponent(colourPickerWIP);

                //focus position
                focusPositionX.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
                focusPositionX.setIncDecButtonsMode(juce::Slider::IncDecButtonMode::incDecButtonsDraggable_Vertical);
                focusPositionX.setNumDecimalPlacesToDisplay(3);
                focusPositionX.setRange(0.0, 1.0, 0.001);
                focusPositionX.onValueChange = [this]
                {
                    updateFocusPosition();
                    if (focusPositionX.getThumbBeingDragged() < 0)
                        renderLfoWaveform();
                };
                focusPositionX.onDragEnd = [this] { renderLfoWaveform(); };
                addChildComponent(focusPositionX);

                focusPositionY.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
                focusPositionY.setIncDecButtonsMode(juce::Slider::IncDecButtonMode::incDecButtonsDraggable_Vertical);
                focusPositionY.setNumDecimalPlacesToDisplay(3);
                focusPositionY.setRange(0.0, 1.0, 0.001);
                focusPositionY.onValueChange = [this]
                {
                    updateFocusPosition();
                    if (focusPositionY.getThumbBeingDragged() < 0)
                        renderLfoWaveform();
                };
                focusPositionY.onDragEnd = [this] { renderLfoWaveform(); };
                addChildComponent(focusPositionY);

                focusPositionLabel.setText("Focus: ", juce::NotificationType::dontSendNotification);
                addChildComponent(focusPositionLabel);
                focusPositionLabel.attachToComponent(&focusPositionX, true);

                //toggle mode
                toggleModeButton.setButtonText("Toggle Mode");
                addChildComponent(toggleModeButton);

                //DAHDSR
                dahdsrWIP.setText("[DAHDSR settings WIP]", juce::NotificationType::dontSendNotification);
                addChildComponent(dahdsrWIP);

                //volume
                volumeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
                volumeSlider.setRange(-60.0, 6.0, 0.01);
                volumeSlider.setSkewFactorFromMidPoint(-6.0);
                volumeSlider.onValueChange = [this] { updateVolume(); };
                volumeSlider.setValue(-6.0, juce::NotificationType::dontSendNotification);
                addChildComponent(volumeSlider);

                volumeLabel.setText("Volume: ", juce::NotificationType::dontSendNotification);
                addChildComponent(volumeLabel);
                volumeLabel.attachToComponent(&volumeSlider, true);

                //pitch
                pitchSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
                pitchSlider.setRange(-60.0, 60.0, 0.1);
                pitchSlider.setTextValueSuffix("st");
                pitchSlider.onValueChange = [this] { updatePitch(); };
                pitchSlider.setValue(0.0, juce::NotificationType::dontSendNotification);
                addChildComponent(pitchSlider);

                pitchLabel.setText("Pitch: ", juce::NotificationType::dontSendNotification);
                addChildComponent(pitchLabel);
                pitchLabel.attachToComponent(&pitchSlider, true);

                //LFO editor
                addChildComponent(lfoEditor);

                //delete region
                deleteRegionButton.setButtonText("Delete Region");
                deleteRegionButton.onClick = [this] { deleteRegion(); };
                addChildComponent(deleteRegionButton);

                //other preparations
                copyRegionParameters();
                setChildVisibility(true);

                setSize(350, 700);

                formatManager.registerBasicFormats();
            }

            ~RegionEditor() override
            {
                //associatedRegion.deleteAndZero(); //do NOT delete the SegmentedRegion.
                associatedRegion = nullptr;
            }

            void paint(juce::Graphics& g) override
            {
                g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));   // clear the background

                g.setColour(juce::Colours::lightgrey);
                g.drawRect(getLocalBounds(), 1);   // draw an outline around the component
            }

            void resized() override
            {
                if (associatedRegion != nullptr)
                {
                    auto area = getLocalBounds();

                    selectedFileLabel.setBounds(area.removeFromTop(20));
                    selectFileButton.setBounds(area.removeFromTop(20));

                    colourPickerWIP.setBounds(area.removeFromTop(20));

                    auto focusArea = area.removeFromTop(20);
                    focusArea.removeFromLeft(focusArea.getWidth() / 3);
                    focusPositionX.setBounds(focusArea.removeFromLeft(focusArea.getWidth() / 2));
                    focusPositionY.setBounds(focusArea);

                    toggleModeButton.setBounds(area.removeFromTop(20));
                    toggleModeButton.onClick = [this] { updateToggleable(); };

                    dahdsrWIP.setBounds(area.removeFromTop(20));

                    auto volumeArea = area.removeFromTop(20);
                    volumeLabel.setBounds(volumeArea.removeFromLeft(volumeArea.getWidth() / 3));
                    volumeSlider.setBounds(volumeArea);

                    auto pitchArea = area.removeFromTop(20);
                    pitchLabel.setBounds(pitchArea.removeFromLeft(pitchArea.getWidth() / 3));
                    pitchSlider.setBounds(pitchArea);



                    deleteRegionButton.setBounds(area.removeFromBottom(20));

                    lfoEditor.setBounds(area); //fill rest with lfoEditor
                }
            }

        private:
            void setChildVisibility(bool shouldBeVisible)
            {
                selectedFileLabel.setVisible(shouldBeVisible);
                selectFileButton.setVisible(shouldBeVisible);

                colourPickerWIP.setVisible(shouldBeVisible);

                focusPositionLabel.setVisible(shouldBeVisible);
                focusPositionX.setVisible(shouldBeVisible);
                focusPositionY.setVisible(shouldBeVisible);

                toggleModeButton.setVisible(shouldBeVisible);

                dahdsrWIP.setVisible(shouldBeVisible);

                volumeLabel.setVisible(shouldBeVisible);
                volumeSlider.setVisible(shouldBeVisible);

                pitchLabel.setVisible(shouldBeVisible);
                pitchSlider.setVisible(shouldBeVisible);

                lfoEditor.setVisible(shouldBeVisible);

                deleteRegionButton.setVisible(shouldBeVisible);
            }

            void copyRegionParameters()
            {
                //ä //WIP
                DBG("[copy region parameters WIP]");

                if (associatedRegion->audioFileName == "")
                    selectedFileLabel.setText("Please select a file", juce::NotificationType::dontSendNotification);
                else
                    selectedFileLabel.setText("Selected file: " + associatedRegion->audioFileName, juce::NotificationType::dontSendNotification);

                focusPositionX.setValue(associatedRegion->focus.getX(), juce::NotificationType::dontSendNotification);
                focusPositionY.setValue(associatedRegion->focus.getY(), juce::NotificationType::dontSendNotification);

                toggleModeButton.setToggleState(associatedRegion->isToggleable(), juce::NotificationType::dontSendNotification);

                Voice* voice = associatedRegion->associatedVoice;
                if (voice != nullptr)
                {
                    volumeSlider.setValue(juce::Decibels::gainToDecibels<double>(voice->getBaseLevel(), -60.0), juce::NotificationType::dontSendNotification);
                    pitchSlider.setValue(voice->getBasePitchShift(), juce::NotificationType::dontSendNotification);
                }

                lfoEditor.copyParameters();
            }

            void selectFile()
            {
                //shutdownAudio();

                fc = std::make_unique<juce::FileChooser>("Select a Wave file shorter than 2 seconds to play...",
                    juce::File{},
                    "*.wav");
                auto chooserFlags = juce::FileBrowserComponent::openMode
                    | juce::FileBrowserComponent::canSelectFiles;

                fc->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
                    {
                        auto file = fc.getResult();

                        if (file == juce::File{})
                            return;

                        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file)); // [2]

                        if (reader.get() != nullptr)
                        {
                            auto duration = (float)reader->lengthInSamples / reader->sampleRate;               // [3]

                            if (true)//(duration <= 7)
                            {
                                //juce::AudioSampleBuffer tempBuffer();
                                tempBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);  // [4]
                                reader->read(&tempBuffer,                                                      // [5]
                                    0,                                                                //  [5.1]
                                    (int)reader->lengthInSamples,                                    //  [5.2]
                                    0,                                                                //  [5.3]
                                    true,                                                             //  [5.4]
                                    true);                                                            //  [5.5]
                                //position = 0;                                                                   // [6]
                                //setAudioChannels(0, (int)reader->numChannels);                                // [7]
                                
                                associatedRegion->setBuffer(tempBuffer, file.getFileName(), reader->sampleRate);
                                selectedFileLabel.setText(file.getFileName(), juce::NotificationType::dontSendNotification);
                                lfoEditor.updateAvailableVoices();
                                updateAllVoiceSettings(); //sets currently selected volume, pitch etc.
                            }
                            else
                            {
                                // handle the error that the file is 2 seconds or longer..
                                DBG("[invalid file - callback WIP]");

                                associatedRegion->setBuffer(juce::AudioSampleBuffer(), "", 0.0); //empty buffer
                            }
                        }
                    });
            }

            void updateFocusPosition()
            {
                associatedRegion->focus.setX(static_cast<float>(focusPositionX.getValue()));
                associatedRegion->focus.setY(static_cast<float>(focusPositionY.getValue()));
                associatedRegion->repaint();
            }

            void renderLfoWaveform()
            {
                associatedRegion->renderLfoWaveform();
            }

            void updateToggleable()
            {
                //associatedRegion->setToggleable(toggleModeButton.getToggleState());
                associatedRegion->setClickingTogglesState(toggleModeButton.getToggleState()); //this also automatically sets isToggleable
            }

            void updateAllVoiceSettings() //used after a voice has been first set or changed
            {
                updateVolume();
            }
            void updateVolume()
            {
                Voice* voice = associatedRegion->associatedVoice;
                if (voice != nullptr)
                    voice->setBaseLevel(juce::Decibels::decibelsToGain<double>(volumeSlider.getValue(), -60.0));
            }
            void updatePitch()
            {
                Voice* voice = associatedRegion->associatedVoice;
                if (voice != nullptr)
                    voice->setBasePitchShift(pitchSlider.getValue());
            }

            void deleteRegion()
            {
                //WIP: later, this will remove the associated Region from the canvas
            }

            //================================================================
            SegmentedRegion* associatedRegion;

            juce::Label selectedFileLabel;
            juce::TextButton selectFileButton;

            juce::Label colourPickerWIP; //WIP: later, this will be a button that opens a separate window containing a juce::ColourSelector (the GUI for these is rather large, see https://www.ccoderun.ca/juce/api/ColourSelector.png )
            //^- when changing region colours, also call associatedRegion->audioEngine->changeRegionColour

            juce::Label focusPositionLabel;
            juce::Slider focusPositionX; //inc/dec slider 
            juce::Slider focusPositionY; //inc/dec slider

            juce::ToggleButton toggleModeButton;

            juce::Label dahdsrWIP; //WIP: later, this will contain a little user interface for DAHDSR settings (or maybe just ADSR since there's already a class for that)

            juce::Label volumeLabel;
            juce::Slider volumeSlider;

            juce::Label pitchLabel;
            juce::Slider pitchSlider;

            class LfoEditor : public juce::Component
            {
            public:
                LfoEditor(SegmentedRegion* associatedRegion)
                {
                    this->associatedRegion = associatedRegion;

                    lfoRateSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
                    lfoRateSlider.setRange(0.01, 100.0, 0.01);
                    lfoRateSlider.setSkewFactorFromMidPoint(1.0);
                    lfoRateSlider.onValueChange = [this] { updateLfoRate(); };
                    addAndMakeVisible(lfoRateSlider);

                    lfoRateLabel.setText("LFO rate: ", juce::NotificationType::dontSendNotification);
                    addAndMakeVisible(lfoRateLabel);
                    lfoRateLabel.attachToComponent(&lfoRateSlider, true);

                    lfoParameterChoice.addSectionHeading("Basic");
                    lfoParameterChoice.addItem("Volume", volume);
                    lfoParameterChoice.addItem("Pitch", pitch);
                    //lfoParameterChoice.addItem("Panning", panning);
                    lfoParameterChoice.addSeparator();
                    lfoParameterChoice.addSectionHeading("LFO");
                    lfoParameterChoice.addItem("LFO Rate", lfoRate);
                    lfoParameterChoice.addSeparator();
                    lfoParameterChoice.addSectionHeading("Experimental");
                    lfoParameterChoice.addItem("Playback Position", playbackPosition);
                    lfoParameterChoice.onChange = [this] { updateLfoParameter(); };
                    addAndMakeVisible(lfoParameterChoice);

                    lfoParameterLabel.setText("Modulated Parameter: ", juce::NotificationType::dontSendNotification);
                    addAndMakeVisible(lfoParameterLabel);
                    lfoParameterLabel.attachToComponent(&lfoParameterChoice, true);

                    updateAvailableVoices();
                    addAndMakeVisible(lfoRegionsList);
                }

                ~LfoEditor() override
                {
                }

                void paint(juce::Graphics& g) override
                {
                    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));   // clear the background
                }

                void resized() override
                {
                    auto area = getLocalBounds();

                    auto lfoRateArea = area.removeFromTop(20);
                    lfoRateLabel.setBounds(lfoRateArea.removeFromLeft(lfoRateArea.getWidth() / 3));
                    lfoRateSlider.setBounds(lfoRateArea);

                    auto lfoParamArea = area.removeFromTop(20);
                    lfoParameterLabel.setBounds(lfoParamArea.removeFromLeft(lfoParamArea.getWidth() / 3));
                    lfoParameterChoice.setBounds(lfoParamArea);

                    lfoRegionsList.setBounds(area);
                }

                void copyParameters()
                {
                    auto lfo = associatedRegion->getAssociatedLfo();

                    //copy rate
                    lfoRateSlider.setValue(lfo->getBaseFrequency(), juce::NotificationType::dontSendNotification);

                    //copy modulated parameter
                    if (lfo->getModulatedParameterID() > 0) //parameter IDs start with 1 (0 isn't included because of how selected item IDs work with ComboBoxes in juce)
                    {
                        lfoParameterChoice.setSelectedId(lfo->getModulatedParameterID(), juce::NotificationType::dontSendNotification);
                    }

                    //copy currently affected voices
                    lfoRegionsList.setCheckedRegions(lfo->getRegionIDs());
                }

                void updateAvailableVoices()
                {
                    lfoRegionsList.clearItems();
                    juce::Array<int> regionIdList;

                    for (int i = 0; i <= associatedRegion->audioEngine->getLastRegionID(); ++i)
                    {
                        if (associatedRegion->getAudioEngine()->checkRegionHasVoice(i) == true)
                        {
                            regionIdList.add(i);
                        }
                    }

                    for (auto it = regionIdList.begin(); it != regionIdList.end(); ++it)
                    {
                        auto newRowNumber = lfoRegionsList.addItem("Region " + juce::String(*it), *it);
                        lfoRegionsList.setRowBackgroundColour(newRowNumber, associatedRegion->audioEngine->getRegionColour(*it));
                        lfoRegionsList.setClickFunction(newRowNumber, [this] { updateLfoVoices(lfoRegionsList.getCheckedRegionIDs()); });
                    }
                }

            private:
                SegmentedRegion* associatedRegion;

                juce::Label lfoRateLabel;
                juce::Slider lfoRateSlider;

                juce::Label lfoParameterLabel;
                juce::ComboBox lfoParameterChoice;

                CheckBoxList lfoRegionsList;

                enum ModulatableParameter
                {
                    volume = 1,
                    pitch,
                    //Panning,

                    lfoRate,
                    //lfoDepth

                    playbackPosition
                };

                void updateLfoRate()
                {
                    associatedRegion->getAssociatedLfo()->setBaseFrequency(lfoRateSlider.getValue());
                }

                void updateLfoParameter()
                {
                    //note: the braces for the case statements are required here to allow for variable initialisations. see https://stackoverflow.com/questions/5136295/switch-transfer-of-control-bypasses-initialization-of-when-calling-a-function
                    switch (lfoParameterChoice.getSelectedId())
                    {
                    case volume:
                    {
                        associatedRegion->getAssociatedLfo()->setPolarity(RegionLfo::Polarity::unipolar);
                        std::function<void(float, Voice*)> func = [](float lfoValue, Voice* v) { v->modulateLevel(static_cast<double>(lfoValue)); };
                        associatedRegion->getAssociatedLfo()->setModulationFunction(func);
                        associatedRegion->getAssociatedLfo()->setModulatedParameterID(volume);
                        break;
                    }

                    case pitch:
                    {
                        associatedRegion->getAssociatedLfo()->setPolarity(RegionLfo::Polarity::bipolar);
                        std::function<void(float, Voice*)> func = [](float lfoValue, Voice* v) { v->modulatePitchShift(static_cast<double>(lfoValue) * 12.0); };
                        associatedRegion->getAssociatedLfo()->setModulationFunction(func);
                        associatedRegion->getAssociatedLfo()->setModulatedParameterID(pitch);
                        break;
                    }

                    case lfoRate:
                    {
                        associatedRegion->getAssociatedLfo()->setPolarity(RegionLfo::Polarity::bipolar);
                        std::function<void(float, Voice*)> func = [](float lfoValue, Voice* v) { v->getLfoFreqModFunction()(lfoValue * 24.0f); };
                        associatedRegion->getAssociatedLfo()->setModulationFunction(func);
                        associatedRegion->getAssociatedLfo()->setModulatedParameterID(lfoRate);
                        break;
                    }

                    case playbackPosition:
                    {
                        associatedRegion->getAssociatedLfo()->setPolarity(RegionLfo::Polarity::unipolar);
                        //WIP
                        break;
                    }

                    default:
                        break;
                    }
                }

                void updateLfoVoices(juce::Array<int> voiceIndices)
                {
                    juce::Array<Voice*> voices;
                    for (auto it = voiceIndices.begin(); it != voiceIndices.end(); ++it)
                    {
                        voices.addArray(associatedRegion->getAudioEngine()->getVoicesWithID(*it));
                    }
                    associatedRegion->getAssociatedLfo()->setVoices(voices);

                    DBG("voices updated.");
                }

                JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoEditor)
            };
            LfoEditor lfoEditor;

            juce::TextButton deleteRegionButton;

            std::unique_ptr<juce::FileChooser> fc;
            juce::AudioFormatManager formatManager;
            juce::AudioSampleBuffer tempBuffer;


            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegionEditor)
        };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegionEditorWindow)
    };

    juce::Component::SafePointer<RegionEditorWindow> regionEditorWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentedRegion)
};
