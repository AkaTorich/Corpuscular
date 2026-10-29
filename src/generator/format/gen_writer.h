/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "json/json.h"

namespace corpuscular {
namespace gen {

  class Patch;

  // Preset -> JSON of the same shape vital 1.0.7 saves (a full .vital file)
  namespace PresetWriter {
    nlohmann::json toJson(const Patch& patch);
  }

} // namespace gen
} // namespace corpuscular
