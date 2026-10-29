/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_wave_math.h"

#include "../gen_random.h"

#include <algorithm>
#include <cmath>

namespace corpuscular {
namespace gen {

  namespace WaveMath {
    double frac(double x) { return x - std::floor(x); }

    std::vector<float> fromHarmonics(int size, const std::vector<double>& amps, const std::vector<double>& phases) {
      std::vector<float> buffer(size, 0.0f);
      int max_harmonic = std::min(static_cast<int>(amps.size()), size / 2 - 1);
      for (int n = 1; n <= max_harmonic; ++n) {
        double amp = amps[n - 1];
        if (std::abs(amp) < 1e-6)
          continue;
        double phase = phases.empty() ? 0.0 : phases[n - 1];
        double step = kTwoPi * n / size;
        for (int i = 0; i < size; ++i)
          buffer[i] += static_cast<float>(amp * std::sin(step * i + phase));
      }
      return buffer;
    }

    void removeDc(std::vector<float>& buffer) {
      double sum = 0.0;
      for (float sample : buffer)
        sum += sample;
      float mean = static_cast<float>(sum / buffer.size());
      for (float& sample : buffer)
        sample -= mean;
    }

    void normalize(std::vector<float>& buffer, float peak) {
      float max_value = 0.0f;
      for (float sample : buffer)
        max_value = std::max(max_value, std::abs(sample));
      if (max_value < 1e-9f)
        return;
      float scale = peak / max_value;
      for (float& sample : buffer)
        sample *= scale;
    }

    void smooth(std::vector<float>& buffer, int passes) {
      int n = static_cast<int>(buffer.size());
      std::vector<float> temp(n);
      for (int p = 0; p < passes; ++p) {
        for (int i = 0; i < n; ++i)
          temp[i] = 0.25f * buffer[(i - 1 + n) % n] + 0.5f * buffer[i] + 0.25f * buffer[(i + 1) % n];
        buffer = temp;
      }
    }

    void makeLoop(std::vector<float>& buffer, double fraction) {
      int n = static_cast<int>(buffer.size());
      int length = std::max(2, static_cast<int>(n * fraction));
      for (int i = 0; i < length; ++i) {
        double t = static_cast<double>(i) / length;
        double weight = 0.5 - 0.5 * std::cos(kPi * t);
        float head = buffer[i];
        float tail = buffer[n - length + i];
        buffer[i] = static_cast<float>(head * weight + tail * (1.0 - weight));
      }
    }

    std::vector<double> randomSpectrum(Random& random, int harmonics, double tilt, double sparsity) {
      std::vector<double> amps(harmonics, 0.0);
      for (int n = 1; n <= harmonics; ++n) {
        if (random.nextDouble() < sparsity && n > 1)
          continue;
        double r = std::pow(random.nextDouble(), 1.5);
        amps[n - 1] = r / std::pow(n, tilt);
      }
      amps[0] = std::max(amps[0], 0.3);
      return amps;
    }

    std::vector<double> randomPhases(Random& random, int harmonics) {
      std::vector<double> phases(harmonics);
      for (int i = 0; i < harmonics; ++i)
        phases[i] = random.nextDouble() * kTwoPi;
      return phases;
    }

    std::vector<double> lerpArray(const std::vector<double>& a, const std::vector<double>& b, double t) {
      size_t n = std::max(a.size(), b.size());
      std::vector<double> result(n);
      for (size_t i = 0; i < n; ++i) {
        double x = i < a.size() ? a[i] : 0.0;
        double y = i < b.size() ? b[i] : 0.0;
        result[i] = lerp(x, y, t);
      }
      return result;
    }
  }

} // namespace gen
} // namespace corpuscular
