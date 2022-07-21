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
class RegionLfo : public Lfo
{
public:
    enum Polarity : int
    {
        unipolar, //LFO values lie within [0,1].
        bipolar //LFO values lie within [-1,1]. This is the default.
    };

    RegionLfo(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable, int regionID);

    void setWaveTable(const juce::AudioBuffer<float>& waveTable, Polarity polarityOfPassedWaveTable);

    void prepare(const juce::dsp::ProcessSpec& spec) override;

    void transitionToState(RegionLfoStateIndex stateToTransitionTo);

    std::function<void(float semitonesToAdd)> getFrequencyModulationFunction();

    juce::Array<int> getAffectedRegionIDs();
    juce::Array<LfoModulatableParameter> getModulatedParameterIDs();

    void addRegionModulation(const juce::Array<Voice*>& newRegionVoices, const std::function<void(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)>& newModulationFunction, LfoModulatableParameter newModulatedParameterID);
    void removeRegionModulation(int regionID);

    int getRegionID();

    void advance() override;
    void advanceUnsafeWithUpdate();
    void advanceUnsafeWithoutUpdate();
    std::function<void()> getAdvancerFunction();

protected:
    int regionID;

    RegionLfoState* states[static_cast<int>(RegionLfoStateIndex::StateIndexCount)]; //fixed size -> more efficient access

    static const RegionLfoStateIndex initialStateIndex = RegionLfoStateIndex::unprepared;
    RegionLfoStateIndex currentStateIndex;
    RegionLfoState* currentState = nullptr; //provides at least one less array lookup during each access (i.e. every sample)

    juce::OwnedArray<juce::Array<Voice*>> affectedVoices; //one array per modulated region (contains all voices of that region)
    juce::Array<std::function<void(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)>> modulationFunctions; //one function per modulated region; having both polarities' values in the function makes it easy and efficient (no ifs or pointers necessary!) to choose between them when defining the modulation function
    juce::Array<LfoModulatableParameter> modulatedParameterIDs;

    juce::AudioBuffer<float> waveTableUnipolar;

    float currentValueUnipolar = 0.0f;
    float currentValueBipolar = 0.0f;

    float depth = 1.0f; //make this modulatable later

    void updateCurrentValues();

    void updateModulatedParameter() override;

    void calculateUnipolarWaveTable();
    void calculateBipolarWaveTable();
};
