/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include "MusicData.h"

// HARMONIC on the ARP tab: a melody laid over a chord progression.
// The chords split the melody into equal parts; the notes on the beats and the long notes become chord notes,
// the quick notes between them stay where they are unless they rub against the chord.
namespace mg
{
    // `count` chords of the scale, tension and release: from the tonic away (VI, IV, II, III), the last one
    // a dominant (V or VII) that the loop resolves into the tonic again. From 6 chords — two phrases,
    // a question ending on a dominant and an answer. Scales without seven notes: different chords, tonic first.
    juce::StringArray harmonyProgression(const juce::String& tonality, const Scale& scale, int count, juce::Random& rng);

    // Where chord `index` of `count` starts, in sixteenths, when they share `total` sixteenths
    int harmonyChordStart(int index, int count, int total);

    // The melody bent onto the chords. A bass puts the root of the chord on the first note of each chord.
    Clip fitToChords(const Clip& melody, const juce::StringArray& chords, int total, bool bass);
}
