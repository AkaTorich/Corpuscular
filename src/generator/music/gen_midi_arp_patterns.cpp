/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_midi_arp_patterns.h"

#include "../gen_random.h"

namespace corpuscular {
namespace gen {

  const int* MidiArpPatterns::getRandom(Random& random) {
    return kPatterns[random.next(kNumPatterns)];
  }

} // namespace gen
} // namespace corpuscular
