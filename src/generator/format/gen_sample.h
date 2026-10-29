/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  // vital reads the sample without checks, so every preset carries one
  class SampleData {
    public:
      static SampleData whiteNoise(int length, int seed);

      std::string encode() const;

      std::string name = "White Noise";
      int sample_rate = 44100;
      std::vector<int16_t> data;
  };

} // namespace gen
} // namespace corpuscular
