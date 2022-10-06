/*
  ==============================================================================

    SegmentedRegion.cpp
    Created: 8 Jul 2022 10:03:18am
    Author:  Aaron

  ==============================================================================
*/

#include "SegmentedRegion.h"


//public

SegmentedRegion::SegmentedRegion(const juce::Path& outline, const juce::Rectangle<float>& relativeBounds, AudioEngine* audioEngine) :
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

SegmentedRegion::~SegmentedRegion()
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

void SegmentedRegion::initialiseImages()
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

void SegmentedRegion::timerCallback()
{
    repaint(); //could the redrawn area be reduced?
}
void SegmentedRegion::setTimerInterval(int newIntervalMs)
{
    if (isTimerRunning())
    {
        stopTimer();
        timerIntervalMs = juce::jmax(20, newIntervalMs);
        startTimer(timerIntervalMs);
    }
    else
    {
        timerIntervalMs = juce::jmax(20, newIntervalMs);
    }
    DBG("set timer interval to " + juce::String(timerIntervalMs) + "ms");
}

void SegmentedRegion::paintOverChildren(juce::Graphics& g)
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
        float curLfoPhase = associatedLfo->getPhase(); //* associatedLfo->getPhaseModParameter()->getModulatedValue();

        if (isPlaying)
            g.setColour(juce::Colour::contrasting(fillColour, fillColour.darker(0.4)));
        else
            g.setColour(juce::Colour::contrasting(fillColour, fillColour.darker(0.4)).withAlpha(0.5f)); //faded when not playing

        //draw line from focus point to point on the outline that corresponds to the associated LFO's current phase
        juce::Point<float> outlinePt = p.getPointAlongPath(curLfoPhase * p.getLength(), juce::AffineTransform(), juce::Path::defaultToleranceForMeasurement);
        g.drawLine(focusPt.x, focusPt.y,
            outlinePt.x, outlinePt.y,
            3.0f);
    }

    auto voices = audioEngine->getVoicesWithID(ID);

    for (auto it = voices.begin(); it != voices.end(); it++)
    {
        if ((*it)->isPlaying())
        {
            return; //there's still voices playing -> keep rendering
        }
    }

    //no more voices playing -> stop rendering
    stopTimer();
}

void SegmentedRegion::resized() //WIP: for some reason, this is called when hovering over the button
{
    //recalculate hitbox
    p.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), false);

    juce::DrawableButton::resized();
}

bool SegmentedRegion::hitTest(int x, int y)
{
    return p.contains((float)x, (float)y);
}

void SegmentedRegion::setState(SegmentedRegionState newState)
{
    if (currentState != newState)
    {
        currentState = newState; //the state mainly changes how the click method behaves
        DBG("set SegmentedRegion's current state to " + juce::String(static_cast<int>(currentState)));

        //WIP: when switching to Playable, preparing the audio will probably be necessary

        //resized();
    }
}

void SegmentedRegion::clicked(const juce::ModifierKeys& modifiers)
{
    juce::Button::buttonStateChanged();

    //clicked(modifiers); //base class stuff

    //modifiers.isRightButtonDown() -> differentiate between left and right mouse button like this

    switch (currentState)
    {
    case SegmentedRegionState::Standby:
        DBG("*stands by*");
        break;

    case SegmentedRegionState::Editing:
        DBG("*shows editor window*");
        if (regionEditorWindow != nullptr)
        {
            regionEditorWindow->toFront(true);
        }
        else
        {
            regionEditorWindow = juce::Component::SafePointer<RegionEditorWindow>(new RegionEditorWindow("Region " + juce::String(ID) + " Editor", this));
        }
        break;

    case SegmentedRegionState::Playable:
        //handled though buttonStateChanged
        break;
    }
}

void SegmentedRegion::setBuffer(juce::AudioSampleBuffer newBuffer, juce::String fileName, double origSampleRate)
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

void SegmentedRegion::renderLfoWaveform()
{
    DBG("rendering LFO's waveform...");

    juce::AudioBuffer<float> waveform(1, juce::jmax<int>(2, (int)p.getLength()) + 1); //minimum size of 2 samples (should always be the case since a minimum of 3 points are necessary to define a region)
    auto samples = waveform.getWritePointer(0);

    //the for-loop will iterate from 0.0 to the end point (length) of the path.
    //to get the desired number of samples, the path must be divided into sections with the following distance:
    float stepDistance = p.getLength() / ((float)(waveform.getNumSamples() - 2));

    juce::Point<float> relativeFocus(focus.getX() * getBounds().getWidth(), focus.getY() * getBounds().getHeight());
    for (int i = 0; i < waveform.getNumSamples() - 1; ++i)
    {

        samples[i] = p.getPointAlongPath((float)i * stepDistance, //this will usually just iterate between each pixel
            juce::AffineTransform(), //no transform
            juce::Path::defaultToleranceForMeasurement //?
        ).getDistanceSquaredFrom(relativeFocus); //use distance from focus point to create a 1D value
    }
    //the last sample will be set to the first one after normalisation (see there)

    //normalise to -1...1 (can be set to 0...1 later via RegionLfo.setPolarity)
    auto range = waveform.findMinMax(0, 0, waveform.getNumSamples() - 1);
    //float mid = range.getStart() + range.getLength() * 0.5f;
    //float mult = 1.0f / (range.getEnd() - mid); //= 1.0f / (mid - range.getStart()) = -1.0f / (range.getStart() - mid) //when normalising to -1...1
    float mult = 1.0f / range.getLength(); //when normalising to 0...1

    for (int i = 0; i < waveform.getNumSamples() - 1; ++i)
    {
        //samples[i] = (samples[i] - mid) * mult; //when normalising to -1...1
        samples[i] = (samples[i] - range.getStart()) * mult; //when normalising to 0...1
    }
    samples[waveform.getNumSamples() - 1] = waveform.getSample(0, 0); //the last sample is equal to the first -> makes wrapping simpler and faster

    //apply to LFO
    if (audioEngine->getLfo(ID) == nullptr) //lfo not yet initialised
    {
        associatedLfo = new RegionLfo(waveform, RegionLfo::Polarity::unipolar, getID()); //no modulation until the voice has been initialised
        audioEngine->addLfo(associatedLfo);
        associatedLfo->setBaseFrequency(0.2f);
    }
    else
    {
        associatedLfo->setWaveTable(waveform, RegionLfo::Polarity::unipolar);
    }

    DBG("LFO's waveform has been rendered.");
}

int SegmentedRegion::getID()
{
    return ID;
}

RegionLfo* SegmentedRegion::getAssociatedLfo()
{
    return associatedLfo;
}

AudioEngine* SegmentedRegion::getAudioEngine()
{
    return audioEngine;
}

Voice* SegmentedRegion::getAssociatedVoice()
{
    return associatedVoice;
}
juce::String SegmentedRegion::getFileName()
{
    return audioFileName;
}




//protected

void SegmentedRegion::buttonStateChanged() //void handleButtonStateChanged() //void clicked(const juce::ModifierKeys& modifiers) override
{
    juce::DrawableButton::buttonStateChanged();

    //clicked(modifiers); //base class stuff

    //modifiers.isRightButtonDown() -> differentiate between left and right mouse button like this

    switch (currentState)
    {
    case SegmentedRegionState::Playable:
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




//private

void SegmentedRegion::startPlaying()
{
    if (audioFileName != "" && !isPlaying)
    {
        DBG("*plays music*");
        isPlaying = true;
        associatedVoice->startNote(0, 1.0f, audioEngine->getSynth()->getSound(0).get(), 64);
        //audioEngine->getSynth()->noteOn(1, 64, 1.0f); //might be worth a thought for later because of polyphony, but since voices will then be chosen automatically, adjustments to the voices class would have to be made
        //audioEngine->getSynth()->getVoice(voiceIndex)->startNote(0, 1.0f, audioEngine->getSynth()->getSound(0).get(), 64);

        startTimer(timerIntervalMs); //animates the LFO line
    }
}
void SegmentedRegion::stopPlaying()
{
    if (audioFileName != "" && isPlaying)
    {
        DBG("*stops music*");
        associatedVoice->stopNote(1.0f, true);
        //audioEngine->getSynth()->noteOff(1, 64, 1.0f, true);
        //audioEngine->getSynth()->getVoice(voiceIndex)->stopNote(1.0f, true);
        isPlaying = false;

        //stopTimer(); //stopped in the drawing method since the LFO will have to keep being drawn for a little longer because of release time
    }
}
