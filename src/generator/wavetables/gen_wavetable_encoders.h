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

  // Wavetable file images in memory
  namespace WavetableEncoders {
    // Mono WAV, all frames concatenated; the clm chunk tells Serum/Vital the frame size
    std::vector<uint8_t> wav(const std::vector<std::vector<float>>& frames, int frame_size, bool float32, bool clm_chunk,
                             const std::string& author, const std::string& name, const std::string& type_name);

    // Surge / Bitwig ".wt": "vawt", uint32 frame size, uint16 frame count, uint16 flags, int16 samples
    std::vector<uint8_t> wt(const std::vector<std::vector<float>>& frames, int frame_size);

    // Headerless little-endian float32
    std::vector<uint8_t> raw(const std::vector<std::vector<float>>& frames, int frame_size);

    // Single-cycle frame to another length, periodic linear interpolation
    std::vector<float> resampleFrame(const std::vector<float>& frame, int size);
  }

} // namespace gen
} // namespace corpuscular
