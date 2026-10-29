/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_scales.h"

#include "gen_scale_manager.h"
#include "../gen_random.h"

#include <algorithm>
#include <climits>
#include <cstdlib>

namespace corpuscular {
namespace gen {

  int Scale::quantizeMask() const {
    int mask = 0;
    for (int semitone : semitones)
      mask |= 1 << (((semitone % 12) + 12) % 12);
    return mask | 1;
  }

  int Scale::quantize(int semitone) const {
    int octave = semitone >= 0 ? semitone / 12 : (semitone - 11) / 12;
    int note = semitone - octave * 12;
    int best = note;
    int best_distance = INT_MAX;
    for (int s : semitones) {
      int scale_note = ((s % 12) + 12) % 12;
      int distance = std::abs(note - scale_note);
      if (distance > 6)
        distance = 12 - distance;
      if (distance < best_distance || (distance == best_distance && scale_note < best)) {
        best_distance = distance;
        best = scale_note;
      }
    }
    return octave * 12 + best;
  }

  static const std::vector<Scale>& psyScales() {
    static const std::vector<Scale> scales = {
      Scale("Natural Minor", { 0, 2, 3, 5, 7, 8, 10, 12 }),
      Scale("Harmonic Minor", { 0, 2, 3, 5, 7, 8, 11, 12 }),
      Scale("Phrygian", { 0, 1, 3, 5, 7, 8, 10, 12 }),
      Scale("Phrygian Dominant", { 0, 1, 4, 5, 7, 8, 10, 12 }),
      Scale("Dorian", { 0, 2, 3, 5, 7, 9, 10, 12 }),
      Scale("Minor Pentatonic", { 0, 3, 5, 7, 10, 12 }),
      Scale("Hungarian Minor", { 0, 2, 3, 6, 7, 8, 11, 12 }),
    };
    return scales;
  }

  const Scale& Scale::randomPsy(Random& random) {
    const std::vector<Scale>& scales = psyScales();
    return scales[random.next(static_cast<int>(scales.size()))];
  }

  std::optional<Scale> Scale::fromLibrary(const std::string& category, const std::string& name) {
    if (category.empty() || name.empty())
      return std::nullopt;
    std::vector<int> semitones = ScaleManager::getScaleAsSemitones(category, name);
    if (semitones.empty())
      return std::nullopt;

    std::vector<int> list;
    for (int x : semitones) {
      if (x >= 0 && x <= 12 && std::find(list.begin(), list.end(), x) == list.end())
        list.push_back(x);
    }
    if (list.size() < 2)
      return std::nullopt;
    return Scale(name, list);
  }

} // namespace gen
} // namespace corpuscular
