/*
  ==============================================================================

    ModulatableParameter.cpp
    Created: 21 Sep 2022 9:25:17am
    Author:  Aaron

  ==============================================================================
*/

#pragma once

#include "ModulatableParameter.h"

//template class ModulatableAdditiveParameter<double>; //forces the double implementation of ModulatableAdditiveParameter to be compiled before compiling this class (see https://stackoverflow.com/questions/2152002/how-do-i-force-a-particular-instance-of-a-c-template-to-instantiate) -> needs to be done to compile the member of that class
//template class ModulatableMultiplicativeParameter<double>; //forces the double implementation of ModulatableAdditiveParameter to be compiled before compiling this class (see https://stackoverflow.com/questions/2152002/how-do-i-force-a-particular-instance-of-a-c-template-to-instantiate) -> needs to be done to compile the member of that class
//this still produces the RegionLfo compilation error OOF 

//#include "RegionLfo.h"
//class RegionLfo;


//#pragma region ModulatableParameter
//
//template <typename T>
//ModulatableParameter<T>::ModulatableParameter(T baseValue) :
//    baseValue(baseValue)
//{
//    currentModulatedValue = baseValue;
//
//    //prepare states
//    states[static_cast<int>(ModulatableParameterStateIndex::outdated)] = static_cast<ModulatableParameterState*>(new ModulatableParameterState_Outdated(*this));
//    states[static_cast<int>(ModulatableParameterStateIndex::upToDate)] = static_cast<ModulatableParameterState*>(new ModulatableParameterState_UpToDate(*this));
//
//    currentStateIndex = initialStateIndex;
//    currentState = states[static_cast<int>(currentStateIndex)];
//}
//
//template <typename T>
//ModulatableParameter<T>::~ModulatableParameter()
//{
//    modulatingLfos.clear();
//    modulatingRegionIDs.clear();
//    lfoEvaluationFunctions.clear();
//
//    //release states
//    delete states[static_cast<int>(ModulatableParameterStateIndex::outdated)];
//    delete states[static_cast<int>(ModulatableParameterStateIndex::upToDate)];
//}
//
//
//template <typename T>
//void ModulatableParameter<T>::addModulator(RegionLfo* newModulatingLfo, int newRegionID, lfoEvalFuncPt newEvalFuncPt)
//{
//    for (auto* it = modulatingRegionIDs.begin(); it != modulatingRegionIDs.end(); it++)
//    {
//        if (*it == newRegionID)
//        {
//            //new LFO already exists in the list -> done
//            return;
//        }
//    }
//
//    //new LFO does not yet exist in the list
//    modulatingLfos.add(newModulatingLfo);
//    modulatingRegionIDs.add(newRegionID);
//    lfoEvaluationFunctions.add(newEvalFuncPt);
//    transitionToState(ModulatableParameterStateIndex::outdated);
//}
//
//template <typename T>
////void ModulatableParameter<T>::removeModulator(RegionLfo* modulatingLfo)
////{
////    for (auto* it = modulatingLfos.begin(), int i = 0; it != modulatingLfos.end(); it++, i++)
////    {
////        if ((*it)->getRegionID() == newModulatingLfo->getRegionID())
////        {
////            //LFO found
////            modulatingLfos.remove(i);
////            lfoEvaluationFunctions.remove(i);
////            transitionToState(ModulatableParameterStateIndex::outdated);
////            return;
////        }
////    }
////
////    //LFO does not exist in the list -> done
////}
//void ModulatableParameter<T>::removeModulator(int regionID)
//{
//    for (auto* itRegionID = modulatingRegionIDs.begin(), int i = 0; itRegionID != modulatingRegionIDs.end(); itRegionID++, i++)
//    {
//        if (*itRegionID == regionID)
//        {
//            //LFO found
//            modulatingLfos.remove(i);
//            modulatingRegionIDs.remove(i);
//            lfoEvaluationFunctions.remove(i);
//            transitionToState(ModulatableParameterStateIndex::outdated);
//            return;
//        }
//    }
//
//    //LFO does not exist in the list -> done
//}
//
//
//template <typename T>
//T ModulatableParameter<T>::getBaseValue()
//{
//    return baseValue;
//}
//
//template <typename T>
//void ModulatableParameter<T>::setBaseValue(T newBaseValue)
//{
//    baseValue = newBaseValue;
//    transitionToState(ModulatableParameterStateIndex::outdated);
//}
//
//
//template <typename T>
//T ModulatableParameter<T>::getModulatedValue()
//{
//    currentState->ensureModulatorIsUpToDate(); //re-calculates the modulated value if it's outdated
//    return currentModulatedValue;
//}
//
//template <typename T>
//void ModulatableParameter<T>::signalModulatorUpdated() //transitions into a pending state, so that next time getModulatedValue() is called, currentModulatedValue is re-calculated
//{
//    //transitionToState(ModulatableParameterStateIndex::outdated);
//    currentState->modulatorHasUpdated(); //should be sliiiightly quicker than the line above, because for the outdated state, this function is empty. and since this function will be called several times during the outdated state except if there's only 1 modulator, it'll be more efficient. and if there's only 1 modulator, not a lot of CPU is used anyway.
//}
//
//
//template <typename T>
//void ModulatableParameter<T>::transitionToState(ModulatableParameterStateIndex stateToTransitionTo)
//{
//    //states cannot be skipped here, so in contrast to the state implementation of RegionLfo or the DAHDSR envelope and such, no while-loop is needed here
//    switch (stateToTransitionTo)
//    {
//    case ModulatableParameterStateIndex::outdated:
//        //DBG("parameter outdated");
//        break;
//
//    case ModulatableParameterStateIndex::upToDate:
//        //DBG("parameter up-to-date")
//        break;
//
//    default:
//        throw std::exception("unhandled state index");
//    }
//
//    //finally, update the current state index
//    currentStateIndex = stateToTransitionTo;
//    currentState = states[static_cast<int>(currentStateIndex)];
//}
//
//#pragma endregion ModulatableParameter
//
//
//
//
//#pragma region ModulatableAdditiveParameter
//
//template <typename T>
//ModulatableAdditiveParameter<T>::ModulatableAdditiveParameter(T baseValue) :
//    ModulatableParameter<T>(baseValue)
//{ }
//
//template <typename T>
//ModulatableAdditiveParameter<T>::~ModulatableAdditiveParameter()
//{
//    ModulatableParameter<T>::~ModulatableParameter<T>();
//}
//
//template <typename T>
//void ModulatableAdditiveParameter<T>::calculateModulatedValue()
//{
//    currentModulatedValue = baseValue;
//
//    auto* itFunc = lfoEvaluationFunctions.begin();
//    for (auto* itLfo = modulatingLfos.begin(); itLfo != modulatingLfos.end(); itLfo++, itFunc++)
//    {
//        currentModulatedValue += (*itFunc)(*itLfo); //handles unipolar vs. bipolar values, scalings, inversions, etc.
//    }
//
//    //parameter is now up-to-date again
//    transitionToState(ModulatableParameterStateIndex::upToDate);
//}
//
////ModulatableAdditiveParameter::ModulatableAdditiveParameter(double baseValue) :
////    ModulatableParameter<double>(baseValue)
////{ }
////
////ModulatableAdditiveParameter::~ModulatableAdditiveParameter()
////{
////    ModulatableParameter<double>::~ModulatableParameter<double>();
////}
////
////void ModulatableAdditiveParameter::calculateModulatedValue()
////{
////    currentModulatedValue = baseValue;
////
////    auto itFunc = lfoEvaluationFunctions.begin();
////    for (auto* itLfo = modulatingLfos.begin(); itLfo != modulatingLfos.end(); itLfo++, itFunc++)
////    {
////        currentModulatedValue += (*itFunc)(*itLfo); //handles unipolar vs. bipolar values, scalings, inversions, etc.
////    }
////
////    //parameter is now up-to-date again
////    transitionToState(ModulatableParameterStateIndex::upToDate);
////}
//
//#pragma endregion ModulatableAdditiveParameter
//
//
//
//
//#pragma region ModulatableMultiplicativeParameter
//
//template <typename T>
//ModulatableMultiplicativeParameter<T>::ModulatableMultiplicativeParameter(T baseValue) :
//    ModulatableParameter<T>(baseValue)
//{ }
//
//template <typename T>
//ModulatableMultiplicativeParameter<T>::~ModulatableMultiplicativeParameter()
//{
//    ModulatableParameter<T>::~ModulatableParameter<T>();
//}
//
//template <typename T>
//void ModulatableMultiplicativeParameter<T>::calculateModulatedValue()
//{
//    currentModulatedValue = baseValue;
//
//    auto* itFunc = lfoEvaluationFunctions.begin();
//    for (auto* itLfo = modulatingLfos.begin(); itLfo != modulatingLfos.end(); itLfo++, itFunc++)
//    {
//        currentModulatedValue += (*itFunc)(*itLfo); //handles unipolar vs. bipolar values, scalings, inversions, etc.
//    }
//
//    //parameter is now up-to-date again
//    transitionToState(ModulatableParameterStateIndex::upToDate);
//}
//
////ModulatableMultiplicativeParameter::ModulatableMultiplicativeParameter(double baseValue) :
////    ModulatableParameter<double>(baseValue)
////{ }
////
////ModulatableMultiplicativeParameter::~ModulatableMultiplicativeParameter()
////{
////    ModulatableParameter<double>::~ModulatableParameter<double>();
////}
////
////void ModulatableMultiplicativeParameter::calculateModulatedValue()
////{
////    currentModulatedValue = baseValue;
////
////    auto* itFunc = lfoEvaluationFunctions.begin();
////    for (auto* itLfo = modulatingLfos.begin(); itLfo != modulatingLfos.end(); itLfo++, itFunc++)
////    {
////        currentModulatedValue += (*itFunc)(*itLfo); //handles unipolar vs. bipolar values, scalings, inversions, etc.
////    }
////
////    //parameter is now up-to-date again
////    transitionToState(ModulatableParameterStateIndex::upToDate);
////}
//
//#pragma endregion ModulatableMultiplicativeParameter
