/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>

namespace corpuscular {
namespace gen {

  class Random;

  // Latin-sounding words for preset and bank names
  namespace LoremGenerator {
    // One capitalized word of two or three syllables
    std::string nextWord(Random& random);
  }

} // namespace gen
} // namespace corpuscular
