/*
  ==============================================================================

    RegionLfoStates.h
    Created: 14 Jul 2022 8:37:32am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "RegionLfoStateIndex.h"

#include "RegionLfo.h"
class RegionLfo;

class RegionLfoState
{
public:
    RegionLfoState(RegionLfo& lfo);

    //envelope events to implement
    virtual void prepared(double newSampleRate) = 0;

    virtual void waveTableSet(int numSamples) = 0;
    virtual void modulationDepthChanged(float newDepth) = 0;
    virtual void modulatedParameterCountChanged(int newCount) = 0;

    virtual void advance() = 0;

    //virtual void updateModulatedParameter() = 0;

protected:
    RegionLfoStateIndex implementedRegionLfoStateIndex = RegionLfoStateIndex::StateIndexCount; //corresponds to the index of the state that the derived class implements
    RegionLfo& lfo; //used to transition to new states. quicker than std::function and even a function pointer (would be an additional function call vs. a direct call when using a reference)
};

class RegionLfoState_Unprepared final : public RegionLfoState
{
public:
    RegionLfoState_Unprepared(RegionLfo& lfo);
    ~RegionLfoState_Unprepared();

    void prepared(double newSampleRate) override;

    void waveTableSet(int numSamples) override;
    void modulationDepthChanged(float newDepth) override;
    void modulatedParameterCountChanged(int newCount) override;

    void advance() override;

    //void updateModulatedParameter() override;
};

class RegionLfoState_WithoutWaveTable final : public RegionLfoState
{
public:
    RegionLfoState_WithoutWaveTable(RegionLfo& lfo);
    ~RegionLfoState_WithoutWaveTable();

    void prepared(double newSampleRate) override;

    void waveTableSet(int numSamples) override;
    void modulationDepthChanged(float newDepth) override;
    void modulatedParameterCountChanged(int newCount) override;

    void advance() override;

    //void updateModulatedParameter() override;
};

class RegionLfoState_WithoutModulatedParameters final : public RegionLfoState
{
public:
    RegionLfoState_WithoutModulatedParameters(RegionLfo& lfo);
    ~RegionLfoState_WithoutModulatedParameters();

    void prepared(double newSampleRate) override;

    void waveTableSet(int numSamples) override;
    void modulationDepthChanged(float newDepth) override;
    void modulatedParameterCountChanged(int newCount) override;

    void advance() override;

    //void updateModulatedParameter() override;
};

class RegionLfoState_Muted final : public RegionLfoState
{
public:
    RegionLfoState_Muted(RegionLfo& lfo);
    ~RegionLfoState_Muted();

    void prepared(double newSampleRate) override;

    void waveTableSet(int numSamples) override;
    void modulationDepthChanged(float newDepth) override;
    void modulatedParameterCountChanged(int newCount) override;

    void advance() override;

    //void updateModulatedParameter() override;
};

class RegionLfoState_Active final : public RegionLfoState
{
public:
    RegionLfoState_Active(RegionLfo& lfo);
    ~RegionLfoState_Active();

    void prepared(double newSampleRate) override;

    void waveTableSet(int numSamples) override;
    void modulationDepthChanged(float newDepth) override;
    void modulatedParameterCountChanged(int newCount) override;

    void advance() override;

    //void updateModulatedParameter() override;
};

class RegionLfoState_ActiveRealTime final : public RegionLfoState //same as RegionLfoState_Active, except that advance() uses 1 if case less (samplesUntilUpdate needn't be checked)
{
public:
    RegionLfoState_ActiveRealTime(RegionLfo& lfo);
    ~RegionLfoState_ActiveRealTime();

    void prepared(double newSampleRate) override;

    void waveTableSet(int numSamples) override;
    void modulationDepthChanged(float newDepth) override;
    void modulatedParameterCountChanged(int newCount) override;

    void advance() override;

    //void updateModulatedParameter() override;
};
