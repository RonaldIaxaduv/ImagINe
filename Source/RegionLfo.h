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

    //std::function<void(float semitonesToAdd)> getFrequencyModulationFunction();

    juce::Array<int> getAffectedRegionIDs();
    juce::Array<LfoModulatableParameter> getModulatedParameterIDs();
    int getNumModulatedParameterIDs();

    //void addRegionModulation(const juce::Array<Voice*>& newRegionVoices, const std::function<void(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)>& newModulationFunction, LfoModulatableParameter newModulatedParameterID);
    void addRegionModulation(LfoModulatableParameter newModulatedParameterID, const juce::Array<Voice*>& newRegionVoices);
    void addRegionModulation(LfoModulatableParameter newModulatedParameterID, RegionLfo* newRegionLfo);
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

    //juce::Array<std::function<void(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)>> modulationFunctions; //one function per modulated region; having both polarities' values in the function makes it easy and efficient (no ifs or pointers necessary!) to choose between them when defining the modulation function

    juce::Array<LfoModulatableParameter> modulatedVoiceParameterIDs;
    typedef void(*voiceModFunctionPt)(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v);
    //typedef juce::HeapBlock<void(*)(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)>::Type> voiceModFunctionElement;
    //juce::Array<void(*)(float lfoValueUnipolar, float lfoValueBipolar, float depth, Voice* v)> voiceModulationFunctions; //THIS CURRENTLY GIVES EXCEPTIONS BECAUSE IT DOESN'T ACTUALLY STORE THE VOICE POINTERS FOR SOME REASON... //one function per modulated voice; having both polarities' values in the function makes it easy and efficient (no ifs or pointers necessary!) to choose between them when defining the modulation function
    //std::list<voiceModFunctionPt> voiceModulationFunctions; //one function per modulated voice; having both polarities' values in the function makes it easy and efficient (no ifs or pointers necessary!) to choose between them when defining the modulation function
    juce::Array<voiceModFunctionPt> voiceModulationFunctions;
    //voiceModFunctionPt[] voiceModulationFunctions;
    juce::OwnedArray<juce::Array<Voice*>> affectedVoices; //one array per entry of voiceModulationFunctions (contains all voices of one single region)

    juce::Array<LfoModulatableParameter> modulatedLfoParameterIDs;
    typedef void(*lfoModFunctionPt)(float lfoValueUnipolar, float lfoValueBipolar, float depth, RegionLfo* lfo);
    juce::Array<lfoModFunctionPt> lfoModulationFunctions; //one function per modulated lfo
    //std::list<lfoModFunctionPt> lfoModulationFunctions; //one function per modulated lfo
    juce::Array<RegionLfo*> affectedLfos; //one per entry of lfoModulationFunctions

    juce::AudioBuffer<float> waveTableUnipolar; //the Lfo::waveTable member is treated as being bipolar

    float currentValueUnipolar = 0.0f;
    float currentValueBipolar = 0.0f;

    float depth = 1.0f; //make this modulatable later

    void updateCurrentValues();

    void updateModulatedParameter() override;
    void updateModulatedParameterUnsafe();

    void calculateUnipolarWaveTable();
    void calculateBipolarWaveTable();
};
