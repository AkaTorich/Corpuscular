/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_lorem_generator.h"

#include "../gen_random.h"

namespace corpuscular {
namespace gen {

  namespace LoremGenerator {
    static const char* const kLoremOnsets[] = { "", "b", "c", "d", "f", "g", "l", "m", "n", "p", "qu", "r", "s", "t", "v" };
    static const char* const kLoremVowels[] = { "a", "e", "i", "o", "u", "ae", "au", "ei", "eu", "ia", "io", "iu", "ua", "ue" };
    static const char* const kLoremCodas[] = { "", "", "", "m", "n", "s", "t", "r", "l", "x", "ns", "nt", "rs", "ct" };

    static const int kLoremNumOnsets = sizeof(kLoremOnsets) / sizeof(kLoremOnsets[0]);
    static const int kLoremNumVowels = sizeof(kLoremVowels) / sizeof(kLoremVowels[0]);
    static const int kLoremNumCodas = sizeof(kLoremCodas) / sizeof(kLoremCodas[0]);

    std::string nextWord(Random& random) {
      int syllables = 2 + random.next(2);
      std::string word;
      for (int i = 0; i < syllables; ++i) {
        word += kLoremOnsets[random.next(kLoremNumOnsets)];
        word += kLoremVowels[random.next(kLoremNumVowels)];
        if (i == syllables - 1)
          word += kLoremCodas[random.next(kLoremNumCodas)];
      }
      if (!word.empty() && word[0] >= 'a' && word[0] <= 'z')
        word[0] = static_cast<char>(word[0] - 'a' + 'A');
      return word;
    }
  }

} // namespace gen
} // namespace corpuscular
