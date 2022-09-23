/*
  ==============================================================================

    ModulatableParameter.h
    Created: 21 Sep 2022 9:25:17am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "ModulatableParameterStateIndex.h"
#include "ModulatableParameterStates.h"
template <typename T>
class ModulatableParameterState;
template <typename T>
class ModulatableParameterState_Outdated;
template <typename T>
class ModulatableParameterState_UpToDate;

#include "RegionLfo.h"
class RegionLfo;

/// <summary>
/// Wrapper class for modulatable parameters. Contains the parameter's base value as well as the current modulated value.
/// It also contains a modulator list to / from which LFOs can add / remove themselves.
/// This ultimately saves some CPU ressources as the modulated value only gets updated when any of the LFOs update(instead of being updated during every sample).
/// </summary>
/// <typeparam name="T">Type of the modulated parameter</typeparam>
template <typename T>
class ModulatableParameter
{
public:
    ModulatableParameter(T baseValue);
    ~ModulatableParameter();

    typedef double(*lfoEvalFuncPt)(RegionLfo* lfo);

    void addModulator(RegionLfo* newModulatingLfo, int newRegionID, lfoEvalFuncPt newEvalFuncPt);
    //void removeModulator(RegionLfo* modulatingLfo);
    void removeModulator(int regionID);

    T getBaseValue();
    void setBaseValue(T newBaseValue);

    T getModulatedValue();
    void signalModulatorUpdated(); //transitions into a pending state, so that next time getModulatedValue() is called, currentModulatedValue is re-calculated

    void transitionToState(ModulatableParameterStateIndex stateToTransitionTo);

    T currentModulatedValue; //required to be public for the implementation of the states - exclusively use getModulatedValue otherwise!
    virtual void calculateModulatedValue() = 0;

protected:
    //states
    ModulatableParameterState<T>* states[static_cast<int>(ModulatableParameterStateIndex::StateIndexCount)]; //fixed size -> more efficient access

    static const ModulatableParameterStateIndex initialStateIndex = ModulatableParameterStateIndex::outdated;
    ModulatableParameterStateIndex currentStateIndex;
    ModulatableParameterState<T>* currentState = nullptr; //ensures one less if case when accessing the modulated value

    //other
    T baseValue;

    juce::Array<RegionLfo*> modulatingLfos;
    juce::Array<int> modulatingRegionIDs;
    juce::Array<lfoEvalFuncPt> lfoEvaluationFunctions; //one per LFO; handle the interpretation of each LFO's current value (some might be inverted, sometimes all of them need to be scaled, some parameters might be unipolar, others bipolar, etc.)
};




/// <summary>
/// Implements additive parameters, i.e. parameters where the individual modulators' values add up.
/// Example: pitch shift(final pitch shift = base pitch shift + pitch shift 1 + pitch shift 2 + ...)
/// </summary>
/// <typeparam name="T">Type of the modulated parameter</typeparam>
template <typename T>
class ModulatableAdditiveParameter final : public ModulatableParameter<T>
{
public:
    ModulatableAdditiveParameter(T baseValue);
    ~ModulatableAdditiveParameter();

    void calculateModulatedValue() override;
};
//class ModulatableAdditiveParameter final : public ModulatableParameter<double>
//{
//public:
//    ModulatableAdditiveParameter(double baseValue);
//    ~ModulatableAdditiveParameter();
//
//    void calculateModulatedValue() override;
//};




/// <summary>
/// Implements multiplicative parameters, i.e. parameters where the individual modulators' values are multiplied.
/// Example: volume(final volume = base volume * volume 1 * volume 2 * ...)
/// </summary>
/// <typeparam name="T">Type of the modulated parameter</typeparam>
template <typename T>
class ModulatableMultiplicativeParameter final : public ModulatableParameter<T>
{
public:
    ModulatableMultiplicativeParameter(T baseValue);
    ~ModulatableMultiplicativeParameter();

    void calculateModulatedValue() override;
};
//class ModulatableMultiplicativeParameter final : public ModulatableParameter<double>
//{
//public:
//    ModulatableMultiplicativeParameter(double baseValue);
//    ~ModulatableMultiplicativeParameter();
//
//    void calculateModulatedValue() override;
//};
