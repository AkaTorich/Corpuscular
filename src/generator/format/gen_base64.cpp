/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_base64.h"

namespace corpuscular {
namespace gen {

  std::string encodeBase64(const uint8_t* data, size_t size) {
    static const char kBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    result.reserve(((size + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 2 < size; i += 3) {
      uint32_t chunk = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8) | uint32_t(data[i + 2]);
      result.push_back(kBase64Alphabet[(chunk >> 18) & 0x3f]);
      result.push_back(kBase64Alphabet[(chunk >> 12) & 0x3f]);
      result.push_back(kBase64Alphabet[(chunk >> 6) & 0x3f]);
      result.push_back(kBase64Alphabet[chunk & 0x3f]);
    }

    size_t remaining = size - i;
    if (remaining == 1) {
      uint32_t chunk = uint32_t(data[i]) << 16;
      result.push_back(kBase64Alphabet[(chunk >> 18) & 0x3f]);
      result.push_back(kBase64Alphabet[(chunk >> 12) & 0x3f]);
      result.push_back('=');
      result.push_back('=');
    }
    else if (remaining == 2) {
      uint32_t chunk = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8);
      result.push_back(kBase64Alphabet[(chunk >> 18) & 0x3f]);
      result.push_back(kBase64Alphabet[(chunk >> 12) & 0x3f]);
      result.push_back(kBase64Alphabet[(chunk >> 6) & 0x3f]);
      result.push_back('=');
    }
    return result;
  }

} // namespace gen
} // namespace corpuscular
