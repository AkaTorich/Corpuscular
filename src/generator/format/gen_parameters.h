/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>

namespace corpuscular {
namespace gen {

  struct ParamSpec {
    const char* name;
    double min;
    double max;
    double default_value;
    bool indexed;

    double range() const { return max - min; }
    double clamp(double value) const;
  };

  // Parameter set and ranges of vital 1.0.7; the table itself is in gen_parameters_table.cpp
  class Parameters {
    public:
      static constexpr const char* kSynthVersion = "1.0.7";
      static constexpr int kModulationSlots = 64;
      static constexpr int kLfoCount = 8;
      static constexpr int kOscCount = 3;

      static int count() { return kTableSize; }
      static const ParamSpec& at(int index) { return kTable[index]; }

      static bool exists(const std::string& name);
      static const ParamSpec& get(const std::string& name);

    private:
      static const ParamSpec kTable[];
      static const int kTableSize;
  };

} // namespace gen
} // namespace corpuscular
