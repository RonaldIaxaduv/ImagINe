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

#include "Voice.h"
struct Voice;

#include "RegionLfoStateIndex.h"

//forward references to the envelope states (required to enable circular dependencies - see https://stackoverflow.com/questions/994253/two-classes-that-refer-to-each-other )
#include "RegionLfoStates.h"
class RegionLfoState;
class RegionLfoState_Unprepared;
class RegionLfoState_WithoutWaveTable;
class RegionLfoState_WithoutModulatedParameters;
class RegionLfoState_Muted;
class RegionLfoState_Active;


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

    RegionLfo(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable, int regionID);
    ~RegionLfo();

    void setWaveTable(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable);

    void prepare(const juce::dsp::ProcessSpec& spec) override;

    void transitionToState(RegionLfoStateIndex stateToTransitionTo);

    juce::Array<int> getAffectedRegionIDs();
    juce::Array<LfoModulatableParameter> getModulatedParameterIDs();
    int getNumModulatedParameterIDs();

    void addRegionModulation(LfoModulatableParameter newModulatedParameterID, const juce::Array<Voice*>& newRegionVoices);
    void addRegionModulation(LfoModulatableParameter newModulatedParameterID, RegionLfo* newRegionLfo);
    void removeRegionModulation(int regionID);

    int getRegionID();

    void advance() override;
    void advanceUnsafeWithUpdate();
    void advanceUnsafeWithoutUpdate();

    int samplesUntilUpdate = 0; //see updateInterval variable
    void resetSamplesUntilUpdate();
    //void setUpdateInterval_Samples(int newUpdateInterval);
    //int getUpdateInterval_Samples();
    void setUpdateInterval_Milliseconds(float newUpdateIntervalMs);
    float getUpdateInterval_Milliseconds();
    void prepareUpdateInterval();

protected:
    int regionID;

    //states
    RegionLfoState* states[static_cast<int>(RegionLfoStateIndex::StateIndexCount)]; //fixed size -> more efficient access

    static const RegionLfoStateIndex initialStateIndex = RegionLfoStateIndex::unprepared;
    RegionLfoStateIndex currentStateIndex;
    RegionLfoState* currentState = nullptr; //provides at least one less array lookup during each access (i.e. every sample)

    bool isPrepared();

    //modulated parameters
    juce::Array<LfoModulatableParameter> modulatedVoiceParameterIDs;
    typedef void(*voiceModFunctionPt)(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v);
    juce::Array<voiceModFunctionPt> voiceModulationFunctions;
    juce::OwnedArray<juce::Array<Voice*>> affectedVoices; //one array per entry of voiceModulationFunctions (contains all voices of one single region)

    juce::Array<LfoModulatableParameter> modulatedLfoParameterIDs;
    typedef void(*lfoModFunctionPt)(float lfoValueUnipolar, float lfoValueBipolar, float depth, RegionLfo* lfo);
    juce::Array<lfoModFunctionPt> lfoModulationFunctions; //one function per modulated lfo
    juce::Array<RegionLfo*> affectedLfos; //one per entry of lfoModulationFunctions

    //unipolar/bipolar stuff
    juce::AudioBuffer<float> waveTableUnipolar; //the Lfo::waveTable member is treated as being bipolar

    float currentValueUnipolar = 0.0f;
    float currentValueBipolar = 0.0f;

    //other
    float depth = 1.0f; //make this modulatable later

    int updateInterval = 0; //the LFO doesn't update with every sample. instead, a certain time interval (in samples) needs to pass until another update happens. higher values should generally decrease CPU usage.
    float updateIntervalMs = 0.0f; //update interval in milliseconds

    void updateCurrentValues();

    void updateModulatedParameter() override;
    void updateModulatedParameterUnsafe();

    void calculateUnipolarWaveTable();
    void calculateBipolarWaveTable();
};
