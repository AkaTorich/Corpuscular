/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  // LFO shape: points (x, y), y = 0 is the top (vital output = 1 - y), a step is two points with one x
  class LfoShape {
    public:
      static constexpr int kMaxPoints = 100;

      int numPoints() const { return static_cast<int>(points.size() / 2); }

      // value is the LFO output 0..1
      void add(double x, double value);

      static LfoShape triangle();
      static LfoShape sine();
      static LfoShape sawDown();

      // Equal steps, levels are the output 0..1 of every step
      static LfoShape steps(const std::string& name, const std::vector<double>& levels);

      // Gate: a step sounds for lengths[i] of its time, then silence; decay is how far the level falls by the end
      static LfoShape gate(const std::string& name, const std::vector<double>& levels,
                           const std::vector<double>& lengths, double decay);

      std::string name = "Triangle";
      bool smooth = false;
      std::vector<double> points;
      std::vector<double> powers;
  };

} // namespace gen
} // namespace corpuscular
