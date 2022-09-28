/*
  ==============================================================================

    ModulatableParameterStates.h
    Created: 21 Sep 2022 9:26:12am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include "ModulatableParameter.h"
#include "ModulatableParameterStateIndex.h"
template <typename T>
class ModulatableParameter;


/// <summary>
/// Abstract class which describes states of the ModulatableParameter class
/// </summary>
/// <typeparam name="T">Type of the modulated parameter</typeparam>
template <typename T>
class ModulatableParameterState
{
public:
    ModulatableParameterState(ModulatableParameter<T>& parameter) :
        parameter(parameter)
    { }

    virtual void modulatorHasUpdated() = 0;
    virtual void ensureModulatorIsUpToDate() = 0;

protected:
    ModulatableParameter<T>& parameter;
    ModulatableParameterStateIndex implementedStateIndex = ModulatableParameterStateIndex::StateIndexCount;
};




/// <summary>
/// Implements the state of a modulatable parameter when the value of its modulated parameter is outdated and needs to be updated the next time it's accessed.
/// </summary>
/// <typeparam name="T">Type of the modulated parameter</typeparam>
template <typename T>
class ModulatableParameterState_Outdated final : public ModulatableParameterState<T>
{
public:
    ModulatableParameterState_Outdated(ModulatableParameter<T>& parameter) :
        ModulatableParameterState(parameter)
    {
        implementedStateIndex = ModulatableParameterStateIndex::outdated;
    }

    void modulatorHasUpdated() override
    {
        //nothing happens; already marked as outdated
    }
    void ensureModulatorIsUpToDate() override //called every time before the parameter accesses the modulated value
    {
        //previous value is outdated -> update
        parameter.calculateModulatedValue();

        //automatically switches to upToDate afterwards
    }
};




/// <summary>
/// Implements the state of a modulatable parameter when the value of its modulated parameter is up-to-date and needn't be updated during its access, until any modulator updates.
/// </summary>
/// <typeparam name="T"></typeparam>
template <typename T>
class ModulatableParameterState_UpToDate final : public ModulatableParameterState<T>
{
public:
    ModulatableParameterState_UpToDate(ModulatableParameter<T>& parameter) :
        ModulatableParameterState(parameter)
    {
        implementedStateIndex = ModulatableParameterStateIndex::upToDate;
    }

    void modulatorHasUpdated() override
    {
        //-> value of modulatedParameter is no longer up-to-date
        parameter.transitionToState(ModulatableParameterStateIndex::outdated);
    }
    void ensureModulatorIsUpToDate() override
    {
        //previous value is still up-to-date -> do nothing
    }
};
