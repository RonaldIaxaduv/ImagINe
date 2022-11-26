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
    full = 0, //this would make it so that the update rate isn't affected by any modulation, so it won't be listed in the combobox. just didn't feel right to leave it out here, tho
    full_dotted, //this would make the update rate slower, so it won't be listed in the combobox. just didn't feel right to leave it out here, tho
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
    sixtyfourth_triole,
    continuous //no quantisation
};