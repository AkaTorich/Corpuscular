/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_table_types.h"

#include "gen_wave_math.h"
#include "../gen_random.h"

#include <algorithm>
#include <cmath>

namespace corpuscular {
namespace gen {

  // ---- Basic Morph ----

  static std::vector<double> basicMorphShape(int kind, int harmonics, Random& random) {
    std::vector<double> a(harmonics, 0.0);
    double pulse_width = 0.1 + random.nextDouble() * 0.35;
    for (int n = 1; n <= harmonics; ++n) {
      switch (kind) {
        case 0: a[n - 1] = n == 1 ? 1.0 : 0.0; break;                                                   // sine
        case 1: a[n - 1] = (n % 2 == 1) ? ((n % 4 == 1 ? 1 : -1) / static_cast<double>(n * n)) : 0.0; break; // triangle
        case 2: a[n - 1] = 1.0 / n; break;                                                              // saw
        case 3: a[n - 1] = (n % 2 == 1) ? 1.0 / n : 0.0; break;                                         // square
        default: a[n - 1] = std::sin(WaveMath::kPi * n * pulse_width) / n; break;                                 // pulse
      }
    }
    return a;
  }

  WaveFrames BasicMorphType::generate(Random& random, int frames, int size) const {
    int harmonics = std::min(128, size / 4);
    std::vector<int> order = { 0, 1, 2, 3, 4 };
    for (int i = static_cast<int>(order.size()) - 1; i > 0; --i) {
      int j = random.next(i + 1);
      std::swap(order[i], order[j]);
    }
    int keys = 3 + random.next(3);
    std::vector<std::vector<double>> specs;
    for (int k = 0; k < keys; ++k)
      specs.push_back(basicMorphShape(order[k % order.size()], harmonics, random));

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double p = WaveMath::framePos(f, frames) * (keys - 1);
      int i0 = std::min(static_cast<int>(p), keys - 2);
      double t = p - i0;
      t = t * t * (3.0 - 2.0 * t);
      std::vector<double> amps = WaveMath::lerpArray(specs[i0], specs[i0 + 1], t);
      result[f] = WaveMath::fromHarmonics(size, amps, {});
    }
    return result;
  }

  // ---- Additive ----

  WaveFrames AdditiveType::generate(Random& random, int frames, int size) const {
    int harmonics = std::min(64, size / 8);
    int keys = 3 + random.next(3);
    double tilt = 0.6 + random.nextDouble() * 1.2;
    std::vector<std::vector<double>> specs;
    for (int k = 0; k < keys; ++k) {
      double sparsity = 0.3 + random.nextDouble() * 0.4;
      specs.push_back(WaveMath::randomSpectrum(random, harmonics, tilt, sparsity));
    }
    std::vector<double> phases;
    if (random.nextDouble() < 0.5)
      phases = WaveMath::randomPhases(random, harmonics);

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double p = WaveMath::framePos(f, frames) * (keys - 1);
      int i0 = std::min(static_cast<int>(p), keys - 2);
      double t = p - i0;
      result[f] = WaveMath::fromHarmonics(size, WaveMath::lerpArray(specs[i0], specs[i0 + 1], t), phases);
    }
    return result;
  }

  // ---- FM ----

  static const int kFmTypeRatios[] = { 1, 1, 2, 2, 3, 3, 4, 5, 7 };
  static const int kFmTypeNumRatios = sizeof(kFmTypeRatios) / sizeof(kFmTypeRatios[0]);

  WaveFrames FmType::generate(Random& random, int frames, int size) const {
    int ratio = kFmTypeRatios[random.next(kFmTypeNumRatios)];
    int ratio2 = random.nextDouble() < 0.4 ? kFmTypeRatios[random.next(kFmTypeNumRatios)] : 0;
    double max_index = 1.5 + random.nextDouble() * 6.0;
    double feedback = random.nextDouble() < 0.4 ? random.nextDouble() * 0.6 : 0.0;

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double index = max_index * std::pow(WaveMath::framePos(f, frames), 1.3);
      std::vector<float> buffer(size);
      double previous = 0.0;
      for (int i = 0; i < size; ++i) {
        double x = static_cast<double>(i) / size;
        double mod = std::sin(WaveMath::kTwoPi * ratio * x + feedback * previous);
        if (ratio2 > 0)
          mod += 0.5 * std::sin(WaveMath::kTwoPi * ratio2 * x);
        previous = mod;
        buffer[i] = static_cast<float>(std::sin(WaveMath::kTwoPi * x + index * mod));
      }
      result[f] = buffer;
    }
    return result;
  }

  // ---- Phase Distortion ----

  WaveFrames PhaseDistortionType::generate(Random& random, int frames, int size) const {
    int variant = random.next(3);
    double max_amount = 0.6 + random.nextDouble() * 0.38;

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double d = max_amount * WaveMath::framePos(f, frames);
      std::vector<float> buffer(size);
      for (int i = 0; i < size; ++i) {
        double x = static_cast<double>(i) / size;
        double knee = 0.5 - d * 0.49;
        double phase = x < knee ? 0.5 * x / knee : 0.5 + 0.5 * (x - knee) / (1.0 - knee);
        double y;
        switch (variant) {
          case 0: y = std::cos(WaveMath::kTwoPi * phase); break;                                  // saw-ish
          case 1: y = std::cos(WaveMath::kTwoPi * phase * (1.0 + 2.0 * d)) * (1.0 - x); break;   // resonant
          default: y = std::cos(WaveMath::kTwoPi * phase) * std::cos(WaveMath::kTwoPi * x * d * 3.0); break;
        }
        buffer[i] = static_cast<float>(y);
      }
      if (variant != 0)
        WaveMath::makeLoop(buffer, 0.05);
      result[f] = buffer;
    }
    return result;
  }

  // ---- Formant ----

  WaveFrames FormantType::generate(Random& random, int frames, int size) const {
    int harmonics = std::min(96, size / 4);
    int peaks = 2 + random.next(2);
    std::vector<double> start(peaks), end(peaks), width(peaks);
    for (int k = 0; k < peaks; ++k) {
      start[k] = 2.0 + random.nextDouble() * 30.0;
      end[k] = 2.0 + random.nextDouble() * 40.0;
      width[k] = 1.5 + random.nextDouble() * 4.0;
    }
    double tilt = 0.5 + random.nextDouble() * 0.6;
    std::vector<double> phases = WaveMath::randomPhases(random, harmonics);

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double t = WaveMath::framePos(f, frames);
      std::vector<double> amps(harmonics);
      for (int n = 1; n <= harmonics; ++n) {
        double gain = 0.05;
        for (int k = 0; k < peaks; ++k) {
          double center = WaveMath::lerp(start[k], end[k], t);
          double d = (n - center) / width[k];
          gain += std::exp(-d * d);
        }
        amps[n - 1] = gain / std::pow(n, tilt);
      }
      result[f] = WaveMath::fromHarmonics(size, amps, phases);
    }
    return result;
  }

  // ---- Wavefold ----

  static double wavefoldTypeFold(double x) {
    x = (x + 1.0) * 0.25;
    x = x - std::floor(x);
    return std::abs(x * 4.0 - 2.0) - 1.0;
  }

  WaveFrames WavefoldType::generate(Random& random, int frames, int size) const {
    double max_gain = 2.5 + random.nextDouble() * 7.0;
    double offset_max = random.nextDouble() * 0.8;
    bool saw_source = random.nextDouble() < 0.3;

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double t = WaveMath::framePos(f, frames);
      double gain = 1.0 + (max_gain - 1.0) * t;
      double offset = offset_max * t;
      std::vector<float> buffer(size);
      for (int i = 0; i < size; ++i) {
        double x = static_cast<double>(i) / size;
        double source = saw_source ? (2.0 * x - 1.0) : std::sin(WaveMath::kTwoPi * x);
        buffer[i] = static_cast<float>(wavefoldTypeFold(source * gain + offset));
      }
      if (saw_source)
        WaveMath::makeLoop(buffer, 0.03);
      result[f] = buffer;
    }
    return result;
  }

  // ---- Hard Sync ----

  WaveFrames HardSyncType::generate(Random& random, int frames, int size) const {
    double max_ratio = 2.0 + random.nextDouble() * 5.0;
    bool square = random.nextDouble() < 0.4;
    bool window = random.nextDouble() < 0.5;

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double ratio = 1.0 + (max_ratio - 1.0) * WaveMath::framePos(f, frames);
      std::vector<float> buffer(size);
      for (int i = 0; i < size; ++i) {
        double x = static_cast<double>(i) / size;
        double phase = WaveMath::frac(ratio * x);
        double y = square ? (phase < 0.5 ? 1.0 : -1.0) : (2.0 * phase - 1.0);
        if (window)
          y *= 1.0 - x * x;
        buffer[i] = static_cast<float>(y);
      }
      WaveMath::makeLoop(buffer, 0.04);
      result[f] = buffer;
    }
    return result;
  }

  // ---- PWM ----

  WaveFrames PwmType::generate(Random& random, int frames, int size) const {
    int harmonics = std::min(128, size / 4);
    double width_start = 0.5;
    double width_end = 0.03 + random.nextDouble() * 0.12;
    double saw_mix = random.nextDouble() * 0.5;

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double t = WaveMath::framePos(f, frames);
      double width = WaveMath::lerp(width_start, width_end, t);
      std::vector<double> amps(harmonics);
      for (int n = 1; n <= harmonics; ++n)
        amps[n - 1] = std::sin(WaveMath::kPi * n * width) / n * (1.0 - saw_mix) + saw_mix * t / n;
      result[f] = WaveMath::fromHarmonics(size, amps, {});
    }
    return result;
  }

  // ---- Spectral ----

  WaveFrames SpectralType::generate(Random& random, int frames, int size) const {
    int harmonics = std::min(128, size / 4);
    int keys = 2 + random.next(3);
    double tilt = 0.2 + random.nextDouble() * 0.8;
    std::vector<std::vector<double>> amps, phases;
    for (int k = 0; k < keys; ++k) {
      double sparsity = 0.1 + random.nextDouble() * 0.5;
      amps.push_back(WaveMath::randomSpectrum(random, harmonics, tilt, sparsity));
      phases.push_back(WaveMath::randomPhases(random, harmonics));
    }

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double p = WaveMath::framePos(f, frames) * (keys - 1);
      int i0 = std::min(static_cast<int>(p), keys - 2);
      double t = p - i0;
      result[f] = WaveMath::fromHarmonics(size, WaveMath::lerpArray(amps[i0], amps[i0 + 1], t), WaveMath::lerpArray(phases[i0], phases[i0 + 1], t));
    }
    return result;
  }

  // ---- Chaos ----

  WaveFrames ChaosType::generate(Random& random, int frames, int size) const {
    double r_start = 3.55 + random.nextDouble() * 0.2;
    double r_end = 3.9 + random.nextDouble() * 0.1;
    double seed = 0.1 + random.nextDouble() * 0.8;
    int hold = 1 + random.next(size / 64);

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double t = WaveMath::framePos(f, frames);
      double r = WaveMath::lerp(r_start, r_end, t);
      double cut = WaveMath::lerp(0.02, 0.35, t);
      std::vector<float> buffer(size);
      double x = seed, low_pass = 0.0, current = 0.0;
      for (int i = 0; i < size; ++i) {
        if (i % hold == 0) {
          x = r * x * (1.0 - x);
          current = x * 2.0 - 1.0;
        }
        low_pass += cut * (current - low_pass);
        buffer[i] = static_cast<float>(low_pass);
      }
      WaveMath::makeLoop(buffer, 0.08);
      result[f] = buffer;
    }
    return result;
  }

  // ---- Psy Bass ----

  WaveFrames PsyBassType::generate(Random& random, int frames, int size) const {
    int harmonics = std::min(128, size / 4);
    double q = 1.5 + random.nextDouble() * 6.0;
    double cut_start = 1.5 + random.nextDouble() * 2.0;
    double cut_end = 12.0 + random.nextDouble() * 50.0;
    double square_mix = random.nextDouble();
    double sub_boost = 0.5 + random.nextDouble();

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double t = WaveMath::framePos(f, frames);
      double cut = std::exp(WaveMath::lerp(std::log(cut_start), std::log(cut_end), t));
      std::vector<double> amps(harmonics);
      for (int n = 1; n <= harmonics; ++n) {
        double source = (1.0 - square_mix) / n + square_mix * ((n % 2 == 1) ? 1.0 / n : 0.0);
        double w = n / cut;
        double low_pass = 1.0 / std::sqrt(std::pow(1.0 - w * w, 2.0) + std::pow(w / q, 2.0));
        low_pass = std::min(low_pass, q);
        amps[n - 1] = source * low_pass;
      }
      amps[0] *= sub_boost;
      result[f] = WaveMath::fromHarmonics(size, amps, {});
    }
    return result;
  }

  // ---- Digital ----

  WaveFrames DigitalType::generate(Random& random, int frames, int size) const {
    int shape = random.next(3);
    int min_bits = 2 + random.next(3);
    int max_hold = 4 + random.next(size / 32);

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double t = WaveMath::framePos(f, frames);
      double bits = WaveMath::lerp(12.0, min_bits, t);
      int hold = 1 + static_cast<int>(max_hold * t);
      double levels = std::pow(2.0, bits);
      std::vector<float> buffer(size);
      double held = 0.0;
      for (int i = 0; i < size; ++i) {
        double x = static_cast<double>(i) / size;
        double y = shape == 0 ? std::sin(WaveMath::kTwoPi * x) : shape == 1 ? (2.0 * x - 1.0) : (1.0 - 4.0 * std::abs(x - 0.5));
        if (i % hold == 0)
          held = roundHalfEven(y * levels) / levels;
        buffer[i] = static_cast<float>(held);
      }
      if (shape == 1)
        WaveMath::makeLoop(buffer, 0.03);
      result[f] = buffer;
    }
    return result;
  }

  // ---- Vocal ----

  // Formant centres in harmonics of a ~110 Hz fundamental: a, e, i, o, u
  static const double kVocalTypeVowels[][3] = {
    { 7.3, 11.0, 24.0 }, { 3.6, 20.0, 25.0 }, { 2.5, 21.0, 27.0 },
    { 4.1, 8.0, 24.0 }, { 3.0, 7.0, 22.0 }
  };
  static const int kVocalTypeNumVowels = sizeof(kVocalTypeVowels) / sizeof(kVocalTypeVowels[0]);

  WaveFrames VocalType::generate(Random& random, int frames, int size) const {
    int harmonics = std::min(96, size / 4);
    int keys = 2 + random.next(3);
    std::vector<int> sequence(keys);
    for (int k = 0; k < keys; ++k)
      sequence[k] = random.next(kVocalTypeNumVowels);
    double breath = random.nextDouble() * 0.08;
    std::vector<double> phases = WaveMath::randomPhases(random, harmonics);
    std::vector<double> noise = WaveMath::randomSpectrum(random, harmonics, 0.3, 0.2);

    WaveFrames result(frames);
    for (int f = 0; f < frames; ++f) {
      double p = WaveMath::framePos(f, frames) * (keys - 1);
      int i0 = std::min(static_cast<int>(p), keys - 2);
      double t = p - i0;
      std::vector<double> amps(harmonics);
      for (int n = 1; n <= harmonics; ++n) {
        double gain = 0.0;
        for (int k = 0; k < 3; ++k) {
          double center = WaveMath::lerp(kVocalTypeVowels[sequence[i0]][k], kVocalTypeVowels[sequence[i0 + 1]][k], t);
          double formant_width = 1.2 + k * 1.2;
          double d = (n - center) / formant_width;
          gain += std::exp(-d * d) * (k == 0 ? 1.0 : 0.6);
        }
        amps[n - 1] = gain / std::pow(n, 0.4) + breath * noise[n - 1];
      }
      result[f] = WaveMath::fromHarmonics(size, amps, phases);
    }
    return result;
  }

} // namespace gen
} // namespace corpuscular
