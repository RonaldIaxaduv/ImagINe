/*
  ==============================================================================

    ModulatableParameterStates.cpp
    Created: 21 Sep 2022 9:26:12am
    Author:  Aaron

  ==============================================================================
*/

#include "ModulatableParameterStates.h"

template <typename T>
ModulatableParameterState<T>::ModulatableParameterState(ModulatableParameter<T>& parameter) :
    parameter(parameter)
{ }




template <typename T>
ModulatableParameterState_Outdated<T>::ModulatableParameterState_Outdated(ModulatableParameter<T>& parameter) :
    ModulatableParameterState(parameter)
{
    implementedStateIndex = ModulatableParameterStateIndex::outdated;
}

template <typename T>
void ModulatableParameterState_Outdated<T>::modulatorHasUpdated()
{
    //nothing happens; already marked as outdated
}

template <typename T>
void ModulatableParameterState_Outdated<T>::ensureModulatorIsUpToDate()
{
    //previous value is outdated -> update
    parameter.calculateModulatedValue();

    //automatically switches to upToDate afterwards
}




template <typename T>
ModulatableParameterState_UpToDate<T>::ModulatableParameterState_UpToDate(ModulatableParameter<T>& parameter) :
    ModulatableParameterState(parameter)
{
    implementedStateIndex = ModulatableParameterStateIndex::upToDate;
}

template <typename T>
void ModulatableParameterState_UpToDate<T>::modulatorHasUpdated()
{
    //-> value of modulatedParameter is no longer up-to-date
    parameter.transitionToState(ModulatableParameterStateIndex::outdated);
}

template <typename T>
void ModulatableParameterState_UpToDate<T>::ensureModulatorIsUpToDate()
{
    //previous value is still up-to-date -> do nothing
}
