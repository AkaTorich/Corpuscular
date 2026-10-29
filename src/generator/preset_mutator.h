/* Corpuscular preset mutator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "json/json.h"

#include <cstdint>

namespace corpuscular {
namespace gen {

  // A close relative of a preset: continuous knobs move a little (amount is the share of each knob's range,
  // about 0.04..0.15), switches, modes, tuning and wavetables stay; stepped LFO melodies swap a few steps,
  // so the notes stay in the same scale.
  nlohmann::json mutatePreset(const nlohmann::json& preset, uint32_t seed, double amount = 0.08);

} // namespace gen
} // namespace corpuscular
