/*
  ==============================================================================

    SegmentableImageStates.cpp
    Created: 7 Oct 2022 3:28:15pm
    Author:  Aaron

  ==============================================================================
*/

#include "SegmentableImageStates.h"


SegmentableImageState::SegmentableImageState(SegmentableImage& image) :
    image(image)
{ }




#pragma region SegmentableImageState_Empty

SegmentableImageState_Empty::SegmentableImageState_Empty(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::empty;
}

void SegmentableImageState_Empty::imageChanged(const juce::Image& newImage)
{
    if (!newImage.isNull())
    {
        DBG("valid image loaded.");
        image.transitionToState(SegmentableImageStateIndex::withImage);
    }
    else
    {
        DBG("invalid image passed.");
    }
}

void SegmentableImageState_Empty::mouseDown(const juce::MouseEvent& event)
{
    //nothing to interact with clicks
}

bool SegmentableImageState_Empty::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    if (key.getTextCharacter() == 'o')
    {
        //WIP: open file dialogue
    }

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_Empty




#pragma region SegmentableImageState_WithImage

SegmentableImageState_WithImage::SegmentableImageState_WithImage(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::withImage;
}

void SegmentableImageState_WithImage::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
    }
}

void SegmentableImageState_WithImage::mouseDown(const juce::MouseEvent& event)
{
    //nothing to do yet
}

bool SegmentableImageState_WithImage::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());
    
    //no key bindings for this state atm

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_WithImage




#pragma region SegmentableImageState_DrawingRegion

SegmentableImageState_DrawingRegion::SegmentableImageState_DrawingRegion(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::drawingRegion;
}

void SegmentableImageState_DrawingRegion::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
        image.clearPlayPaths();
        image.transitionToState(SegmentableImageStateIndex::drawingRegion);
    }
}

void SegmentableImageState_DrawingRegion::mouseDown(const juce::MouseEvent& event)
{
    if (image.hasStartedDrawing())
    {
        //add point to path
        image.addPointToPath(event.position);
    }
    else
    {
        //start new path
        image.startNewPath(event.position);
    }
}

bool SegmentableImageState_DrawingRegion::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    if (key == juce::KeyPress::backspaceKey)
    {
        image.deleteLastNode();
        return true;
    }
    else if (key == juce::KeyPress::escapeKey)
    {
        image.resetPath();
        return true;
    }
    else if (key.getTextCharacter() == 'o')
    {
        image.tryCompletePath_Region();
        return true;
    }
    else if (key == juce::KeyPress::deleteKey)
    {
        //try to delete region (if there is any here)
        auto mousePos = image.getMouseXYRelative();
        juce::Component* region = image.getComponentAt(mousePos.getX(), mousePos.getY());

        if (dynamic_cast<SegmentedRegion*>(region) != nullptr)
        {
            //-> the pointed-at object is indeed a play path.

            //define message box and its callback
            void(*f)(int, SegmentableImage*, int) = [](int result, SegmentableImage* image, int regionID)
            {
                if (result > 0)
                {
                    image->removeRegion(regionID);
                }
            };
            juce::ModalComponentManager::Callback* cb = juce::ModalCallbackFunction::withParam(f, &image, static_cast<SegmentedRegion*>(region)->getID());


            auto result = juce::NativeMessageBox::showYesNoBox(juce::MessageBoxIconType::WarningIcon,
                "Delete Region?",
                "Are you sure that you would like to delete Region " + juce::String(static_cast<SegmentedRegion*>(region)->getID()) + "?",
                region,
                cb);

            return true;
        }
    }

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_DrawingRegion




#pragma region SegmentableImageState_EditingRegions

SegmentableImageState_EditingRegions::SegmentableImageState_EditingRegions(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::editingRegions;
}

void SegmentableImageState_EditingRegions::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
        image.clearPlayPaths();
    }
}

void SegmentableImageState_EditingRegions::mouseDown(const juce::MouseEvent& event)
{
    //nothing to interact with clicks (regions will intercept clicks)
}

bool SegmentableImageState_EditingRegions::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    if (key == juce::KeyPress::deleteKey)
    {
        //try to delete region (if there is any here)
        auto mousePos = image.getMouseXYRelative();
        juce::Component* region = image.getComponentAt(mousePos.getX(), mousePos.getY());

        if (dynamic_cast<SegmentedRegion*>(region) != nullptr)
        {
            //-> the pointed-at object is indeed a region.

            //define message box and its callback
            void(*f)(int, SegmentableImage*, int) = [](int result, SegmentableImage* image, int regionID)
            {
                if (result > 0)
                {
                    image->removeRegion(regionID);
                }
            };
            juce::ModalComponentManager::Callback* cb = juce::ModalCallbackFunction::withParam(f, &image, static_cast<SegmentedRegion*>(region)->getID());


            auto result = juce::NativeMessageBox::showYesNoBox(juce::MessageBoxIconType::WarningIcon,
                                                              "Delete Region?",
                                                              "Are you sure that you would like to delete Region " + juce::String(static_cast<SegmentedRegion*>(region)->getID()) + "?",
                                                              region,
                                                              cb);

            return true;
        }
    }

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_EditingRegions




#pragma region SegmentableImageState_PlayingRegions

SegmentableImageState_PlayingRegions::SegmentableImageState_PlayingRegions(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::playingRegions;
}

void SegmentableImageState_PlayingRegions::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
        image.clearPlayPaths();
    }
}

void SegmentableImageState_PlayingRegions::mouseDown(const juce::MouseEvent& event)
{
    //nothing to interact with clicks (active regions will intercept clicks)
}

bool SegmentableImageState_PlayingRegions::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    //currently no key bindings for this state

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_PlayingRegions




#pragma region SegmentableImageState_DrawingPlayPath

SegmentableImageState_DrawingPlayPath::SegmentableImageState_DrawingPlayPath(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::drawingPlayPath;
}

void SegmentableImageState_DrawingPlayPath::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
        image.clearPlayPaths();
        image.transitionToState(SegmentableImageStateIndex::drawingPlayPath);
    }
}

void SegmentableImageState_DrawingPlayPath::mouseDown(const juce::MouseEvent& event)
{
    if (image.hasStartedDrawing())
    {
        //add point to path
        image.addPointToPath(event.position);
    }
    else
    {
        //start new path
        image.startNewPath(event.position);
    }
}

bool SegmentableImageState_DrawingPlayPath::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    if (key == juce::KeyPress::backspaceKey)
    {
        image.deleteLastNode();
        return true;
    }
    else if (key == juce::KeyPress::escapeKey)
    {
        image.resetPath();
        return true;
    }
    else if (key.getTextCharacter() == 'o')
    {
        image.tryCompletePath_PlayPath();
        return true;
    }
    else if (key == juce::KeyPress::deleteKey)
    {
        //try to delete play path (if there is any here)
        auto mousePos = image.getMouseXYRelative();
        juce::Component* playPath = image.getComponentAt(mousePos.getX(), mousePos.getY());

        if (dynamic_cast<PlayPath*>(playPath) != nullptr)
        {
            //-> the pointed-at object is indeed a play path.

            //define message box and its callback
            void(*f)(int, SegmentableImage*, int) = [](int result, SegmentableImage* image, int pathID)
            {
                if (result > 0)
                {
                    image->removePlayPath(pathID);
                }
            };
            juce::ModalComponentManager::Callback* cb = juce::ModalCallbackFunction::withParam(f, &image, static_cast<PlayPath*>(playPath)->getID());


            auto result = juce::NativeMessageBox::showYesNoBox(juce::MessageBoxIconType::WarningIcon,
                                                               "Delete Play Path?",
                                                               "Are you sure that you would like to delete Play Path " + juce::String(static_cast<PlayPath*>(playPath)->getID()) + "?",
                                                               playPath,
                                                               cb);

            return true;
        }
    }

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_DrawingPlayPath




#pragma region SegmentableImageState_EditingPlayPaths

SegmentableImageState_EditingPlayPaths::SegmentableImageState_EditingPlayPaths(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::editingPlayPaths;
}

void SegmentableImageState_EditingPlayPaths::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
        image.clearPlayPaths();
    }
}

void SegmentableImageState_EditingPlayPaths::mouseDown(const juce::MouseEvent& event)
{
    //nothing to interact with clicks (play paths will intercept clicks)
}

bool SegmentableImageState_EditingPlayPaths::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    if (key == juce::KeyPress::deleteKey)
    {
        //try to delete play path (if there is any here)
        auto mousePos = image.getMouseXYRelative();
        juce::Component* playPath = image.getComponentAt(mousePos.getX(), mousePos.getY());

        if (dynamic_cast<PlayPath*>(playPath) != nullptr)
        {
            //define message box and its callback
            void(*f)(int, SegmentableImage*, int) = [](int result, SegmentableImage* image, int pathID)
            {
                if (result > 0)
                {
                    image->removePlayPath(pathID);
                }
            };
            juce::ModalComponentManager::Callback* cb = juce::ModalCallbackFunction::withParam(f, &image, static_cast<PlayPath*>(playPath)->getID());


            auto result = juce::NativeMessageBox::showYesNoBox(juce::MessageBoxIconType::WarningIcon,
                                                               "Delete Play Path?",
                                                               "Are you sure that you would like to delete Play Path " + juce::String(static_cast<PlayPath*>(playPath)->getID()) + "?",
                                                               playPath,
                                                               cb);

            return true;
        }
    }

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_EditingPlayPaths




#pragma region SegmentableImageState_PlayingPlayPaths

SegmentableImageState_PlayingPlayPaths::SegmentableImageState_PlayingPlayPaths(SegmentableImage& image) :
    SegmentableImageState(image)
{
    implementedStateIndex = SegmentableImageStateIndex::playingPlayPaths;
}

void SegmentableImageState_PlayingPlayPaths::imageChanged(const juce::Image& newImage)
{
    if (newImage.isNull())
    {
        DBG("invalid new image passed. clearing content and resetting to empty.");
        image.transitionToState(SegmentableImageStateIndex::empty); //automatically clears content
    }
    else
    {
        DBG("valid new image loaded. clearing previous content.");
        image.resetPath();
        image.clearRegions();
        image.clearPlayPaths();
    }
}

void SegmentableImageState_PlayingPlayPaths::mouseDown(const juce::MouseEvent& event)
{
    //nothing to interact with clicks (active play paths will intercept clicks)
}

bool SegmentableImageState_PlayingPlayPaths::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    /*switch (key.getKeyCode())
    {
    case juce::KeyPress::createFromDescription("o").getKeyCode():

    }*/

    DBG(key.getTextCharacter());

    //currently no key bindings for this state

    //else
    DBG("unhandled key");
    return false;
}

#pragma endregion SegmentableImageState_PlayingPlayPaths
