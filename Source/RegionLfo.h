/*
  ==============================================================================

    RegionLfo.h
    Created: 29 Jun 2022 1:47:03pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Lfo.h"

#include "RegionLfoStateIndex.h"

//forward references to the envelope states (required to enable circular dependencies - see https://stackoverflow.com/questions/994253/two-classes-that-refer-to-each-other )
#include "RegionLfoStates.h"
class RegionLfoState;
class RegionLfoState_Unprepared;
class RegionLfoState_WithoutWaveTable;
class RegionLfoState_WithoutModulatedParameters;
class RegionLfoState_Muted;
class RegionLfoState_Active;

#include "ModulatableParameter.h"


/// <summary>
/// an LFO which is associated with a SegmentedRegion and its Voice(s)
/// </summary>
class RegionLfo final : public Lfo
{
public:
    enum Polarity : int
    {
        unipolar, //LFO values lie within [0,1].
        bipolar //LFO values lie within [-1,1]. This is the default.
    };

    RegionLfo(int regionID);
    RegionLfo(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable, int regionID);
    ~RegionLfo();

    void setWaveTable(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable);

    void prepare(const juce::dsp::ProcessSpec& spec) override;

    void transitionToState(RegionLfoStateIndex stateToTransitionTo);

    juce::Array<int> getAffectedRegionIDs();
    juce::Array<LfoModulatableParameter> getModulatedParameterIDs();
    int getNumModulatedParameterIDs();

    ModulatableAdditiveParameter<double>* getFrequencyModParameter();
    ModulatableMultiplicativeParameter<double>* getPhaseModParameter();
    ModulatableMultiplicativeParameter<double>* getUpdateIntervalParameter();

    float getModulationDepth();

    void addRegionModulation(LfoModulatableParameter newModulatedParameterID, int newRegionID, const juce::Array<ModulatableParameter<double>*>& newParameters);
    void removeRegionModulation(int regionID);

    int getRegionID();

    void advance() override;
    void advanceUnsafeWithUpdate();
    void advanceUnsafeWithoutUpdate();

    void setPhase(float relativeTablePos) override;
    float getPhase() override;
    void resetPhase(bool updateParameters = true) override;
    void resetPhaseUnsafe_WithoutUpdate();
    void resetPhaseUnsafe_WithUpdate();
    float getLatestModulatedPhase();

    void updateCurrentValues();

    double getCurrentValue_Unipolar();
    double getCurrentValue_Bipolar();

    int samplesUntilUpdate = 0; //see updateInterval variable
    void resetSamplesUntilUpdate();
    void setUpdateInterval_Milliseconds(float newUpdateIntervalMs);
    float getUpdateInterval_Milliseconds();
    void prepareUpdateInterval();
    double getMsUntilUpdate();

protected:
    int regionID; //ID of the region that this LFO is associated with

    //states
    RegionLfoState* states[static_cast<int>(RegionLfoStateIndex::StateIndexCount)]; //fixed size -> more efficient access

    static const RegionLfoStateIndex initialStateIndex = RegionLfoStateIndex::unprepared;
    RegionLfoStateIndex currentStateIndex;
    RegionLfoState* currentState = nullptr; //provides at least one less array lookup during each access (i.e. every sample)

    bool isPrepared();

    //modulated parameters
    juce::OwnedArray<juce::Array<ModulatableParameter<double>*>> modulatedParameters; //may modulate several voices of one region - hence, the additional wrapping array
    juce::Array<LfoModulatableParameter> modulatedParameterIDs;
    juce::Array<int> affectedRegionIDs;

    //unipolar/bipolar stuff
    juce::AudioBuffer<float> waveTableUnipolar; //the Lfo::waveTable member is treated as being bipolar

    float currentValueUnipolar = 0.0f;
    float currentValueBipolar = 0.0f;

    //other
    ModulatableAdditiveParameter<double> frequencyModParameter; //replaces the frequency modulation members of LFO
    void evaluateFrequencyModulation() override;

    ModulatableMultiplicativeParameter<double> phaseModParameter;
    float latestModulatedPhase = 1.0f;

    ModulatableMultiplicativeParameter<double> updateIntervalParameter;

    float depth = 1.0f; //intensity of the modulation

    int updateInterval = 0; //the LFO doesn't update with every sample. instead, a certain time interval (in samples) needs to pass until another update happens. higher values should generally decrease CPU usage.
    float updateIntervalMs = 0.0f; //update interval in milliseconds

    void updateModulatedParameter() override;
    void updateModulatedParameterUnsafe();

    void calculateUnipolarWaveTable();
    void calculateBipolarWaveTable();
};
