/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "../format/gen_wavetable.h"

#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  class Random;

  // Oscillator wavetables: frames from the wave kinds, vital crossfades between the keyframes
  namespace WavetableFactory {
    std::vector<std::string> kinds();

    // vital's default table: one saw
    WavetableData init();

    // Sine -> triangle -> saw -> square
    WavetableData basicShapes();

    WavetableData make(const std::string& kind, Random& random, int keyframes, const std::string& name);
  }

} // namespace gen
} // namespace corpuscular
