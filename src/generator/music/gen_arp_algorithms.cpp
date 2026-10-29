/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_arp_algorithms.h"

#include "gen_midi_arp_patterns.h"
#include "gen_scales.h"
#include "../gen_random.h"
#include "../wavetables/gen_wave_math.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>

namespace corpuscular {
namespace gen {

  const std::vector<std::string>& ArpAlgorithms::kinds() {
    static const std::vector<std::string> kinds = { "psyarp", "melody", "chord", "rhythmic", "bass", "midi" };
    return kinds;
  }

  Phrase ArpAlgorithms::make(const std::string& requested_kind, const Scale& scale) {
    std::string kind = requested_kind;
    if (kind.empty() || kind == "random") {
      const std::vector<std::string>& all = kinds();
      kind = random_.nextDouble() < 0.4 ? "midi" : all[random_.next(static_cast<int>(all.size()) - 1)];
    }

    const std::vector<int>& s = scale.semitones;
    Phrase phrase;
    phrase.kind = kind;
    if (kind == "melody")
      melody(phrase, s);
    else if (kind == "chord")
      chord(phrase, s);
    else if (kind == "rhythmic")
      rhythmic(phrase, s);
    else if (kind == "bass")
      bass(phrase, s);
    else if (kind == "midi")
      midi(phrase, scale);
    else
      psyArp(phrase, s);

    for (int i = 0; i < Phrase::kSteps; ++i)
      phrase.notes[i] = scale.quantize(phrase.notes[i]);
    phrase.fold();
    return phrase;
  }

  std::vector<int> ArpAlgorithms::holdPattern() {
    static const std::vector<std::vector<std::vector<int>>> categories = {
      { { 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15 }, { 1, 2, 3, 4, 9, 10, 11, 12 }, { 1, 2, 5, 6, 9, 10, 13, 14 } },
      { { 1, 2, 9, 10 }, { 1, 2, 3, 13, 14, 15 }, { 5, 6, 13, 14 }, { 3, 4, 11, 12 } },
      { { 2, 3, 6, 7, 10, 11, 14, 15 }, { 4, 5, 12, 13 }, { 2, 3, 10, 11 }, { 3, 4, 7, 8, 11, 12, 15, 16 } },
      { { 1, 2, 5, 9, 10, 13 }, { 1, 4, 5, 8, 9, 12, 13, 16 }, { 1, 2, 7, 8, 9, 10, 15, 16 } },
      { { 1, 9 }, { 5, 13 }, { 4, 12 }, { 8, 16 }, { 2, 6, 10, 14 } },
      { { 1, 3, 5, 7, 9, 11, 13, 15 }, { 1, 5, 9, 13 }, { 1, 3, 9, 11 }, { 4, 8, 12, 16 } },
      { std::vector<int>(), std::vector<int>() },
    };
    const std::vector<std::vector<int>>& category = categories[random_.next(static_cast<int>(categories.size()))];
    return category[random_.next(static_cast<int>(category.size()))];
  }

  void ArpAlgorithms::midi(Phrase& phrase, const Scale& scale) {
    const int* pattern = MidiArpPatterns::getRandom(random_);
    for (int i = 0; i < Phrase::kSteps; ++i) {
      phrase.notes[i] = scale.quantize(pattern[i]);
      phrase.velocity[i] = i % 4 == 0 ? range(0.80, 0.98) : (i % 2 == 0 ? range(0.65, 0.88) : range(0.55, 0.82));
    }
    for (int step : holdPattern())
      phrase.hold[step - 1] = 1.0;
  }

  void ArpAlgorithms::psyArp(Phrase& phrase, const std::vector<int>& scale) {
    std::vector<int> extended;
    for (int octave = 0; octave <= 2; ++octave) {
      for (int degree : scale)
        extended.push_back(degree + octave * 12);
    }
    int count = static_cast<int>(extended.size());
    int length = static_cast<int>(scale.size());

    int contour = rangeInt(0, 7);
    int gravity_pull = rangeInt(4, 10);
    int position = rangeInt(0, count / 2);
    int direction = random_.nextDouble() < 0.5 ? 1 : -1;
    double step_bias = range(0.3, 0.7);
    double octave_jump = range(0.05, 0.25);
    double direction_change = range(0.1, 0.3);

    std::vector<int> gravity;
    switch (rangeInt(0, 4)) {
      case 0: gravity = { 0 }; break;
      case 1: gravity = { 0, 12 }; break;
      case 2: gravity = { 12 }; break;
      case 3: gravity = { 0, 7, 12 }; break;
      default: gravity = { 0, scale[length / 2], 12 }; break;
    }
    int gravity_count = static_cast<int>(gravity.size());

    for (int i = 0; i < Phrase::kSteps; ++i) {
      bool home = i == 0 || i == Phrase::kSteps - 1 || i % gravity_pull == 0;
      int semitone;
      if (home) {
        int target;
        if (contour <= 1)
          target = gravity[i < 8 ? 0 : gravity_count - 1];
        else if (contour == 2)
          target = gravity[i < 8 ? gravity_count - 1 : 0];
        else if (contour == 3) {
          int index = static_cast<int>(std::sin(static_cast<double>(i) / 16 * WaveMath::kPi) * (gravity_count - 1) + 0.5);
          target = gravity[std::min(index, gravity_count - 1)];
        }
        else
          target = gravity[random_.next(gravity_count)];

        int best = 0;
        int best_distance = INT_MAX;
        for (int k = 0; k < count; ++k) {
          int distance = std::abs(extended[k] - target);
          if (distance < best_distance) {
            best_distance = distance;
            best = k;
          }
        }
        position = best;
        semitone = extended[position];
      }
      else {
        double r = random_.nextDouble();
        if (r < step_bias)
          position += direction;
        else if (r < step_bias + octave_jump)
          position += direction * length;
        else if (r < step_bias + octave_jump + direction_change) {
          direction = -direction;
          position += direction;
        }
        else
          position += direction * rangeInt(2, 3);

        if (position < 0) {
          position = -position;
          direction = 1;
        }
        if (position >= count) {
          position = 2 * (count - 1) - position;
          direction = -1;
        }
        position = std::max(0, std::min(position, count - 1));
        semitone = extended[position];
      }
      phrase.notes[i] = semitone;
      phrase.velocity[i] = i % 4 == 0 ? range(0.80, 0.98) : (i % 2 == 0 ? range(0.65, 0.88) : range(0.55, 0.82));
      phrase.hold[i] = (i == 7 || i == 15) ? range(0.3, 0.7) : (random_.nextDouble() < 0.10 ? range(0.2, 0.6) : 0.0);
    }
  }

  void ArpAlgorithms::melody(Phrase& phrase, const std::vector<int>& scale) {
    static const int kStableDegrees[] = { 0, 2, 4 };
    int length = static_cast<int>(scale.size());
    std::vector<int> theme = motif(scale, 4);
    int theme_length = static_cast<int>(theme.size());
    int contour = rangeInt(0, 3);
    const int half = 8;

    for (int i = 0; i < Phrase::kSteps; ++i) {
      bool call = i < half;
      bool phrase_end = i == half - 1 || i == Phrase::kSteps - 1;
      bool down = i % 4 == 0;
      bool peak = i == 5 || i == 10;
      int index = i % theme_length;
      int degree;
      if (phrase_end)
        degree = std::min(kStableDegrees[rangeInt(0, 2)], length - 1);
      else if (call)
        degree = positiveMod(theme[index] + contourOffset(i, half, contour), length);
      else {
        int variant = rangeInt(0, 2);
        degree = variant == 0 ? positiveMod(theme[index] + 2, length)
               : variant == 1 ? positiveMod(length - 1 - theme[index], length)
               : theme[theme_length - 1 - index];
        degree = positiveMod(degree + contourOffset(i - half, half, contour), length);
      }

      phrase.notes[i] = scale[degree] + (peak && random_.nextDouble() < 0.5 ? 12 : 0);
      bool ghost = !down && !phrase_end && !peak && random_.nextDouble() < 0.2;
      phrase.velocity[i] = ghost ? range(0.25, 0.4) : peak ? range(0.85, 0.98) : down ? range(0.72, 0.88)
                         : phrase_end ? range(0.55, 0.72) : range(0.58, 0.78);
      phrase.hold[i] = phrase_end ? range(0.5, 0.9) : (!down && random_.nextDouble() < 0.15) ? range(0.4, 0.8)
                     : i % 8 == 7 ? range(0.3, 0.6) : 0.0;
    }
  }

  void ArpAlgorithms::chord(Phrase& phrase, const std::vector<int>& scale) {
    static const int kProgressions[][4][3] = {
      { { 0, 2, 4 }, { 4, 6, 1 }, { 5, 0, 2 }, { 3, 5, 0 } },
      { { 0, 2, 4 }, { 3, 5, 0 }, { 4, 6, 1 }, { 0, 2, 4 } },
      { { 5, 0, 2 }, { 3, 5, 0 }, { 0, 2, 4 }, { 4, 6, 1 } },
      { { 1, 3, 5 }, { 4, 6, 1 }, { 0, 2, 4 }, { 0, 2, 4 } },
      { { 0, 2, 4 }, { 5, 0, 2 }, { 3, 5, 0 }, { 4, 6, 1 } },
    };
    static const int kNumProgressions = sizeof(kProgressions) / sizeof(kProgressions[0]);

    int length = static_cast<int>(scale.size());
    const int (&progression)[4][3] = kProgressions[random_.next(kNumProgressions)];
    for (int i = 0; i < Phrase::kSteps; ++i) {
      const int* chord_degrees = progression[(i / 4) % 4];
      int degree = chord_degrees[i % 3] % length;
      int semitone = scale[degree] + (i % 4 == 0 && i > 0 && random_.nextDouble() < 0.25 ? 12 : 0);
      phrase.notes[i] = semitone;
      bool ghost = i % 4 != 0 && random_.nextDouble() < 0.15;
      bool sync = i % 4 == 2 && random_.nextDouble() < 0.3;
      phrase.velocity[i] = ghost ? range(0.25, 0.4) : i % 4 == 0 ? range(0.82, 0.96) : sync ? range(0.78, 0.9) : range(0.55, 0.75);
      phrase.hold[i] = i % 4 == 3 ? range(0.5, 1.0) : sync ? range(0.3, 0.7) : 0.0;
    }
  }

  void ArpAlgorithms::rhythmic(Phrase& phrase, const std::vector<int>& scale) {
    static const int kRhythms[][8] = {
      { 1, 0, 1, 0, 1, 0, 1, 0 }, { 1, 0, 0, 1, 0, 0, 1, 0 }, { 1, 0, 1, 0, 0, 1, 0, 0 },
      { 1, 1, 0, 1, 0, 1, 0, 0 }, { 1, 0, 0, 1, 0, 1, 0, 1 }, { 1, 0, 1, 1, 0, 0, 1, 0 },
    };
    static const int kNumRhythms = sizeof(kRhythms) / sizeof(kRhythms[0]);

    int length = static_cast<int>(scale.size());
    const int* rhythm = kRhythms[random_.next(kNumRhythms)];
    std::vector<int> theme = motif(scale, 4);
    int theme_length = static_cast<int>(theme.size());
    int m = 0;
    for (int i = 0; i < Phrase::kSteps; ++i) {
      int degree = std::min(theme[m % theme_length], length - 1);
      if (rhythm[i % 8] == 1) {
        if (i >= 8)
          degree = std::max(0, degree - 1);
        m++;
        phrase.notes[i] = scale[degree];
        phrase.velocity[i] = i % 4 == 0 ? range(0.8, 0.95) : random_.nextDouble() < 0.35 ? range(0.75, 0.9) : range(0.55, 0.75);
        phrase.hold[i] = range(0.2, 0.6);
      }
      else {
        phrase.notes[i] = i > 0 ? phrase.notes[i - 1] : scale[degree];
        phrase.velocity[i] = random_.nextDouble() < 0.2 ? range(0.2, 0.35) : 0.0;
      }
    }
  }

  void ArpAlgorithms::bass(Phrase& phrase, const std::vector<int>& scale) {
    static const int kBassMotifs[][4] = { { 0, 0, 4, 0 }, { 0, 2, 4, 2 }, { 0, 4, 0, 6 }, { 0, 0, 3, 4 }, { 0, 2, 0, 4 } };
    static const int kNumBassMotifs = sizeof(kBassMotifs) / sizeof(kBassMotifs[0]);

    int length = static_cast<int>(scale.size());
    const int* theme = kBassMotifs[random_.next(kNumBassMotifs)];
    int shift = random_.nextDouble() < 0.5 ? 3 : 4;
    for (int i = 0; i < Phrase::kSteps; ++i) {
      bool down = i % 4 == 0;
      bool end = i == 7 || i == 15;
      int degree = end ? 0 : theme[i % 4];
      if (!end && i >= 8)
        degree = (degree + shift) % length;
      degree %= length;
      phrase.notes[i] = scale[degree] - 12 + (down && i > 0 && random_.nextDouble() < 0.2 ? 12 : 0);
      bool ghost = !down && random_.nextDouble() < 0.18;
      phrase.velocity[i] = ghost ? range(0.25, 0.4) : down ? range(0.85, 0.97) : end ? range(0.7, 0.85) : range(0.55, 0.75);
      phrase.hold[i] = down ? range(0.6, 0.95) : end ? range(0.5, 0.9) : range(0.2, 0.5);
    }
  }

  std::vector<int> ArpAlgorithms::motif(const std::vector<int>& scale, int length) {
    static const int kMotifStart[] = { 0, 2, 4 };
    int scale_length = static_cast<int>(scale.size());
    std::vector<int> result(length);
    result[0] = kMotifStart[rangeInt(0, 2)] % scale_length;
    for (int i = 1; i < length; ++i) {
      int step = rangeInt(-2, 2);
      if (step == 0)
        step = 1;
      result[i] = positiveMod(result[i - 1] + step, scale_length);
    }
    return result;
  }

  int ArpAlgorithms::contourOffset(int position, int total, int contour) {
    double phase = static_cast<double>(position) / total;
    switch (contour) {
      case 0: return static_cast<int>(roundHalfEven(std::sin(phase * WaveMath::kPi) * 2.0));
      case 1: return static_cast<int>(roundHalfEven((1.0 - phase) * 2.0));
      case 2: return static_cast<int>(roundHalfEven(phase * 3.0));
      default: return static_cast<int>(roundHalfEven(std::sin(phase * WaveMath::kPi * 2.0) * 2.0));
    }
  }

  double ArpAlgorithms::range(double a, double b) {
    return a + random_.nextDouble() * (b - a);
  }

  int ArpAlgorithms::rangeInt(int a, int b) {
    return random_.next(a, b + 1);
  }

} // namespace gen
} // namespace corpuscular
