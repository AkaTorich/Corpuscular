/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  class Patch;
  class Phrase;
  class Scale;

  // Phrase -> stepped LFOs inside the preset: hold one note and the melody plays
  namespace SequenceEncoder {
    // Pitch: LFO -> osc_X_transpose; amount = span / 96 (transpose range is -48..48)
    int applyPitch(Patch& patch, const Phrase& phrase, const Scale& scale, const std::vector<int>& oscs,
                   const std::string& tempo);

    // Rhythm and accents: an LFO opens the oscillator levels per step (rest is silence, legato fills the step)
    int applyGate(Patch& patch, const Phrase& phrase, const std::vector<int>& oscs, const std::string& tempo,
                  double staccato, double decay);

    // Even rolling gate of a psytrance bass: a gap on the kick beat, then three sixteenths
    int applyRollingGate(Patch& patch, const std::vector<int>& oscs, const std::vector<int>& pattern,
                         double staccato, double decay);
  }

} // namespace gen
} // namespace corpuscular
