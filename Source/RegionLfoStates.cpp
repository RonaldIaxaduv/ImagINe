/*
  ==============================================================================

    RegionLfoStates.cpp
    Created: 14 Jul 2022 8:37:32am
    Author:  Aaron

  ==============================================================================
*/

#include "RegionLfoStates.h"


RegionLfoState::RegionLfoState(RegionLfo& lfo) :
    lfo(lfo)
{ }




#pragma region RegionLfoState_Unprepared

RegionLfoState_Unprepared::RegionLfoState_Unprepared(RegionLfo& lfo) :
    RegionLfoState(lfo)
{
    implementedRegionLfoStateIndex = RegionLfoStateIndex::unprepared;
}

RegionLfoState_Unprepared::~RegionLfoState_Unprepared()
{ }

void RegionLfoState_Unprepared::prepared(double newSampleRate)
{
    if (newSampleRate > 0.0)
    {
        lfo.transitionToState(RegionLfoStateIndex::withoutWaveTable);
    }
}

void RegionLfoState_Unprepared::waveTableSet(int numSamples)
{
    //doesn't count as preparing even if the wavetable has samples, because the sample rate hasn't been set yet, i.e. the LFO's frequency cannot be calculated yet.
    //when the LFO has been prepared, it will automatically skip the WithoutWaveTable state though if the wavetable has already been set
}

void RegionLfoState_Unprepared::modulationDepthChanged(float newDepth)
{
    //doesn't matter while unprepared
}

void RegionLfoState_Unprepared::modulatedParameterCountChanged(int newCount)
{
    //doesn't matter while unprepared
}

void RegionLfoState_Unprepared::advance()
{
    //doesn't do anything while unprepared
}

float RegionLfoState_Unprepared::getPhase()
{
    return 0.0f; //no wavetable set yet
}
void RegionLfoState_Unprepared::setPhase(float relativeTablePos)
{
    //no wavetable set yet
}
void RegionLfoState_Unprepared::resetPhase(bool updateParameters)
{
    //no wavetable set yet
}

//void RegionLfoState_Unprepared::updateModulatedParameter()
//{
//    //doesn't do anything while unprepared
//}

#pragma endregion RegionLfoState_Unprepared




#pragma region RegionLfoState_WithoutWaveTable

RegionLfoState_WithoutWaveTable::RegionLfoState_WithoutWaveTable(RegionLfo& lfo) :
    RegionLfoState(lfo)
{
    implementedRegionLfoStateIndex = RegionLfoStateIndex::withoutWaveTable;
}

RegionLfoState_WithoutWaveTable::~RegionLfoState_WithoutWaveTable()
{ }

void RegionLfoState_WithoutWaveTable::prepared(double newSampleRate)
{
    if (newSampleRate <= 0.0)
    {
        lfo.transitionToState(RegionLfoStateIndex::unprepared);
    }
}

void RegionLfoState_WithoutWaveTable::waveTableSet(int numSamples)
{
    if (numSamples > 0)
    {
        lfo.transitionToState(RegionLfoStateIndex::withoutModulatedParameters);
    }
}

void RegionLfoState_WithoutWaveTable::modulationDepthChanged(float newDepth)
{
    //doesn't matter while wave table hasn't been set
}

void RegionLfoState_WithoutWaveTable::modulatedParameterCountChanged(int newCount)
{
    //doesn't matter while wave table hasn't been set
}

void RegionLfoState_WithoutWaveTable::advance()
{
    //doesn't do anything while wave table hasn't been set
}

float RegionLfoState_WithoutWaveTable::getPhase()
{
    return 0.0f; //no wavetable set yet
}
void RegionLfoState_WithoutWaveTable::setPhase(float relativeTablePos)
{
    //no wavetable set yet
}
void RegionLfoState_WithoutWaveTable::resetPhase(bool updateParameters)
{
    //no wavetable set yet
}

//void RegionLfoState_WithoutWaveTable::updateModulatedParameter()
//{
//    //doesn't do anything while wave table hasn't been set
//}

#pragma endregion RegionLfoState_WithoutWaveTable




#pragma region RegionLfoState_WithoutModulatedParameters

RegionLfoState_WithoutModulatedParameters::RegionLfoState_WithoutModulatedParameters(RegionLfo& lfo) :
    RegionLfoState(lfo)
{
    implementedRegionLfoStateIndex = RegionLfoStateIndex::withoutModulatedParameters;
}

RegionLfoState_WithoutModulatedParameters::~RegionLfoState_WithoutModulatedParameters()
{ }

void RegionLfoState_WithoutModulatedParameters::prepared(double newSampleRate)
{
    if (newSampleRate <= 0.0)
    {
        lfo.transitionToState(RegionLfoStateIndex::unprepared);
    }
}

void RegionLfoState_WithoutModulatedParameters::waveTableSet(int numSamples)
{
    if (numSamples == 0)
    {
        lfo.transitionToState(RegionLfoStateIndex::withoutWaveTable);
    }
}

void RegionLfoState_WithoutModulatedParameters::modulationDepthChanged(float newDepth)
{
    //doesn't matter while no modulated parameters have been set
}

void RegionLfoState_WithoutModulatedParameters::modulatedParameterCountChanged(int newCount)
{
    if (newCount > 0)
    {
        lfo.transitionToState(RegionLfoStateIndex::active); //automatically switches to muted if depth is zero
    }
}

void RegionLfoState_WithoutModulatedParameters::advance()
{
    lfo.advanceUnsafeWithoutUpdate(); //needs to update phase (displayed on the region!), but no update necessary

    if (!lfo.samplesUntilUpdate--) //update phase when samplesUntilUpdate == 0 (important to keep the LFO line updated)
    {
        lfo.updateLatestModulatedPhase();
        lfo.resetSamplesUntilUpdate();
    }
}

float RegionLfoState_WithoutModulatedParameters::getPhase()
{
    //return lfo.getCurrentTablePos() / static_cast<float>(lfo.getNumSamplesUnsafe());
    return lfo.getLatestModulatedPhase();
}
void RegionLfoState_WithoutModulatedParameters::setPhase(float relativeTablePos)
{
    lfo.setCurrentTablePos(relativeTablePos);
}
void RegionLfoState_WithoutModulatedParameters::resetPhase(bool updateParameters)
{
    lfo.resetPhaseUnsafe_WithoutUpdate(); //no parameters yet so no need to do updates
}

//void RegionLfoState_WithoutModulatedParameters::updateModulatedParameter()
//{
//    //doesn't do anything while no modulated parameters have been set
//}

#pragma endregion RegionLfoState_WithoutModulatedParameters




#pragma region RegionLfoState_Muted

RegionLfoState_Muted::RegionLfoState_Muted(RegionLfo& lfo) :
    RegionLfoState(lfo)
{
    implementedRegionLfoStateIndex = RegionLfoStateIndex::muted;
}

RegionLfoState_Muted::~RegionLfoState_Muted()
{ }

void RegionLfoState_Muted::prepared(double newSampleRate)
{
    if (newSampleRate <= 0.0)
    {
        lfo.transitionToState(RegionLfoStateIndex::unprepared);
    }
}

void RegionLfoState_Muted::waveTableSet(int numSamples)
{
    if (numSamples == 0)
    {
        lfo.transitionToState(RegionLfoStateIndex::withoutWaveTable);
    }
}

void RegionLfoState_Muted::modulationDepthChanged(float newDepth)
{
    if (newDepth > 0.0f)
    {
        lfo.transitionToState(RegionLfoStateIndex::active);
    }
}

void RegionLfoState_Muted::modulatedParameterCountChanged(int newCount)
{
    if (newCount == 0)
    {
        lfo.transitionToState(RegionLfoStateIndex::withoutModulatedParameters);
    }
}

void RegionLfoState_Muted::advance()
{
    lfo.advanceUnsafeWithoutUpdate(); //leaves out if cases (for samples in the wave table) and doesn't update the modulated value

    if (!lfo.samplesUntilUpdate--) //update phase when samplesUntilUpdate == 0 (important to keep the LFO line updated)
    {
        lfo.updateLatestModulatedPhase();
        lfo.resetSamplesUntilUpdate();
    }
}

float RegionLfoState_Muted::getPhase()
{
    //return lfo.getCurrentTablePos() / static_cast<float>(lfo.getNumSamplesUnsafe());
    return lfo.getLatestModulatedPhase();
}
void RegionLfoState_Muted::setPhase(float relativeTablePos)
{
    lfo.setCurrentTablePos(relativeTablePos);
}
void RegionLfoState_Muted::resetPhase(bool updateParameters)
{
    lfo.resetPhaseUnsafe_WithoutUpdate(); //muted, so no need to do updates
}

//void RegionLfoState_Muted::updateModulatedParameter()
//{
//    //doesn't do anything while depth is zero
//}

#pragma endregion RegionLfoState_Muted




#pragma region RegionLfoState_Active

RegionLfoState_Active::RegionLfoState_Active(RegionLfo& lfo) :
    RegionLfoState(lfo)
{
    implementedRegionLfoStateIndex = RegionLfoStateIndex::active;
}

RegionLfoState_Active::~RegionLfoState_Active()
{ }

void RegionLfoState_Active::prepared(double newSampleRate)
{
    if (newSampleRate <= 0.0)
    {
        lfo.transitionToState(RegionLfoStateIndex::unprepared);
    }
}

void RegionLfoState_Active::waveTableSet(int numSamples)
{
    if (numSamples == 0)
    {
        lfo.transitionToState(RegionLfoStateIndex::withoutWaveTable);
    }
}

void RegionLfoState_Active::modulationDepthChanged(float newDepth)
{
    if (newDepth == 0.0f)
    {
        lfo.transitionToState(RegionLfoStateIndex::muted);
    }
}

void RegionLfoState_Active::modulatedParameterCountChanged(int newCount)
{
    if (newCount == 0)
    {
        lfo.transitionToState(RegionLfoStateIndex::withoutModulatedParameters);
    }
}

void RegionLfoState_Active::advance()
{
    if (lfo.samplesUntilUpdate--) //only update when samplesUntilUpdate == 0
    {
        lfo.advanceUnsafeWithoutUpdate();
    }
    else
    {
        lfo.advanceUnsafeWithUpdate(); //also updates the latest modulated phase
        lfo.resetSamplesUntilUpdate();
    }
}

float RegionLfoState_Active::getPhase()
{
    //return lfo.getCurrentTablePos() / static_cast<float>(lfo.getNumSamplesUnsafe());
    return lfo.getLatestModulatedPhase();
}
void RegionLfoState_Active::setPhase(float relativeTablePos)
{
    lfo.setCurrentTablePos(relativeTablePos);
}
void RegionLfoState_Active::resetPhase(bool updateParameters)
{
    if (updateParameters)
    {
        lfo.resetPhaseUnsafe_WithUpdate();
    }
    else
    {
        lfo.resetPhaseUnsafe_WithoutUpdate();
    }

}

//void RegionLfoState_Active::updateModulatedParameter()
//{
//    updateParameterFunc();
//}

#pragma endregion RegionLfoState_Active




#pragma region RegionLfoState_ActiveRealTime

RegionLfoState_ActiveRealTime::RegionLfoState_ActiveRealTime(RegionLfo& lfo) :
    RegionLfoState(lfo)
{
    implementedRegionLfoStateIndex = RegionLfoStateIndex::active;
}

RegionLfoState_ActiveRealTime::~RegionLfoState_ActiveRealTime()
{ }

void RegionLfoState_ActiveRealTime::prepared(double newSampleRate)
{
    if (newSampleRate <= 0.0)
    {
        lfo.transitionToState(RegionLfoStateIndex::unprepared);
    }
}

void RegionLfoState_ActiveRealTime::waveTableSet(int numSamples)
{
    if (numSamples == 0)
    {
        lfo.transitionToState(RegionLfoStateIndex::withoutWaveTable);
    }
}

void RegionLfoState_ActiveRealTime::modulationDepthChanged(float newDepth)
{
    if (newDepth == 0.0f)
    {
        lfo.transitionToState(RegionLfoStateIndex::muted);
    }
}

void RegionLfoState_ActiveRealTime::modulatedParameterCountChanged(int newCount)
{
    if (newCount == 0)
    {
        lfo.transitionToState(RegionLfoStateIndex::withoutModulatedParameters);
    }
}

void RegionLfoState_ActiveRealTime::advance()
{
    //doesn't need to check for samplesUntilUpdate -> saves 1 if case
    lfo.advanceUnsafeWithUpdate();
}

float RegionLfoState_ActiveRealTime::getPhase()
{
    //return lfo.getCurrentTablePos() / static_cast<float>(lfo.getNumSamplesUnsafe());
    return lfo.getLatestModulatedPhase();
}
void RegionLfoState_ActiveRealTime::setPhase(float relativeTablePos)
{
    lfo.setCurrentTablePos(relativeTablePos);
}
void RegionLfoState_ActiveRealTime::resetPhase(bool updateParameters)
{
    if (updateParameters)
    {
        lfo.resetPhaseUnsafe_WithUpdate();
    }
    else
    {
        lfo.resetPhaseUnsafe_WithoutUpdate();
    }

}

//void RegionLfoState_ActiveRealTime::updateModulatedParameter()
//{
//    updateParameterFunc();
//}

#pragma endregion RegionLfoState_ActiveRealTime
