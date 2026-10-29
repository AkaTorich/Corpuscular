/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_sample.h"

#include "gen_base64.h"
#include "../gen_random.h"

namespace corpuscular {
namespace gen {

  SampleData SampleData::whiteNoise(int length, int seed) {
    Random random(static_cast<uint32_t>(seed));
    SampleData sample;
    sample.name = "White Noise";
    sample.data.resize(length);
    for (int i = 0; i < length; ++i)
      sample.data[i] = static_cast<int16_t>(random.next(-26000, 26001));
    return sample;
  }

  std::string SampleData::encode() const {
    // Little-endian int16
    std::vector<uint8_t> bytes(data.size() * 2);
    for (size_t i = 0; i < data.size(); ++i) {
      uint16_t value = static_cast<uint16_t>(data[i]);
      bytes[i * 2] = static_cast<uint8_t>(value & 0xff);
      bytes[i * 2 + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
    }
    return encodeBase64(bytes.data(), bytes.size());
  }

} // namespace gen
} // namespace corpuscular
