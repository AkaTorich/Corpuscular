/* Corpuscular LFO generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "json/json.h"

#include <cstdint>
#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  // LFO shape families, every call gives a different shape of the family
  std::vector<std::string> lfoTypes();

  struct GeneratedLfo {
    std::string name;
    std::string type;
    nlohmann::json data;    // the shape as a .vitallfo file (name, num_points, points, powers, smooth)
  };

  GeneratedLfo createLfo(const std::string& type, uint32_t seed);

} // namespace gen
} // namespace corpuscular
