/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  class Random;

  // Preset scale: semitones from the tonic (0 ... 12, as in ScaleManager)
  class Scale {
    public:
      Scale(std::string scale_name, std::vector<int> scale_semitones) :
          name(std::move(scale_name)), semitones(std::move(scale_semitones)) { }

      // osc_X_transpose_quantize mask: bit n allows semitone n
      int quantizeMask() const;

      int quantize(int semitone) const;

      // Scales psytrance lives in; used when no scale is chosen
      static const Scale& randomPsy(Random& random);

      // Scale from the library, or nothing when the category or name is empty or unknown
      static std::optional<Scale> fromLibrary(const std::string& category, const std::string& name);

      std::string name;
      std::vector<int> semitones;
  };

} // namespace gen
} // namespace corpuscular
