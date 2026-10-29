/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>

namespace corpuscular {
namespace gen {

  class Random;

  // Pronounceable two-word wavetable names, same scheme as the preset generators
  namespace NameGenerator {
    std::string nextWord(Random& random);
    std::string nextName(Random& random);
  }

} // namespace gen
} // namespace corpuscular
