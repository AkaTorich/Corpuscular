/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace corpuscular {
namespace gen {

  // Standard base64 with padding, as Convert.ToBase64String.
  std::string encodeBase64(const uint8_t* data, size_t size);

} // namespace gen
} // namespace corpuscular
