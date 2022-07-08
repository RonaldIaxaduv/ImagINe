/*
  ==============================================================================

    DahdsrEnvelopeStateIndex.h
    Created: 7 Jul 2022 9:50:32pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

//not an enum class since using it in for-loops and as array indices is the main use case
//while it would be more convenient not to declare this an enum class/struct (since implicit conversion only works for plain enum),
//this allows enum names to be used in other places, which is cleaner (a normal enum essentially only declares static const variables, which
//is why there cannot be two normal enums that have one or more matching names...)
//the obvious disadvantage is that explicit conversions (static_cast<int>) need to be used which make the code less readable.
//the good news, however, is that static_cast is applied at compile time, i.e. at least it shouldn't make the code any slower.
enum struct DahdsrEnvelopeStateIndex : int
{
    unprepared = 0,
    idle,
    delay,
    attack,
    hold,
    decay,
    sustain,
    release,
    StateIndexCount
};
