/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <vector>

namespace corpuscular {
namespace gen {

  class Random;

  // Shared numeric helpers: additive synthesis, normalization, smoothing
  namespace WaveMath {
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kTwoPi = kPi * 2.0;

    inline double lerp(double a, double b, double t) { return a + (b - a) * t; }
    double frac(double x);
    inline double clamp(double x, double low, double high) { return x < low ? low : (x > high ? high : x); }

    // Position of frame f in [0..1] across the table
    inline double framePos(int f, int frames) { return frames <= 1 ? 0.0 : static_cast<double>(f) / (frames - 1); }

    // Sum of sines: amps[n - 1] is the amplitude of harmonic n, phases in radians (empty = zero phases)
    std::vector<float> fromHarmonics(int size, const std::vector<double>& amps, const std::vector<double>& phases);

    void removeDc(std::vector<float>& buffer);
    void normalize(std::vector<float>& buffer, float peak = 0.98f);

    // 3-tap circular smoothing, tames aliasing of time-domain shapes
    void smooth(std::vector<float>& buffer, int passes);

    // Crossfade the tail into the head so the cycle loops without a click
    void makeLoop(std::vector<float>& buffer, double fraction);

    // Random harmonic spectrum with a spectral tilt and sparsity
    std::vector<double> randomSpectrum(Random& random, int harmonics, double tilt, double sparsity);
    std::vector<double> randomPhases(Random& random, int harmonics);

    std::vector<double> lerpArray(const std::vector<double>& a, const std::vector<double>& b, double t);
  }

} // namespace gen
} // namespace corpuscular
