/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_lfo_shape.h"

#include <algorithm>
#include <stdexcept>

namespace corpuscular {
namespace gen {

  static double lfoShapeClamp01(double value) {
    return value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
  }

  void LfoShape::add(double x, double value) {
    if (numPoints() >= kMaxPoints)
      throw std::runtime_error("LFO has more than 100 points");
    points.push_back(lfoShapeClamp01(x));
    points.push_back(1.0 - lfoShapeClamp01(value));
    powers.push_back(0.0);
  }

  LfoShape LfoShape::triangle() {
    LfoShape shape;
    shape.name = "Triangle";
    shape.add(0.0, 0.0);
    shape.add(0.5, 1.0);
    shape.add(1.0, 0.0);
    return shape;
  }

  LfoShape LfoShape::sine() {
    LfoShape shape;
    shape.name = "Sin";
    shape.smooth = true;
    shape.add(0.0, 0.0);
    shape.add(0.5, 1.0);
    shape.add(1.0, 0.0);
    return shape;
  }

  LfoShape LfoShape::sawDown() {
    LfoShape shape;
    shape.name = "Saw Down";
    shape.add(0.0, 1.0);
    shape.add(1.0, 0.0);
    return shape;
  }

  LfoShape LfoShape::steps(const std::string& name, const std::vector<double>& levels) {
    LfoShape shape;
    shape.name = name;
    int n = static_cast<int>(levels.size());
    for (int i = 0; i < n; ++i) {
      shape.add(static_cast<double>(i) / n, levels[i]);
      shape.add(static_cast<double>(i + 1) / n, levels[i]);
    }
    return shape;
  }

  LfoShape LfoShape::gate(const std::string& name, const std::vector<double>& levels,
                          const std::vector<double>& lengths, double decay) {
    LfoShape shape;
    shape.name = name;
    int n = static_cast<int>(levels.size());
    for (int i = 0; i < n; ++i) {
      double x0 = static_cast<double>(i) / n;
      double width = 1.0 / n;
      double length = std::max(0.15, std::min(1.0, lengths[i]));
      double top = levels[i];
      if (length >= 0.999) {
        shape.add(x0, top);
        shape.add(x0 + width, top * (1.0 - decay * 0.5));
      }
      else {
        double x_end = x0 + width * length;
        shape.add(x0, top);
        shape.add(x_end, top * (1.0 - decay));
        shape.add(x_end, 0.0);
        shape.add(x0 + width, 0.0);
      }
    }
    return shape;
  }

} // namespace gen
} // namespace corpuscular
