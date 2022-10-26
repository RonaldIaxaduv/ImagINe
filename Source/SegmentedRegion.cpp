/*
  ==============================================================================

    SegmentedRegion.cpp
    Created: 8 Jul 2022 10:03:18am
    Author:  Aaron

  ==============================================================================
*/

#include "SegmentedRegion.h"


//public

SegmentedRegion::SegmentedRegion(const juce::Path& outline, const juce::Rectangle<float>& relativeBounds, juce::Colour fillColour, AudioEngine* audioEngine) :
    juce::DrawableButton("", ButtonStyle::ImageStretched),
    p(outline),
    relativeBounds(relativeBounds),
    fillColour(fillColour), currentLfoLine()
{
    regionEditorWindow = nullptr;

    this->audioEngine = audioEngine;
    ID = audioEngine->addNewRegion(fillColour); //also generates the region's LFO and all its Voice instances
    associatedLfo = audioEngine->getLfo(ID);
    associatedVoices = audioEngine->getVoicesWithID(ID);

    //initialise states
    states[static_cast<int>(SegmentedRegionStateIndex::notInteractable)] = static_cast<SegmentedRegionState*>(new SegmentedRegionState_NotInteractable(*this));
    states[static_cast<int>(SegmentedRegionStateIndex::editable)] = static_cast<SegmentedRegionState*>(new SegmentedRegionState_Editable(*this));
    states[static_cast<int>(SegmentedRegionStateIndex::playable)] = static_cast<SegmentedRegionState*>(new SegmentedRegionState_Playable(*this));
    int initialisedStates = 3;
    jassert(initialisedStates == static_cast<int>(SegmentedRegionStateIndex::StateIndexCount));

    currentStateIndex = initialStateIndex;
    currentState = states[static_cast<int>(currentStateIndex)];

    //initialise images
    initialiseImages();

    //default: set focus in the centre of the shape
    focus = p.getBounds().getCentre();
    focus.setXY(focus.getX() / p.getBounds().getWidth(), focus.getY() / p.getBounds().getHeight()); //relative centre

    //LFO
    renderLfoWaveform(); //initialises LFO further (generates its wavetable)

    setBuffer(juce::AudioSampleBuffer(), "", 0.0); //no audio file set yet -> empty buffer
    
    //currentLfoLine = juce::Line<float>(juce::Point<float>(0.0f, 0.0f), juce::Point<float>(static_cast<float>(getWidth()), static_cast<float>(getHeight()))); //diagonal -> entire region will be redrawn (a little hacky, but ensures that the LFO line is drawn before the region is first played)
    //repaint(); //paints the LFO line
}

SegmentedRegion::~SegmentedRegion()
{
    DBG("destroying SegmentedRegion...");

    stopTimer();

    //WIP maybe the close button of the editor window will have to be pressed before dropping the pointer
    if (regionEditorWindow != nullptr)
    {
        DBG("RegionEditorWindow still alive -> deleting");
        regionEditorWindow.deleteAndZero();
    }

    //release states
    currentState = nullptr;
    delete states[static_cast<int>(SegmentedRegionStateIndex::notInteractable)];
    delete states[static_cast<int>(SegmentedRegionStateIndex::editable)];
    delete states[static_cast<int>(SegmentedRegionStateIndex::playable)];
    int deletedStates = 3;
    jassert(deletedStates == static_cast<int>(SegmentedRegionStateIndex::StateIndexCount));

    //release Voice(s)
    associatedVoices.clear();
    audioEngine->removeVoicesWithID(getID());

    //release LFO
    associatedLfo = nullptr;
    audioEngine->removeLfo(getID()); //exception freeing heap after the LFO has been destroyed -> some invalid member?

    //release audio engine
    audioEngine = nullptr;

    //~DrawableButton(); //this caused dangling pointers in the past -> try not to call if at all possible!

    DBG("SegmentedRegion destroyed.");
}

void SegmentedRegion::initialiseImages()
{
    setSize(static_cast<int>(500.0f * relativeBounds.getWidth()),
        static_cast<int>(500.0f * relativeBounds.getHeight())); //must be set before the images are drawn, because drawables require bounds to be displayed! 

    DBG("initial size of segmented region: " + getBounds().toString());
    DBG("bounds of the passed path: " + p.getBounds().toString());

    float outlineThickness = 1.0f;
    float inherentTransparency = 0.70f;
    float disabledTransparency = 0.20f;

    normalImage.setPath(p);
    normalImage.setFill(juce::FillType(fillColour.withAlpha(inherentTransparency)));
    //normalImage.setBufferedToImage(true) <- WIP: do this? would this improve performance?
    //normalImage.setHasFocusOutline
    normalImage.setStrokeThickness(outlineThickness);
    normalImage.setStrokeFill(juce::FillType(normalImage.getFill().colour.contrasting()));
    normalImage.setBounds(getBounds());

    overImage.setPath(p);
    overImage.setFill(juce::FillType(fillColour.brighter(0.2f).withAlpha(inherentTransparency)));
    overImage.setStrokeThickness(outlineThickness);
    overImage.setStrokeFill(juce::FillType(overImage.getFill().colour.contrasting()));
    overImage.setBounds(getBounds());

    downImage.setPath(p);
    downImage.setFill(juce::FillType(fillColour.darker(0.2f).withAlpha(inherentTransparency)));
    downImage.setStrokeThickness(outlineThickness);
    downImage.setStrokeFill(juce::FillType(downImage.getFill().colour.contrasting()));
    downImage.setBounds(getBounds());

    disabledImage.setPath(p);
    disabledImage.setFill(juce::FillType(fillColour.withAlpha(disabledTransparency)));
    disabledImage.setStrokeThickness(outlineThickness);
    disabledImage.setStrokeFill(juce::FillType(disabledImage.getFill().colour.contrasting()));
    disabledImage.setBounds(getBounds());


    normalImageOn.setPath(p);
    normalImageOn.setFill(juce::FillType(fillColour.darker(0.4f).withAlpha(inherentTransparency)));
    normalImageOn.setStrokeThickness(outlineThickness);
    normalImageOn.setStrokeFill(juce::FillType(normalImageOn.getFill().colour.contrasting()));
    normalImageOn.setBounds(getBounds());

    overImageOn.setPath(p);
    overImageOn.setFill(juce::FillType(fillColour.darker(0.2f).withAlpha(inherentTransparency)));
    overImageOn.setStrokeThickness(outlineThickness);
    overImageOn.setStrokeFill(juce::FillType(overImageOn.getFill().colour.contrasting()));
    overImageOn.setBounds(getBounds());

    downImageOn.setPath(p);
    downImageOn.setFill(juce::FillType(fillColour.darker(0.6f).withAlpha(inherentTransparency)));
    downImageOn.setStrokeThickness(outlineThickness);
    downImageOn.setStrokeFill(juce::FillType(downImageOn.getFill().colour.contrasting()));
    downImageOn.setBounds(getBounds());

    disabledImageOn.setPath(p);
    disabledImageOn.setFill(juce::FillType(fillColour.darker(0.4f).withAlpha(disabledTransparency)));
    disabledImageOn.setStrokeThickness(outlineThickness);
    disabledImageOn.setStrokeFill(juce::FillType(disabledImageOn.getFill().colour.contrasting()));
    disabledImageOn.setBounds(getBounds());

    setImages(&normalImage, &overImage, &downImage, &disabledImage, &normalImageOn, &overImageOn, &downImageOn, &disabledImageOn);

    setColour(ColourIds::backgroundOnColourId, juce::Colours::transparentBlack); //otherwise, the button has a grey background colour while the button is toggled on
    lfoLineColour = juce::Colour::contrasting(fillColour, fillColour.contrasting());
    focusPointColour = fillColour.contrasting();
}

void SegmentedRegion::timerCallback()
{
    //repaint(); //WIP: could the redrawn area be reduced?

    juce::Rectangle<float> previousArea(juce::jmin(currentLfoLine.getStartX(), currentLfoLine.getEndX()),
                                        juce::jmin(currentLfoLine.getStartY(), currentLfoLine.getEndY()),
                                        juce::jmax(currentLfoLine.getStartX(), currentLfoLine.getEndX()) - juce::jmin(currentLfoLine.getStartX(), currentLfoLine.getEndX()),
                                        juce::jmax(currentLfoLine.getStartY(), currentLfoLine.getEndY()) - juce::jmin(currentLfoLine.getStartY(), currentLfoLine.getEndY()));

    if (associatedLfo == nullptr)
    {
        DBG("NO LFO SET YET"); //if the code arrives here, this method might need a state-dependent implementation...
        return;
    }
    
    float curLfoPhase = associatedLfo->getLatestModulatedPhase(); //basically the same value as getModulatedValue of the parameter, but won't update that parameter (which would mess with the modulation)
    juce::Point<float> focusPt(focus.x * getBounds().getWidth(),
        focus.y * getBounds().getHeight()); 
    juce::Point<float> outlinePt = p.getPointAlongPath(curLfoPhase * p.getLength(), juce::AffineTransform(), juce::Path::defaultToleranceForMeasurement);
    currentLfoLine = juce::Line<float>(focusPt.x, focusPt.y,
                                       outlinePt.x, outlinePt.y);

    juce::Rectangle<float> currentArea(juce::jmin(currentLfoLine.getStartX(), currentLfoLine.getEndX()),
                                       juce::jmin(currentLfoLine.getStartY(), currentLfoLine.getEndY()),
                                       juce::jmax(currentLfoLine.getStartX(), currentLfoLine.getEndX()) - juce::jmin(currentLfoLine.getStartX(), currentLfoLine.getEndX()),
                                       juce::jmax(currentLfoLine.getStartY(), currentLfoLine.getEndY()) - juce::jmin(currentLfoLine.getStartY(), currentLfoLine.getEndY()));

    repaint(previousArea.expanded(lfoLineThickness, lfoLineThickness).toNearestInt());
    repaint(currentArea.expanded(lfoLineThickness, lfoLineThickness).toNearestInt());
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
    //DBG("set timer interval to " + juce::String(timerIntervalMs) + "ms");
}

void SegmentedRegion::paintOverChildren(juce::Graphics& g)
{
    //if the normal paint method were used, the button images would be in front of whatever is drawn there.
    //with this method, everything is drawn in front of the button images.

    juce::Point<float> focusPt(focus.x * getBounds().getWidth(),
        focus.y * getBounds().getHeight());

    //draw focus point
    g.setColour(focusPointColour);
    float focusRadius = 2.5f;
    g.fillEllipse(focusPt.x - focusRadius,
        focusPt.y - focusRadius,
        2.0f * focusRadius,
        2.0f * focusRadius);

    if (associatedLfo == nullptr)
    {
        stopTimer();
        DBG("STOP!");
        return;
    }

    //draw LFO line
    //float curLfoPhase = associatedLfo->getLatestModulatedPhase(); //basically the same value as getModulatedValue of the parameter, but won't update that parameter (which would mess with the modulation)

    if (isPlaying)
        g.setColour(lfoLineColour);
    else
        g.setColour(lfoLineColour.withAlpha(0.5f)); //faded when not playing

    //draw line from focus point to point on the outline that corresponds to the associated LFO's current phase
    //juce::Point<float> outlinePt = p.getPointAlongPath(curLfoPhase * p.getLength(), juce::AffineTransform(), juce::Path::defaultToleranceForMeasurement);
    //g.drawLine(focusPt.x, focusPt.y,
    //    outlinePt.x, outlinePt.y,
    //    lfoLineThickness);
    g.setColour(focusPointColour);
    g.drawLine(currentLfoLine, lfoLineThickness);
    g.setColour(lfoLineColour);
    g.drawLine(currentLfoLine, lfoLineThickness * 0.666f);

    //check whether the update interval changed (e.g. due to modulation)
    //int newTimerIntervalMs = static_cast<int>(associatedLfo->getUpdateInterval_Milliseconds());
    int newTimerIntervalMs = juce::jmax(20, static_cast<int>(associatedLfo->getMsUntilUpdate()));
    if (newTimerIntervalMs != timerIntervalMs)
    {
        setTimerInterval(newTimerIntervalMs);
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
    //DBG("voices of region " + juce::String(ID) + " have stopped. stopping timer...");
    stopTimer();
}

void SegmentedRegion::resized() //WIP: for some reason, this is called when hovering over the button
{
    //recalculate hitbox
    p.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), false);

    //repaint LFO line
    timerCallback();

    juce::DrawableButton::resized();
}

bool SegmentedRegion::hitTest(int x, int y)
{
    return currentState->hitTest(x, y);
}
bool SegmentedRegion::hitTest_Interactable(int x, int y)
{
    return p.contains((float)x, (float)y);
}

//void SegmentedRegion::setState(SegmentedRegionState newState)
//{
//    if (currentState != newState)
//    {
//        currentState = newState; //the state mainly changes how the click method behaves
//        DBG("set SegmentedRegion's current state to " + juce::String(static_cast<int>(currentState)));
//
//        //WIP: when switching to Playable, preparing the audio will probably be necessary
//
//        //resized();
//    }
//}
void SegmentedRegion::transitionToState(SegmentedRegionStateIndex stateToTransitionTo)
{
    bool nonInstantStateFound = false;

    while (!nonInstantStateFound) //check if any states can be skipped (might be the case depending on what has been prepared already)
    {
        switch (stateToTransitionTo)
        {
        case SegmentedRegionStateIndex::notInteractable:
            nonInstantStateFound = true;

            setToggleState(false, juce::NotificationType::dontSendNotification);
            setToggleable(false);
            setClickingTogglesState(false);

            if (isPlaying)
            {
                stopPlaying();
            }

            if (regionEditorWindow != nullptr)
            {
                regionEditorWindow.deleteAndZero(); //close editor window
            }

            DBG("SegmentedRegion not interactable");
            break;

        case SegmentedRegionStateIndex::editable:
            nonInstantStateFound = true;

            setToggleState(false, juce::NotificationType::dontSendNotification);
            setToggleable(false);
            setClickingTogglesState(false);

            DBG("SegmentedRegion editable");
            break;

        case SegmentedRegionStateIndex::playable:
            nonInstantStateFound = true;

            setClickingTogglesState(shouldBeToggleable); //selected exclusively for the playable state, also automatically sets isToggleable
            setToggleState(isPlaying, juce::NotificationType::dontSendNotification);

            DBG("SegmentedRegion playable");
            break;

        default:
            throw std::exception("unhandled state index");
        }
    }

    //finally, update the current state index
    currentStateIndex = stateToTransitionTo;
    currentState = states[static_cast<int>(currentStateIndex)];
}

void SegmentedRegion::setShouldBeToggleable(bool newShouldBeToggleable)
{
    shouldBeToggleable = newShouldBeToggleable;
}
bool SegmentedRegion::getShouldBeToggleable()
{
    return shouldBeToggleable;
}

void SegmentedRegion::triggerButtonStateChanged()
{
    juce::Button::buttonStateChanged();
}
void SegmentedRegion::triggerDrawableButtonStateChanged()
{
    juce::DrawableButton::buttonStateChanged();
}

void SegmentedRegion::clicked(const juce::ModifierKeys& modifiers)
{
    if (modifiers.isLeftButtonDown())
    {
        return currentState->clicked(modifiers);
    }
}

void SegmentedRegion::setBuffer(juce::AudioSampleBuffer newBuffer, juce::String fileName, double origSampleRate)
{
    //if (audioFileName != "")
    //{
    //    //delete old voices
    //    associatedVoices.clear();
    //    audioEngine->removeVoicesWithID(getID());
    //}

    buffer = newBuffer;
    audioFileName = fileName;

    if (audioFileName != "")
    {
        if (associatedVoices.size() == 0)
        {
            audioEngine->initialiseVoicesForRegion(getID()); //may create more than 1 voice
            associatedVoices = audioEngine->getVoicesWithID(getID());
        }

        //update buffers
        for (auto itVoice = associatedVoices.begin(); itVoice != associatedVoices.end(); itVoice++)
        {
            (*itVoice)->setOsc(newBuffer, origSampleRate); //
        }
    }
    else
    {
        //set to empty buffer
        auto emptyBuffer = juce::AudioSampleBuffer();
        for (auto itVoice = associatedVoices.begin(); itVoice != associatedVoices.end(); itVoice++)
        {
            (*itVoice)->setOsc(emptyBuffer, 0);
        }
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

bool SegmentedRegion::isEditorOpen()
{
    return regionEditorWindow != nullptr;
}
void SegmentedRegion::sendEditorToFront()
{
    regionEditorWindow->toFront(true);
}
void SegmentedRegion::openEditor()
{
    regionEditorWindow = juce::Component::SafePointer<RegionEditorWindow>(new RegionEditorWindow("Region " + juce::String(ID) + " Editor", this));
}

void SegmentedRegion::startPlaying()
{
    if (audioFileName != "" && !isPlaying)
    {
        DBG("*plays music*");
        isPlaying = true;

        associatedVoices[currentVoiceIndex]->startNote(0, 1.0f, audioEngine->getSynth()->getSound(0).get(), 64); //cycle through all voices (incremented after this voice stops)
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
        associatedVoices[currentVoiceIndex]->stopNote(1.0f, true); //cycles through all voices bit by bit
        currentVoiceIndex = (currentVoiceIndex + 1) % associatedVoices.size(); //next time, play the next voice
        //audioEngine->getSynth()->noteOff(1, 64, 1.0f, true);
        //audioEngine->getSynth()->getVoice(voiceIndex)->stopNote(1.0f, true);
        isPlaying = false;

        //stopTimer(); //stopped in the drawing method since the LFO will have to keep being drawn for a little longer because of release time
    }
}

RegionLfo* SegmentedRegion::getAssociatedLfo()
{
    return associatedLfo;
}

AudioEngine* SegmentedRegion::getAudioEngine()
{
    return audioEngine;
}

juce::Array<Voice*> SegmentedRegion::getAssociatedVoices()
{
    juce::Array<Voice*> output;
    output.addArray(associatedVoices);
    return output;
}
juce::String SegmentedRegion::getFileName()
{
    return audioFileName;
}




//protected

void SegmentedRegion::buttonStateChanged() //void handleButtonStateChanged() //void clicked(const juce::ModifierKeys& modifiers) override
{
    currentState->buttonStateChanged();
}
