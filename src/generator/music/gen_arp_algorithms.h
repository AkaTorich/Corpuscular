/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "gen_phrase.h"

#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  class Random;
  class Scale;

  // 16-step melodic phrases, ported from the Spire generator (PsyArp, Melody, Chord, Rhythmic, Bass, MIDI base)
  class ArpAlgorithms {
    public:
      static const std::vector<std::string>& kinds();

      explicit ArpAlgorithms(Random& random) : random_(random) { }

      // kind is one of kinds() or "random"
      Phrase make(const std::string& kind, const Scale& scale);

      // Steps where a note ties into the next one (legato), patterns from the Spire generator
      std::vector<int> holdPattern();

    private:
      void midi(Phrase& phrase, const Scale& scale);
      void psyArp(Phrase& phrase, const std::vector<int>& scale);
      void melody(Phrase& phrase, const std::vector<int>& scale);
      void chord(Phrase& phrase, const std::vector<int>& scale);
      void rhythmic(Phrase& phrase, const std::vector<int>& scale);
      void bass(Phrase& phrase, const std::vector<int>& scale);

      std::vector<int> motif(const std::vector<int>& scale, int length);
      static int contourOffset(int position, int total, int contour);

      double range(double a, double b);
      int rangeInt(int a, int b);
      static int positiveMod(int a, int m) { return ((a % m) + m) % m; }

      Random& random_;
  };

} // namespace gen
} // namespace corpuscular
