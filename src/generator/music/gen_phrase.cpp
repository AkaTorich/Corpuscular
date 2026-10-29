/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_phrase.h"

#include <algorithm>
#include <cmath>

namespace corpuscular {
namespace gen {

  int Phrase::minNote() const {
    return *std::min_element(notes.begin(), notes.end());
  }

  int Phrase::maxNote() const {
    return *std::max_element(notes.begin(), notes.end());
  }

  void Phrase::fold() {
    for (int i = 0; i < kSteps; ++i) {
      while (notes[i] < -12)
        notes[i] += 12;
      while (notes[i] > 24)
        notes[i] -= 12;
    }

    while (span() > 24) {
      double sum = 0.0;
      for (int note : notes)
        sum += note;
      double mid = sum / kSteps;

      int farthest = 0;
      for (int i = 1; i < kSteps; ++i) {
        if (std::abs(notes[i] - mid) > std::abs(notes[farthest] - mid))
          farthest = i;
      }
      notes[farthest] += notes[farthest] > mid ? -12 : 12;
    }
  }

} // namespace gen
} // namespace corpuscular
