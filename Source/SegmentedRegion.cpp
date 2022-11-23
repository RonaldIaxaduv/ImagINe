/*
  ==============================================================================

    SegmentedRegion.cpp
    Created: 8 Jul 2022 10:03:18am
    Author:  Aaron

  ==============================================================================
*/

#include "SegmentedRegion.h"


//constants
const float SegmentedRegion::lfoLineThickness = 8.0f; //4.0f;
const float SegmentedRegion::focusRadius = 2.5f;
const float SegmentedRegion::outlineThickness = 1.0f;
const float SegmentedRegion::inherentTransparency = 0.70f;
const float SegmentedRegion::disabledTransparency = 0.20f;
const float SegmentedRegion::disabledTransparencyOutline = 0.50f;

//public

SegmentedRegion::SegmentedRegion(const juce::Path& outline, const juce::Rectangle<float>& relativeBounds, const juce::Rectangle<int>& parentBounds, juce::Colour fillColour, AudioEngine* audioEngine) :
    juce::DrawableButton("", ButtonStyle::ImageStretched),
    p(outline),
    relativeBounds(relativeBounds),
    fillColour(fillColour), currentLfoLine()
{
    regionEditorWindow = nullptr;

    this->audioEngine = audioEngine;
    ID = audioEngine->addNewRegion(fillColour, this); //also generates the region's LFO and all its Voice instances.
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
    
    setBufferedToImage(true);
    setMouseClickGrabsKeyboardFocus(false);
    //setRepaintsOnMouseActivity(false); //doesn't work for DrawableButton sadly (but makes kind of sense since it needs to change its background image while interacting with it)
    //currentLfoLine = juce::Line<float>(juce::Point<float>(0.0f, 0.0f), juce::Point<float>(static_cast<float>(getWidth()), static_cast<float>(getHeight()))); //diagonal -> entire region will be redrawn (a little hacky, but ensures that the LFO line is drawn before the region is first played)
    //repaint(); //paints the LFO line
    setSize(parentBounds.getWidth(), parentBounds.getHeight());
}

SegmentedRegion::~SegmentedRegion()
{
    DBG("destroying SegmentedRegion...");

    stopTimer();

    if (regionEditorWindow != nullptr)
    {
        //close editor window
        regionEditorWindow.deleteAndZero();
    }

    //release states
    currentState = nullptr;
    delete states[static_cast<int>(SegmentedRegionStateIndex::notInteractable)];
    delete states[static_cast<int>(SegmentedRegionStateIndex::editable)];
    delete states[static_cast<int>(SegmentedRegionStateIndex::playable)];
    int deletedStates = 3;
    jassert(deletedStates == static_cast<int>(SegmentedRegionStateIndex::StateIndexCount));

    //release LFO
    associatedLfo = nullptr;
    audioEngine->removeLfo(getID()); //exception freeing heap after the LFO has been destroyed -> some invalid member?

    //release Voice(s)
    associatedVoices.clear();
    audioEngine->removeVoicesWithID(getID());

    //release audio engine
    audioEngine = nullptr;

    //~DrawableButton(); //this caused dangling pointers in the past -> try not to call if at all possible!

    DBG("SegmentedRegion destroyed.");
}

void SegmentedRegion::initialiseImages()
{
    if (getWidth() == 0 || getHeight() == 0)
    {
        DBG("no region size set yet. assigning temporary values.");
        setSize(static_cast<int>(500.0f * relativeBounds.getWidth()),
            static_cast<int>(500.0f * relativeBounds.getHeight())); //must be set before the images are drawn, because drawables require bounds to be displayed! the values don't really matter; they are only temporary.
    }
    else
    {
        DBG("region size already set, no temporary values required.");
    }

    DBG("initial size of segmented region: " + getBounds().toString());
    DBG("bounds of the passed path: " + p.getBounds().toString());

    juce::Colour invertedFillColour = juce::Colour::fromRGBA(255 - fillColour.getRed(), 255 - fillColour.getGreen(), 255 - fillColour.getBlue(), 255);

    normalImage.setPath(p);
    normalImage.setFill(juce::FillType(fillColour.withAlpha(inherentTransparency)));
    //normalImage.setBufferedToImage(true) <- WIP: do this? would this improve performance?
    //normalImage.setHasFocusOutline
    normalImage.setStrokeThickness(outlineThickness);
    normalImage.setStrokeFill(juce::FillType(normalImage.getFill().colour.withAlpha(1.0f).contrasting()));
    normalImage.setBounds(getBounds());

    overImage.setPath(p);
    overImage.setFill(juce::FillType(fillColour.brighter(0.2f).withAlpha(inherentTransparency)));
    overImage.setStrokeThickness(outlineThickness);
    overImage.setStrokeFill(juce::FillType(overImage.getFill().colour.withAlpha(1.0f).contrasting()));
    overImage.setBounds(getBounds());

    downImage.setPath(p);
    downImage.setFill(juce::FillType(fillColour.darker(0.2f).withAlpha(inherentTransparency)));
    downImage.setStrokeThickness(outlineThickness);
    //downImage.setStrokeFill(juce::FillType(downImage.getFill().colour.withAlpha(1.0f).contrasting()));
    downImage.setStrokeFill(juce::FillType(invertedFillColour.darker(0.2f)));
    downImage.setBounds(getBounds());

    disabledImage.setPath(p);
    disabledImage.setFill(juce::FillType(fillColour.withAlpha(disabledTransparency)));
    disabledImage.setStrokeThickness(outlineThickness);
    disabledImage.setStrokeFill(juce::FillType(disabledImage.getFill().colour.withAlpha(1.0f).contrasting().withAlpha(disabledTransparencyOutline)));
    disabledImage.setBounds(getBounds());


    normalImageOn.setPath(p);
    normalImageOn.setFill(juce::FillType(fillColour.darker(0.4f).withAlpha(inherentTransparency)));
    normalImageOn.setStrokeThickness(outlineThickness);
    //normalImageOn.setStrokeFill(juce::FillType(normalImageOn.getFill().colour.withAlpha(1.0f).contrasting()));
    normalImageOn.setStrokeFill(juce::FillType(invertedFillColour.darker(0.4f)));
    normalImageOn.setBounds(getBounds());

    overImageOn.setPath(p);
    overImageOn.setFill(juce::FillType(fillColour.darker(0.2f).withAlpha(inherentTransparency)));
    overImageOn.setStrokeThickness(outlineThickness);
    //overImageOn.setStrokeFill(juce::FillType(overImageOn.getFill().colour.withAlpha(1.0f).contrasting()));
    overImageOn.setStrokeFill(juce::FillType(invertedFillColour.darker(0.2f)));
    overImageOn.setBounds(getBounds());

    downImageOn.setPath(p);
    downImageOn.setFill(juce::FillType(fillColour.darker(0.6f).withAlpha(inherentTransparency)));
    downImageOn.setStrokeThickness(outlineThickness);
    //downImageOn.setStrokeFill(juce::FillType(downImageOn.getFill().colour.withAlpha(1.0f).contrasting()));
    downImageOn.setStrokeFill(juce::FillType(invertedFillColour.darker(0.6f)));
    downImageOn.setBounds(getBounds());

    disabledImageOn.setPath(p);
    disabledImageOn.setFill(juce::FillType(fillColour.darker(0.4f).withAlpha(disabledTransparency)));
    disabledImageOn.setStrokeThickness(outlineThickness);
    //disabledImageOn.setStrokeFill(juce::FillType(disabledImageOn.getFill().colour.withAlpha(1.0f).contrasting().withAlpha(disabledTransparency)));
    disabledImageOn.setStrokeFill(juce::FillType(invertedFillColour.darker(0.4).withAlpha(disabledTransparencyOutline)));
    disabledImageOn.setBounds(getBounds());

    setImages(&normalImage, &overImage, &downImage, &disabledImage, &normalImageOn, &overImageOn, &downImageOn, &disabledImageOn);

    setColour(ColourIds::backgroundOnColourId, juce::Colours::transparentBlack); //otherwise, the button has a grey background colour while the button is toggled on
    lfoLineColour = juce::Colour::contrasting(fillColour, fillColour.contrasting());
    focusPointColour = fillColour.contrasting();
}

void SegmentedRegion::timerCallback()
{
    //repaint(); //this is highly inefficient because it redraws lots of unchanged space

    if (associatedLfo == nullptr)
    {
        DBG("NO LFO SET YET"); //if the code arrives here, this method might need a state-dependent implementation...
        return;
    }
    
    //calculate area previously taken on by currentLfoLine
    juce::Rectangle<float> previousArea(juce::jmin(currentLfoLine.getStartX(), currentLfoLine.getEndX()),
        juce::jmin(currentLfoLine.getStartY(), currentLfoLine.getEndY()),
        juce::jmax(currentLfoLine.getStartX(), currentLfoLine.getEndX()) - juce::jmin(currentLfoLine.getStartX(), currentLfoLine.getEndX()),
        juce::jmax(currentLfoLine.getStartY(), currentLfoLine.getEndY()) - juce::jmin(currentLfoLine.getStartY(), currentLfoLine.getEndY()));

    //update currentLfoLine
    //float curLfoPhase = associatedLfo->getLatestModulatedPhase(); //basically the same value as getModulatedValue of the parameter, but won't update that parameter (which would mess with the modulation)
    float curLfoPhase = associatedLfo->getPhase(); //basically the same value as getModulatedValue of the parameter, but won't update that parameter (which would mess with the modulation)
    juce::Point<float> outlinePt = p.getPointAlongPath(curLfoPhase * p.getLength(), juce::AffineTransform(), juce::Path::defaultToleranceForMeasurement);
    currentLfoLine = juce::Line<float>(focusAbs.x, focusAbs.y,
                                       outlinePt.x, outlinePt.y);

    juce::Rectangle<float> currentArea(juce::jmin(currentLfoLine.getStartX(), currentLfoLine.getEndX()),
                                       juce::jmin(currentLfoLine.getStartY(), currentLfoLine.getEndY()),
                                       juce::jmax(currentLfoLine.getStartX(), currentLfoLine.getEndX()) - juce::jmin(currentLfoLine.getStartX(), currentLfoLine.getEndX()),
                                       juce::jmax(currentLfoLine.getStartY(), currentLfoLine.getEndY()) - juce::jmin(currentLfoLine.getStartY(), currentLfoLine.getEndY()));

    //repaint(previousArea.expanded(lfoLineThickness, lfoLineThickness).toNearestInt());
    //repaint(currentArea.expanded(lfoLineThickness, lfoLineThickness).toNearestInt());
    repaint(previousArea.getUnion(currentArea).expanded(lfoLineThickness, lfoLineThickness).toNearestInt()); //this might be faster than the above, because paintOverChildren might be called twice otherwise(?)
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

    if (associatedLfo == nullptr)
    {
        stopTimer();
        DBG("STOP!");
        return;
    }

    //draw line from focus point to point on the outline that corresponds to the associated LFO's current phase
    float effectiveLfoLineThickness = juce::jmax(1.0f, lfoLineThickness * associatedLfo->getDepth()); //LFOs with a higher depth have thicker LFO lines
    if (isPlaying)
    {
        //draw LFO line
        g.setColour(focusPointColour); //for creating an outline
        g.drawLine(currentLfoLine, effectiveLfoLineThickness);
        g.setColour(lfoLineColour); //main line
        g.drawLine(currentLfoLine, effectiveLfoLineThickness * 0.8f);
    }
    else
    {
        //draw LFO line, but with reduced alpha
        g.setColour(focusPointColour.withAlpha(0.5f)); //for creating an outline
        g.drawLine(currentLfoLine, effectiveLfoLineThickness);
        g.setColour(lfoLineColour.withAlpha(0.5f)); //main line
        g.drawLine(currentLfoLine, effectiveLfoLineThickness * 0.8f);
    }

    //draw focus point (looks better when drawn after the line)
    g.setColour(focusPointColour.contrasting());
    g.fillEllipse(focusAbs.x - focusRadius,
        focusAbs.y - focusRadius,
        2.0f * focusRadius,
        2.0f * focusRadius);
    g.setColour(focusPointColour);
    g.fillEllipse(focusAbs.x - focusRadius * 0.75,
                  focusAbs.y - focusRadius * 0.75,
                  1.5f * focusRadius,
                  1.5f * focusRadius);

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

    //update (actual) focus point position
    focusAbs = juce::Point<float>(focus.x * getBounds().getWidth(),
                                  focus.y * getBounds().getHeight());

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

void SegmentedRegion::forceRepaint()
{
    SegmentedRegionStateIndex previousStateIndex = currentStateIndex;

    transitionToState(SegmentedRegionStateIndex::editable); //trick: transition to a state where hitTest doesn't always return false but actually corresponds to the region's hitbox
    repaint(); //now repainting should work <- actually, it doesn't... there's not enough of a delay...

    transitionToState(previousStateIndex);
}

juce::String SegmentedRegion::getTooltip()
{
    switch (currentStateIndex)
    {
    case SegmentedRegionStateIndex::editable:
        return "Click on this region to open its editor window. You may have editor windows of other regions open at the same time.";

    //case SegmentedRegionStateIndex::playable:
    //    return "Click on this region to play it. If it's toggleable, it will keep playing until you click it again.";
        //^- i'm pretty sure this would only get annoying

    default:
        return "";
    }
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
void SegmentedRegion::transitionToState(SegmentedRegionStateIndex stateToTransitionTo, bool keepPlayingAndEditing)
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

            if (!keepPlayingAndEditing)
            {
                if (isPlaying)
                {
                    stopPlaying();
                }

                if (regionEditorWindow != nullptr)
                {
                    regionEditorWindow.deleteAndZero(); //close editor window
                }
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

    if (currentStateIndex == SegmentedRegionStateIndex::playable)
    {
        setClickingTogglesState(shouldBeToggleable); //selected exclusively for the playable state, also automatically sets isToggleable
    }
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
    this->origSampleRate = origSampleRate;

    bool wasSuspended = audioEngine->isSuspended();
    audioEngine->suspendProcessing(true);
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
            (*itVoice)->setOsc(newBuffer, origSampleRate); //update sampler file in every voice
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
    audioEngine->suspendProcessing(wasSuspended);

    DBG("new buffer has been set. length: " + juce::String(origSampleRate > 0.0 ? static_cast<double>(buffer.getNumSamples()) / origSampleRate : 0.0) + " seconds.");
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
    bool wasSuspended = audioEngine->isSuspended();
    if (audioEngine->getLfo(ID) == nullptr) //lfo not yet initialised
    {
        associatedLfo = new RegionLfo(waveform, RegionLfo::Polarity::unipolar, getID()); //no modulation until the voice has been initialised
        
        audioEngine->suspendProcessing(true);
        audioEngine->addLfo(associatedLfo);
    }
    else
    {
        audioEngine->suspendProcessing(true);
        associatedLfo->setWaveTable(waveform, RegionLfo::Polarity::unipolar);
    }
    audioEngine->suspendProcessing(wasSuspended);

    float maxLength = std::sqrtf(static_cast<float>(getParentWidth() * getParentWidth() + getParentHeight() * getParentHeight())); //diagonal of the image (-> longest line)
    float maxDepthCutoff = 0.4f; //ratio of maxLength required to reach depth=1
    float actualLength = (std::sqrtf(range.getEnd()) - std::sqrtf(range.getStart())); //subtract (ignore) the minimum length, because the minimum only adds a constant offset
    actualLength = actualLength / maxLength; //ratio of maxLength
    float newDepth = juce::jmin(actualLength, maxDepthCutoff) / maxDepthCutoff;
    associatedLfo->setDepth(newDepth);
    DBG("new depth: " + juce::String(associatedLfo->getDepth()));

    DBG("LFO's waveform has been rendered.");
}

int SegmentedRegion::getID()
{
    return ID;
}
bool SegmentedRegion::tryChangeID(int newID)
{
    if (audioEngine->tryChangeRegionID(ID, newID))
    {
        //the new ID wasn't already taken and the IDs of the LFO and all voices have been adjusted
        ID = newID;
        return true;
    }
    else
    {
        DBG("The ID could not be changed.");
        return false;
    }
}

juce::Point<float> SegmentedRegion::getFocusPoint()
{
    return focus;
}
void SegmentedRegion::setFocusPoint(juce::Point<float> newFocusPoint) //does not recalculate wavetable!
{
    focus = newFocusPoint;
    focusAbs = juce::Point<float>(focus.x * getBounds().getWidth(),
                                  focus.y * getBounds().getHeight());
    timerCallback(); //redraws LFO line
}

bool SegmentedRegion::isEditorOpen()
{
    return regionEditorWindow != nullptr;
}
void SegmentedRegion::sendEditorToFront()
{
    regionEditorWindow->toFront(true);
    regionEditorWindow->getContentComponent()->grabKeyboardFocus();
}
void SegmentedRegion::openEditor()
{
    regionEditorWindow = juce::Component::SafePointer<RegionEditorWindow>(new RegionEditorWindow("Region " + juce::String(ID) + " Editor", this));
    regionEditorWindow->getContentComponent()->grabKeyboardFocus();
}
void SegmentedRegion::refreshEditor()
{
    if (regionEditorWindow != nullptr)
    {
        regionEditorWindow->refreshEditor();
    }
}

void SegmentedRegion::setIsPlaying_Click(bool shouldBePlaying)
{
    isPlaying_click = shouldBePlaying;
}
bool SegmentedRegion::getIsPlaying_Click()
{
    return isPlaying_click;
}
bool SegmentedRegion::shouldBePlaying()
{
    return isPlaying_click || isPlaying_courier || isPlaying_midi;
}
void SegmentedRegion::startPlaying(bool toggleButtonState)
{
    if (audioFileName != "" && !isPlaying && shouldBePlaying()) //audio file is set, and the region isn't playing yet, but it was requested to do so
    {
        DBG("*plays music*");
        isPlaying = true;
        
        //DBG("voice: " + juce::String(currentVoiceIndex + 1) + " / " + juce::String(associatedVoices.size()));

        associatedVoices[currentVoiceIndex]->startNote(0, 1.0f, audioEngine->getSynth()->getSound(0).get(), 64); //cycle through all voices (incremented after this voice stops)
        //audioEngine->getSynth()->noteOn(1, 64, 1.0f); //might be worth a thought for later because of polyphony, but since voices will then be chosen automatically, adjustments to the voices class would have to be made
        //audioEngine->getSynth()->getVoice(voiceIndex)->startNote(0, 1.0f, audioEngine->getSynth()->getSound(0).get(), 64);

        //try to set the button's toggle state to "down" (needs to be done cross-thread for MIDI messages because they do not run on the same thread as couriers and clicks)
        if (toggleButtonState)
        {
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            {
                setToggleState(true, juce::NotificationType::dontSendNotification);
                //setState(juce::Button::ButtonState::buttonDown);
            }
            else
            {
                juce::MessageManager::getInstance()->callFunctionOnMessageThread([](void* data)
                    {
                        static_cast<SegmentedRegion*>(data)->setToggleState(true, juce::NotificationType::dontSendNotification);
                        return static_cast<void*>(nullptr);
                    }, this);
            }
        }

        startTimer(timerIntervalMs); //animates the LFO line
    }
}
void SegmentedRegion::stopPlaying(bool toggleButtonState)
{
    if (isPlaying && !shouldBePlaying()) //region is playing, but it was requested that it shouldn't do so anymore. (the audio file needn't be checked because the region cannot start playing without the file having been checked beforehand.)
    {
        DBG("*stops music*");
        associatedVoices[currentVoiceIndex]->stopNote(1.0f, true); //cycles through all voices bit by bit
        currentVoiceIndex = (currentVoiceIndex + 1) % associatedVoices.size(); //next time, play the next voice
        //audioEngine->getSynth()->noteOff(1, 64, 1.0f, true);
        //audioEngine->getSynth()->getVoice(voiceIndex)->stopNote(1.0f, true);
        isPlaying = false;

        //try to set the button's toggle state to "up" (needs to be done cross-thread for MIDI messages because they do not run on the same thread as couriers and clicks)
        if (toggleButtonState)
        {
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            {
                setToggleState(false, juce::NotificationType::dontSendNotification);
                //setState(juce::Button::ButtonState::buttonNormal);
            }
            else
            {
                juce::MessageManager::getInstance()->callFunctionOnMessageThread([](void* data)
                    {
                        static_cast<SegmentedRegion*>(data)->setToggleState(false, juce::NotificationType::dontSendNotification);
                        return static_cast<void*>(nullptr);
                    }, this);
            }
        }

        //stopTimer(); //stopped in the drawing method since the LFO will have to keep being drawn for a little longer because of release time
    }
}

int SegmentedRegion::getMidiChannel()
{
    return midiChannel;
}
void SegmentedRegion::setMidiChannel(int newMidiChannel)
{
    midiChannel = newMidiChannel;
}
int SegmentedRegion::getMidiNote()
{
    return noteNumber;
}
void SegmentedRegion::setMidiNote(int newNoteNumber)
{
    noteNumber = newNoteNumber;
}
void SegmentedRegion::handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    //DBG("registered ON note: " + juce::String(midiChannel) + " " + juce::String(midiNoteNumber));

    //LONG VERSION (easier to understand, but has too many if cases):
    //if (this->midiChannel >= 0 && noteNumber >= 0)
    //{
    //    //region is setup to receive MIDI (WIP: maybe make this into a state - this isn't too urgent, though, because MIDI messages are rare compared to samples)

    //    if (this->midiChannel == 0 || (this->midiChannel == midiChannel))
    //    {
    //        //region listens to the given MIDI channel -> evaluate note

    //        if (noteNumber == midiNoteNumber)
    //        {
    //            //note number matches with the one that the region responds to
    //            if (shouldBeToggleable && isPlaying_midi)
    //            {
    //                isPlaying_midi = false;
    //                stopPlaying();
    //            }
    //            else
    //            {
    //                isPlaying_midi = true;
    //                startPlaying();
    //            }
    //        }
    //    }
    //}

    //SHORT VERSION (combines several of the above if cases into one case to improve performance):
    if ((this->midiChannel >= 0 && noteNumber >= 0) && (this->midiChannel == 0 || (this->midiChannel == midiChannel)) && (noteNumber == midiNoteNumber))
    {
        //region is set up to receive MIDI, and the incoming MIDI corresponds to that which the region recognises
        if (shouldBeToggleable && isPlaying_midi)
        {
            //setToggleState(!getToggleState(), juce::NotificationType::sendNotificationAsync); //WIP: this would be cleaner, but requires a message manager (throws as assert)
            isPlaying_midi = false;
            stopPlaying();
        }
        else
        {
            isPlaying_midi = true;
            startPlaying();
        }
        
    }
}
void SegmentedRegion::handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    //DBG("registered OFF note: " + juce::String(midiChannel) + " " + juce::String(midiNoteNumber));

    //LONG VERSION (easier to understand, but has too many if cases):
    //if (this->midiChannel >= 0 && noteNumber >= 0)
    //{
    //    //region is setup to receive MIDI (WIP: maybe make this into a state - this isn't too urgent, though, because MIDI messages are rare compared to samples)

    //    if (this->midiChannel == 0 || (this->midiChannel == midiChannel))
    //    {
    //        //region listens to the given MIDI channel -> evaluate note

    //        if (noteNumber == midiNoteNumber)
    //        {
    //            //note number matches with the one that the region responds to
    //            
    //            if (!isToggleable())
    //            {
    //                isPlaying_midi = false;
    //                stopPlaying();
    //            }
    //        }
    //    }
    //}

    //SHORT VERSION (combines several of the above if cases into one case to improve performance):
    if ((this->midiChannel >= 0 && noteNumber >= 0) && (this->midiChannel == 0 || (this->midiChannel == midiChannel)) && (noteNumber == midiNoteNumber) && (!isToggleable()))
    {
        //region is set up to receive MIDI, and the incoming MIDI corresponds to that which the region recognises
        isPlaying_midi = false;
        stopPlaying();
    }
}

void SegmentedRegion::signalCourierEntered()
{
    if (currentCourierCount++ == 0)
    {
        //only attempt playing if there weren't any couriers before (note: doesn't play if the region was already set to playing through the region playing mode)
        isPlaying_courier = true;
        startPlaying();
    }
}
void SegmentedRegion::signalCourierLeft()
{
    if (--currentCourierCount == 0)
    {
        //only attempt to stop playing if there are no couriers left
        isPlaying_courier = false;
        stopPlaying();
    }
}
void SegmentedRegion::resetCouriers()
{
    if (currentCourierCount > 0)
    {
        isPlaying_courier = false;
        stopPlaying();
    }
    currentCourierCount = 0;
}
int SegmentedRegion::getNumCouriers()
{
    return currentCourierCount;
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

bool SegmentedRegion::serialise(juce::XmlElement* xmlRegion, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("serialising SegmentedRegion...");
    bool serialisationSuccessful = true;

    xmlRegion->setAttribute("ID", ID);
    xmlRegion->setAttribute("shouldBeToggleable", shouldBeToggleable);
    xmlRegion->setAttribute("path", p.toString());
    xmlRegion->setAttribute("fillColour", fillColour.toString());

    juce::XmlElement* xmlRelativeBounds = xmlRegion->createNewChildElement("relativeBounds");
    xmlRelativeBounds->setAttribute("x", relativeBounds.getX());
    xmlRelativeBounds->setAttribute("y", relativeBounds.getY());
    xmlRelativeBounds->setAttribute("width", relativeBounds.getWidth());
    xmlRelativeBounds->setAttribute("height", relativeBounds.getHeight());

    juce::XmlElement* xmlFocus = xmlRegion->createNewChildElement("focus");
    xmlFocus->setAttribute("x", focus.getX());
    xmlFocus->setAttribute("y", focus.getY());

    xmlRegion->setAttribute("midiChannel", midiChannel);
    xmlRegion->setAttribute("noteNumber", noteNumber);

    xmlRegion->setAttribute("audioFileName", audioFileName);
    xmlRegion->setAttribute("origSampleRate", origSampleRate);
    xmlRegion->setAttribute("polyphony", associatedVoices.size()); //it's better to store this here than in the AudioEngine methods, because here, the Voice classes' oscs can be directly updated with the corresponding buffer



    //store buffer in attachedData
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    size_t bufferMemorySize = static_cast<size_t>(numChannels * numSamples) * sizeof(float);

    if (bufferMemorySize > 0)
    {
        //serialise buffer
        juce::MemoryBlock bufferMemory(bufferMemorySize);
        juce::MemoryOutputStream bufferStream (bufferMemory, false); //by using this stream, the samples can be written in a certain endian (unlike memBuffer.append()), ensuring portability. little endian will be used.

        //prepend size and number of the channels so that they can be read correctly
        bufferStream.writeInt(numChannels);
        bufferStream.writeInt(numSamples);
        bufferStream.flush();

        //copy the content of the buffer - unfortunately, the channels can't be written as a whole, so it has to be done sample-by-sample...
        for (int ch = 0; ch < numChannels; ++ch)
        {
            //bufferMemory.append(buffer.getReadPointer(ch), static_cast<size_t>(numSamples) * sizeof(float)); //while this would be able to copy a full channel, it wouldn't ensure a specific endian, causing portability issues!
            auto* samples = buffer.getReadPointer(ch);

            for (int s = 0; s < numSamples; ++s)
            {
                bufferStream.writeFloat(samples[s]);
            }
            bufferStream.flush();
        }

        attachedData->add(bufferMemory);
        xmlRegion->setAttribute("bufferMemory_index", attachedData->size() - 1);
    }
    else //bufferMemorySize == 0
    {
        //no buffer contained -> nothing to serialise
        xmlRegion->setAttribute("bufferMemory_index", -1); //nothing attached
    }

    DBG(juce::String(serialisationSuccessful ? "SegmentedRegion has been serialised." : "SegmentedRegion could not be serialised."));
    return serialisationSuccessful;
}
bool SegmentedRegion::deserialise(juce::XmlElement* xmlRegion, juce::Array<juce::MemoryBlock>* attachedData)
{
    DBG("deserialising SegmentedRegion...");
    bool deserialisationSuccessful = true;

    deserialisationSuccessful = tryChangeID(xmlRegion->getIntAttribute("ID", -1));
    if (deserialisationSuccessful)
    {
        shouldBeToggleable = xmlRegion->getBoolAttribute("shouldBeToggleable", false);
        p.clear();
        p.restoreFromString(xmlRegion->getStringAttribute("path", ""));
        fillColour = juce::Colour::fromString(xmlRegion->getStringAttribute("fillColour", juce::Colours::black.toString()));

        midiChannel = xmlRegion->getIntAttribute("midiChannel", -1);
        noteNumber = xmlRegion->getIntAttribute("noteNumber", -1);

        juce::XmlElement* xmlRelativeBounds = xmlRegion->getChildByName("relativeBounds");
        if (xmlRelativeBounds != nullptr)
        {
            relativeBounds.setBounds(xmlRelativeBounds->getDoubleAttribute("x", 0.0),
                xmlRelativeBounds->getDoubleAttribute("y", 0.0),
                xmlRelativeBounds->getDoubleAttribute("width", 0.0),
                xmlRelativeBounds->getDoubleAttribute("height", 0.0)
            );
        }
        else
        {
            DBG("no relativeBounds data found.");
            relativeBounds.setBounds(0.0, 0.0, 0.0, 0.0);
            deserialisationSuccessful = false;
        }

        if (deserialisationSuccessful)
        {
            juce::XmlElement* xmlFocus = xmlRegion->getChildByName("focus");
            if (xmlFocus != nullptr)
            {
                focus.setXY(xmlFocus->getDoubleAttribute("x", 0.5),
                    xmlFocus->getDoubleAttribute("y", 0.5)
                );
            }
            else
            {
                DBG("no focus data found.");
                focus.setXY(0.5, 0.5);
                //deserialisationSuccessful = false; //not problematic
            }

            if (deserialisationSuccessful)
            {
                audioFileName = xmlRegion->getStringAttribute("audioFileName", "");
                origSampleRate = xmlRegion->getDoubleAttribute("origSampleRate", 0.0);

                //apply polyphony (buffers will be updated later)
                int polyphony = xmlRegion->getIntAttribute("polyphony", 1);
                if (associatedVoices.size() != polyphony)
                {
                    audioEngine->removeVoicesWithID(getID());
                    audioEngine->initialiseVoicesForRegion(getID(), polyphony); //initialises the given amount of voices for this region
                    associatedVoices = audioEngine->getVoicesWithID(getID()); //update associated voices
                }

                //restore buffer from attachedData
                int bufferMemoryIndex = xmlRegion->getIntAttribute("bufferMemory_index", -1);
                if (bufferMemoryIndex >= 0 && bufferMemoryIndex < attachedData->size())
                {
                    //buffer data contained in attachedData -> get block and restore
                    juce::MemoryBlock bufferMemory = (*attachedData)[bufferMemoryIndex];
                    juce::MemoryInputStream bufferStream(bufferMemory, false); //by using this stream, the samples can be read in a certain endian, ensuring portability. little endian will be used.

                    //get the size and number of the channels so that they can be read correctly
                    int numChannels = bufferStream.readInt();
                    int numSamples = bufferStream.readInt();
                    juce::AudioSampleBuffer tempBuffer(numChannels, numSamples);

                    //copy the content of the buffer - unfortunately, the channels can't be written as a whole, so it has to be done sample-by-sample...
                    for (int ch = 0; ch < numChannels; ++ch)
                    {
                        //bufferMemory.append(buffer.getReadPointer(ch), static_cast<size_t>(numSamples) * sizeof(float)); //while this would be able to copy a full channel, it wouldn't ensure a specific endian, causing portability issues!
                        auto* samples = tempBuffer.getWritePointer(ch);

                        for (int s = 0; s < numSamples; ++s)
                        {
                            samples[s] = bufferStream.readFloat();
                        }
                    }

                    setBuffer(tempBuffer, audioFileName, origSampleRate); //correctly updates associated voices, too
                }
                else
                {
                    //buffer data not contained in attachedData (buffer was probably empty)

                    DBG("buffer not contained in attachedData. this might be because the buffer was simply empty.");
                    audioFileName = "";
                    origSampleRate = 0.0;
                    setBuffer(juce::AudioSampleBuffer(), "", 0.0); //sets buffer to be empty. correctly updates associated voices, too
                }
            }
        }
    }



    //data has been read. now, re-initialise all remaining components.
    initialiseImages(); //re-initialises all images
    renderLfoWaveform(); //updates LFO (generates its wavetable)
    resized(); //updates focusAbs, calculates the current lfoLine and redraws the component (or rather, it's *supposed* to redraw it...)

    DBG(juce::String(deserialisationSuccessful ? "SegmentedRegion has been deserialised." : "SegmentedRegion could not be deserialised."));
    return deserialisationSuccessful;
}




//protected

void SegmentedRegion::buttonStateChanged() //void handleButtonStateChanged() //void clicked(const juce::ModifierKeys& modifiers) override
{
    currentState->buttonStateChanged();
}

void SegmentedRegion::mouseDown(const juce::MouseEvent& e)
{
    if (isEnabled())
    {
        juce::DrawableButton::mouseDown(e);
    }
    else
    {
        getParentComponent()->mouseDown(e.getEventRelativeTo(getParentComponent())); //when the region is disabled, pass clicks back to the segmentable image (setInterceptsMouseClicks(false, false) doesn't do anything for DrawableButtons sadly...)
    }
}
void SegmentedRegion::mouseUp(const juce::MouseEvent& e)
{
    if (isEnabled())
    {
        juce::DrawableButton::mouseUp(e);
    }
    else
    {
        getParentComponent()->mouseUp(e.getEventRelativeTo(getParentComponent())); //when the region is disabled, pass clicks back to the segmentable image (setInterceptsMouseClicks(false, false) doesn't do anything for DrawableButtons sadly...)
    }
}
