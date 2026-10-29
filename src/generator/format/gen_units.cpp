/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_units.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace corpuscular {
namespace gen {

  namespace ParamUnits {
    static const char* const kParamUnitsTempoNames[] = {
      "Freeze", "32/1", "16/1", "8/1", "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"
    };

    double envSeconds(double seconds) { return std::pow(std::max(0.0, seconds), 0.25); }

    double hz(double hz) { return 69.0 + 12.0 * std::log2(hz / 440.0); }

    double lfoHz(double hz) { return std::log2(hz); }

    double volumeDb(double db) { return (db + 80.0) * (db + 80.0); }

    double amplitude(double amp) { return std::sqrt(std::max(0.0, amp)); }

    int tempo(const std::string& name) {
      int count = static_cast<int>(sizeof(kParamUnitsTempoNames) / sizeof(kParamUnitsTempoNames[0]));
      for (int i = 0; i < count; ++i) {
        if (name == kParamUnitsTempoNames[i])
          return i;
      }
      throw std::invalid_argument("No tempo " + name);
    }

    double effectOrder(const std::vector<int>& order) {
      if (order.size() != Fx::kCount)
        throw std::invalid_argument("All 9 effects are needed");

      long long code = 0;
      for (int i = 1; i < static_cast<int>(order.size()); ++i) {
        int index = 0;
        for (int j = 0; j < i; ++j) {
          if (order[i] < order[j])
            index++;
        }
        code *= i + 1;
        code += index;
      }
      return static_cast<double>(code);
    }

    std::vector<int> decodeEffectOrder(double value) {
      int size = Fx::kCount;
      std::vector<int> order(size);
      int code = static_cast<int>(value);
      for (int i = 0; i < size; ++i)
        order[i] = i;

      for (int i = 0; i < size; ++i) {
        int remaining = size - i;
        int index = remaining - 1;
        int inversions = code % remaining;
        code /= remaining;
        int placement = order[index - inversions];
        for (int j = index - inversions; j < index; ++j)
          order[j] = order[j + 1];
        order[index] = placement;
      }
      return order;
    }
  }

} // namespace gen
} // namespace corpuscular
