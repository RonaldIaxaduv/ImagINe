/*
  ==============================================================================

    PitchQuantisationMethod.h
    Created: 6 Oct 2022 1:22:50pm
    Author:  Aaron

  ==============================================================================
*/

#pragma once

enum class PitchQuantisationMethod : int
{
    continuous = 0,
    semitones,
    scale_major,
    scale_minor,
    scale_octaves,

    //reversed scales: going from higher notes to lower ones instead of the other way around
    scale_semitonesReversed,
    scale_majorReversed,
    scale_minorReversed,

    //oriental
    scale_hijazKar, //https://en.wikipedia.org/wiki/Double_harmonic_scale double harmonic major scale (often associated with arabic music)
    scale_hungarianMinor, //https://en.wikipedia.org/wiki/Hungarian_minor_scale double harmonic minor scale
    scale_persian, //https://en.wikipedia.org/wiki/Persian_scale

    //blues
    //https://en.wikipedia.org/wiki/Blues_scale
    scale_bluesHexa,
    scale_bluesHepta,
    scale_bluesNona,
        
    //japanese
    scale_ritsu, //https://en.wikipedia.org/wiki/Ritsu_and_ryo_scales
    scale_ryo, //https://en.wikipedia.org/wiki/Ritsu_and_ryo_scales
    scale_minyo, //https://en.wikipedia.org/wiki/Yo_scale
    scale_miyakoBushi, //https://en.wikipedia.org/wiki/In_scale

    //other
    scale_wholeTone, //https://en.wikipedia.org/wiki/Whole-tone_scale
    scale_pentatonicMajor, //https://en.wikipedia.org/wiki/Pentatonic_scale
    scale_pentatonicMinor, //https://en.wikipedia.org/wiki/Pentatonic_scale
    scale_halfUp, //0 on the first half, 1 on the second
    scale_halfDown, //-1 on the first half, 0 on the second

    MethodCount
};