/*
  ==============================================================================

    UpdateRateQuantisationMethod.h
    Created: 22 Nov 2022 9:30:47pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum class UpdateRateQuantisationMethod : int
{
    continuous = 0,
    full, //this doesn't do anything, so it won't be listed in the combobox. juce didn't feel right to leave it out here, tho
    full_dotted, //same as above
    full_triole,
    half,
    half_dotted,
    half_triole,
    quarter,
    quarter_dotted,
    quarter_triole,
    eighth,
    eighth_dotted,
    eighth_triole,
    sixteenth,
    sixteenth_dotted,
    sixteenth_triole,
    thirtysecond,
    thirtysecond_dotted,
    thirtysecond_triole,
    sixtyfourth,
    sixtyfourth_dotted,
    sixtyfourth_triole
};