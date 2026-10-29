/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

namespace corpuscular {
namespace gen {

  class Random;

  // Arp melodic patterns extracted from MIDI files; data is in gen_midi_arp_patterns_data.cpp
  class MidiArpPatterns {
    public:
      static constexpr int kSteps = 16;

      static int count() { return kNumPatterns; }
      static const int* get(int index) { return kPatterns[index]; }
      static const int* getRandom(Random& random);

    private:
      static const int kPatterns[][kSteps];
      static const int kNumPatterns;
  };

} // namespace gen
} // namespace corpuscular
