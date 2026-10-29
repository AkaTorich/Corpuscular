/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_name_generator.h"

#include "../gen_random.h"

namespace corpuscular {
namespace gen {

  namespace NameGenerator {
    static const char* const kNameGeneratorOnsets[] = {
      "", "b", "c", "d", "f", "g", "k", "l", "m", "n", "p", "r", "s", "t", "v", "z", "x"
    };
    static const char* const kNameGeneratorVowels[] = { "a", "e", "i", "o", "u", "ae", "au", "ei", "ia", "io", "ua", "y" };
    static const char* const kNameGeneratorCodas[] = { "", "", "", "m", "n", "s", "t", "r", "l", "x", "ns", "nt", "rk", "ct" };

    static const int kNameGeneratorNumOnsets = sizeof(kNameGeneratorOnsets) / sizeof(kNameGeneratorOnsets[0]);
    static const int kNameGeneratorNumVowels = sizeof(kNameGeneratorVowels) / sizeof(kNameGeneratorVowels[0]);
    static const int kNameGeneratorNumCodas = sizeof(kNameGeneratorCodas) / sizeof(kNameGeneratorCodas[0]);

    std::string nextWord(Random& random) {
      int syllables = 2 + random.next(2);
      std::string word;
      for (int i = 0; i < syllables; ++i) {
        word += kNameGeneratorOnsets[random.next(kNameGeneratorNumOnsets)];
        word += kNameGeneratorVowels[random.next(kNameGeneratorNumVowels)];
        if (i == syllables - 1)
          word += kNameGeneratorCodas[random.next(kNameGeneratorNumCodas)];
      }
      if (!word.empty() && word[0] >= 'a' && word[0] <= 'z')
        word[0] = static_cast<char>(word[0] - 'a' + 'A');
      return word;
    }

    std::string nextName(Random& random) {
      std::string first = nextWord(random);
      return first + " " + nextWord(random);
    }
  }

} // namespace gen
} // namespace corpuscular
