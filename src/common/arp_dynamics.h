/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#pragma once

#include "arp_sequencer.h"

// VELOCITY of the ARP tab: how hard each note of the pattern is played. Not random — the strength follows the music:
// - the beat: the first sixteenth of a bar is the strongest, then the half bar, the beats, the eighths; the quick notes
//   between them are the softest;
// - with HARMONIC, the first note of every chord is played like a downbeat;
// - long notes are stronger than quick ones, and higher notes stronger than lower ones (the peak of a phrase stands out);
// - the phrase (the pattern, four bars at most) grows toward its tension at about two thirds and relaxes at the end,
//   like the chords' tension and resolution.
// `amount` 0..1 is how far the notes spread around the middle strength (0 = all equal).
namespace arp_dynamics {
  constexpr int kBaseVelocity = 96;
  constexpr int kMaxSpread = 30;

  // `chord_count` — how many chords the melody went through (0 = no chords)
  void apply(std::vector<ArpSequencer::Note>& notes, int length, float amount, int chord_count);
}
