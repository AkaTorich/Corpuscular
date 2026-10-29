/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_wavetable.h"

#include "gen_base64.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace corpuscular {
namespace gen {

  WaveKeyframe::WaveKeyframe(int frame_position, std::vector<float> frame) :
      position(std::max(0, std::min(WavetableData::kMaxPosition, frame_position))), wave(std::move(frame)) {
    if (wave.size() != WavetableData::kFrameSize)
      throw std::invalid_argument("A wave frame is 2048 samples");
  }

  std::string WavetableData::encodeWave(const std::vector<float>& wave) {
    // Little-endian float32, as vital stores it
    std::vector<uint8_t> bytes(wave.size() * 4);
    for (size_t i = 0; i < wave.size(); ++i) {
      uint32_t bits = 0;
      std::memcpy(&bits, &wave[i], sizeof(bits));
      bytes[i * 4] = static_cast<uint8_t>(bits & 0xff);
      bytes[i * 4 + 1] = static_cast<uint8_t>((bits >> 8) & 0xff);
      bytes[i * 4 + 2] = static_cast<uint8_t>((bits >> 16) & 0xff);
      bytes[i * 4 + 3] = static_cast<uint8_t>((bits >> 24) & 0xff);
    }
    return encodeBase64(bytes.data(), bytes.size());
  }

} // namespace gen
} // namespace corpuscular
