/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  struct WaveKeyframe {
    WaveKeyframe(int frame_position, std::vector<float> frame);

    int position;              // 0..256
    std::vector<float> wave;   // 2048 samples
  };

  // Oscillator wavetable: one group with a single "Wave Source" component
  class WavetableData {
    public:
      static constexpr int kFrameSize = 2048;
      static constexpr int kMaxPosition = 256;

      void add(int position, std::vector<float> wave) { keyframes.emplace_back(position, std::move(wave)); }

      static std::string encodeWave(const std::vector<float>& wave);

      std::string name = "Init";
      std::string author;
      bool full_normalize = true;
      bool remove_all_dc = true;
      int interpolation_style = 1;  // 0 none, 1 linear, 2 cubic
      int interpolation = 0;        // 0 time domain, 1 spectral
      std::vector<WaveKeyframe> keyframes;
  };

} // namespace gen
} // namespace corpuscular
