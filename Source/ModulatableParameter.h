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

//#include "RegionLfo.h" //do NOT include this! since RegionLfo has a ModulatableAdditiveParameter member and including RegionLfo's header would require ModulatableAdditiveParameter<double> to already have been initialised, that creates a cyclic compilation error!
class RegionLfo; //this is enough for ModulatableParameter to be compiled since it only needs pointers to RegionLfo (-> known size in memory) and no methods

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
    ModulatableParameter(T baseValue) :
        baseValue(baseValue)
    {
        currentModulatedValue = baseValue;

        //prepare states
        states[static_cast<int>(ModulatableParameterStateIndex::outdated)] = static_cast<ModulatableParameterState<T>*>(new ModulatableParameterState_Outdated<T>(*this));
        states[static_cast<int>(ModulatableParameterStateIndex::upToDate)] = static_cast<ModulatableParameterState<T>*>(new ModulatableParameterState_UpToDate<T>(*this));
        int initialisedStates = 2;
        jassert(initialisedStates == static_cast<int>(ModulatableParameterStateIndex::StateIndexCount));

        currentStateIndex = initialStateIndex;
        currentState = states[static_cast<int>(currentStateIndex)];
    }
    ~ModulatableParameter()
    {
        DBG("destroying ModulatableParameter...");

        //release states
        currentState = nullptr;
        delete states[static_cast<int>(ModulatableParameterStateIndex::outdated)];
        delete states[static_cast<int>(ModulatableParameterStateIndex::upToDate)];
        int deletedStates = 2;
        jassert(deletedStates == static_cast<int>(ModulatableParameterStateIndex::StateIndexCount));

        //rest
        modulatingLfos.clear();
        modulatingRegionIDs.clear();
        lfoEvaluationFunctions.clear();

        DBG("ModulatableParameter destroyed.");
    }

    typedef double(*lfoEvalFuncPt)(RegionLfo* lfo); //pointer to a function that evaluates the current value of an LFO

    void addModulator(RegionLfo* newModulatingLfo, int newRegionID, lfoEvalFuncPt newEvalFuncPt)
    {
        for (auto* it = modulatingRegionIDs.begin(); it != modulatingRegionIDs.end(); it++)
        {
            if (*it == newRegionID)
            {
                //new LFO already exists in the list -> done
                return;
            }
        }

        //new LFO does not yet exist in the list
        modulatingLfos.add(newModulatingLfo);
        modulatingRegionIDs.add(newRegionID);
        lfoEvaluationFunctions.add(newEvalFuncPt);
        transitionToState(ModulatableParameterStateIndex::outdated);
    }
    //void removeModulator(RegionLfo* modulatingLfo);
    void removeModulator(int regionID)
    {
        int i = 0;
        for (auto* itRegionID = modulatingRegionIDs.begin(); itRegionID != modulatingRegionIDs.end(); itRegionID++, i++)
        {
            if (*itRegionID == regionID)
            {
                //LFO found
                modulatingLfos.remove(i);
                modulatingRegionIDs.remove(i);
                lfoEvaluationFunctions.remove(i);
                transitionToState(ModulatableParameterStateIndex::outdated);
                return;
            }
        }

        //LFO does not exist in the list -> done
    }
    juce::Array<RegionLfo*> getModulators()
    {
        juce::Array<RegionLfo*> output;
        output.addArray(modulatingLfos);
        return output;
    }

    T getBaseValue()
    {
        return baseValue;
    }
    void setBaseValue(T newBaseValue)
    {
        baseValue = newBaseValue;
        transitionToState(ModulatableParameterStateIndex::outdated);
    }

    T getModulatedValue()
    {
        currentState->ensureModulatorIsUpToDate(); //re-calculates the modulated value if it's outdated
        return currentModulatedValue;
    }
    void signalModulatorUpdated() //transitions into a pending state, so that next time getModulatedValue() is called, currentModulatedValue is re-calculated
    {
        //transitionToState(ModulatableParameterStateIndex::outdated);
        currentState->modulatorHasUpdated(); //should be sliiiightly quicker than the line above, because for the outdated state, this function is empty. and since this function will be called several times during the outdated state except if there's only 1 modulator, it'll be more efficient. and if there's only 1 modulator, not a lot of CPU is used anyway.
    }

    void transitionToState(ModulatableParameterStateIndex stateToTransitionTo)
    {
        //states cannot be skipped here, so in contrast to the state implementation of RegionLfo or the DAHDSR envelope and such, no while-loop is needed here
        switch (stateToTransitionTo)
        {
        case ModulatableParameterStateIndex::outdated:
            //DBG("parameter outdated");
            break;

        case ModulatableParameterStateIndex::upToDate:
            //DBG("parameter up-to-date")
            break;

        default:
            throw std::exception("unhandled state index");
        }

        //finally, update the current state index
        currentStateIndex = stateToTransitionTo;
        currentState = states[static_cast<int>(currentStateIndex)];
    }

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
    ModulatableAdditiveParameter(T baseValue) :
        ModulatableParameter<T>::ModulatableParameter(baseValue)
    { }
    /*~ModulatableAdditiveParameter() //<- IMPORTANT: this caused some problems with the deletion of some scalars for some reason. luckily it's not needed atm, but if it ever is, look into that issue further.
    {
        ModulatableParameter<T>::~ModulatableParameter<T>();
    }*/

    void calculateModulatedValue() override
    {
        currentModulatedValue = baseValue;

        auto* itFunc = lfoEvaluationFunctions.begin();
        for (auto* itLfo = modulatingLfos.begin(); itLfo != modulatingLfos.end(); itLfo++, itFunc++)
        {
            currentModulatedValue += (*itFunc)(*itLfo); //handles unipolar vs. bipolar values, scalings, inversions, etc.
        }

        //parameter is now up-to-date again
        transitionToState(ModulatableParameterStateIndex::upToDate);
    }
};



/// <summary>
/// Implements multiplicative parameters, i.e. parameters where the individual modulators' values are multiplied.
/// Example: volume(final volume = base volume * volume 1 * volume 2 * ...)
/// </summary>
/// <typeparam name="T">Type of the modulated parameter</typeparam>
template <typename T>
class ModulatableMultiplicativeParameter final : public ModulatableParameter<T>
{
public:
    ModulatableMultiplicativeParameter(T baseValue) :
        ModulatableParameter<T>::ModulatableParameter(baseValue)
    { }
    /*~ModulatableMultiplicativeParameter() //<- IMPORTANT: this caused some problems with the deletion of some scalars for some reason. luckily it's not needed atm, but if it ever is, look into that issue further.
    {
        ModulatableParameter<T>::~ModulatableParameter<T>();
    }*/

    void calculateModulatedValue() override
    {
        currentModulatedValue = baseValue;

        //DBG("base " + juce::String(baseValue));

        auto* itFunc = lfoEvaluationFunctions.begin();
        for (auto* itLfo = modulatingLfos.begin(); itLfo != modulatingLfos.end(); itLfo++, itFunc++)
        {
            currentModulatedValue *= (*itFunc)(*itLfo); //handles unipolar vs. bipolar values, scalings, inversions, etc.
            //DBG("*" + juce::String((*itFunc)(*itLfo)));
        }

        //DBG("mod val: " + juce::String(currentModulatedValue));

        //parameter is now up-to-date again
        transitionToState(ModulatableParameterStateIndex::upToDate);
    }
};
