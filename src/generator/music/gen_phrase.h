/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <array>
#include <string>

namespace corpuscular {
namespace gen {

  // 16 melody steps: semitones from the tonic, velocity 0..1 (0 is a rest), hold 0..1
  class Phrase {
    public:
      static constexpr int kSteps = 16;

      Phrase() {
        notes.fill(0);
        velocity.fill(0.0);
        hold.fill(0.0);
      }

      int minNote() const;
      int maxNote() const;
      int span() const { return maxNote() - minNote(); }

      // Notes into the -12..+24 window keeping the scale tone; range at most two octaves
      void fold();

      std::array<int, kSteps> notes;
      std::array<double, kSteps> velocity;
      std::array<double, kSteps> hold;
      std::string kind;
  };

} // namespace gen
} // namespace corpuscular
