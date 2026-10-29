/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <random>

namespace corpuscular {
namespace gen {

  // Seeded random source with the calls the generator recipes use (same ranges as System.Random).
  class Random {
    public:
      explicit Random(uint32_t seed) : engine_(seed) { }

      // [0, 2147483647)
      int next() { return static_cast<int>(nextDouble() * 2147483647.0); }

      // [0, max)
      int next(int max) {
        if (max <= 0)
          return 0;
        return static_cast<int>(nextDouble() * max);
      }

      // [min, max)
      int next(int min, int max) {
        if (min >= max)
          return min;
        int64_t range = static_cast<int64_t>(max) - min;
        return static_cast<int>(min + static_cast<int64_t>(nextDouble() * range));
      }

      // [0, 1) with 53 random bits.
      double nextDouble() {
        uint32_t high = engine_() >> 5;
        uint32_t low = engine_() >> 6;
        return (high * 67108864.0 + low) * (1.0 / 9007199254740992.0);
      }

    private:
      std::mt19937 engine_;
  };

  // Finite check on the bits, safe under /fp:fast.
  inline bool isFiniteValue(double value) {
    uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return ((bits >> 52) & 0x7ff) != 0x7ff;
  }

  // Round half to even, like Math.Round.
  inline double roundHalfEven(double value) {
    double floor_value = std::floor(value);
    double difference = value - floor_value;
    if (difference > 0.5)
      return floor_value + 1.0;
    if (difference < 0.5)
      return floor_value;
    return std::fmod(floor_value, 2.0) == 0.0 ? floor_value : floor_value + 1.0;
  }

} // namespace gen
} // namespace corpuscular
