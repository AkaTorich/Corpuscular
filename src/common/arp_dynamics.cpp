/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "arp_dynamics.h"

#include "Harmony.h"

namespace arp_dynamics {
  namespace {
    constexpr int kStepsPerBar = 16;
    constexpr int kMaxPhrase = 4 * kStepsPerBar;
    // Where the phrase is most tense, as a share of its length
    constexpr double kPeak = 0.68;

    // How strong a place in the bar is
    double beatWeight(int step) {
      int position = step % kStepsPerBar;
      if (position == 0)
        return 1.0;
      if (position == 8)
        return 0.78;
      if (position % 4 == 0)
        return 0.58;
      if (position % 2 == 0)
        return 0.36;
      return 0.18;
    }

    bool startsChord(int step, int length, int chord_count) {
      for (int i = 0; i < chord_count; ++i) {
        if (mg::harmonyChordStart(i, chord_count, length) == step)
          return true;
      }
      return false;
    }

    // Rises to the tension, then relaxes
    double phraseArc(int step, int length) {
      int phrase = std::max(1, std::min(length, kMaxPhrase));
      double position = (step % phrase) / (double)phrase;
      return position < kPeak ? position / kPeak : (1.0 - position) / (1.0 - kPeak);
    }
  }

  void apply(std::vector<ArpSequencer::Note>& notes, int length, float amount, int chord_count) {
    if (notes.empty())
      return;
    length = std::max(1, length);

    int lowest = 127, highest = 0;
    for (const ArpSequencer::Note& note : notes) {
      lowest = std::min(lowest, note.midi);
      highest = std::max(highest, note.midi);
    }
    int range = std::max(1, highest - lowest);

    std::vector<double> scores;
    scores.reserve(notes.size());
    for (const ArpSequencer::Note& note : notes) {
      int step = note.start % length;
      double beat = beatWeight(step);
      if (chord_count > 0 && startsChord(step, length, chord_count))
        beat = 1.1;
      double held = std::min(note.length, 4) / 4.0;
      double height = (note.midi - lowest) / (double)range;
      scores.push_back(0.6 * beat + 0.18 * held + 0.12 * height + 0.15 * phraseArc(step, length));
    }

    double low = *std::min_element(scores.begin(), scores.end());
    double high = *std::max_element(scores.begin(), scores.end());
    float spread = jlimit(0.0f, 1.0f, amount) * kMaxSpread;
    for (size_t i = 0; i < notes.size(); ++i) {
      double share = high > low ? (scores[i] - low) / (high - low) : 0.5;
      notes[i].velocity = jlimit(1, 127, roundToInt(kBaseVelocity + (share * 2.0 - 1.0) * spread));
    }
  }
}
