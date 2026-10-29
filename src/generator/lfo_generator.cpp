/* Corpuscular LFO generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "lfo_generator.h"

#include "preset_generator.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <random>

namespace corpuscular {
namespace gen {

  namespace {
    constexpr int kLfoGenMaxPoints = 100;

    // Collects points as (x, output 0..1, curve power to the next point)
    class LfoGenBuilder {
      public:
        void add(double x, double value, double power = 0.0) {
          if ((int)xs_.size() >= kLfoGenMaxPoints)
            return;
          xs_.push_back(std::min(1.0, std::max(0.0, x)));
          values_.push_back(std::min(1.0, std::max(0.0, value)));
          powers_.push_back(power);
        }

        bool full(int extra) const { return (int)xs_.size() + extra > kLfoGenMaxPoints; }

        nlohmann::json toJson(const std::string& name, bool smooth) {
          if (xs_.empty()) {
            add(0.0, 0.0);
            add(1.0, 0.0);
          }
          // Vital wants the shape to start at 0 and end at 1 with rising x
          xs_.front() = 0.0;
          xs_.back() = 1.0;
          for (size_t i = 1; i < xs_.size(); ++i)
            xs_[i] = std::max(xs_[i], xs_[i - 1]);

          nlohmann::json points = nlohmann::json::array();
          nlohmann::json powers = nlohmann::json::array();
          for (size_t i = 0; i < xs_.size(); ++i) {
            points.push_back(xs_[i]);
            points.push_back(1.0 - values_[i]);
            powers.push_back(powers_[i]);
          }

          nlohmann::json data;
          data["name"] = name;
          data["num_points"] = (int)xs_.size();
          data["points"] = points;
          data["powers"] = powers;
          data["smooth"] = smooth;
          return data;
        }

      private:
        std::vector<double> xs_;
        std::vector<double> values_;
        std::vector<double> powers_;
    };

    class LfoGenRandom {
      public:
        explicit LfoGenRandom(uint32_t seed) : engine_(seed) { }
        double range(double low, double high) { return std::uniform_real_distribution<double>(low, high)(engine_); }
        int integer(int low, int high) { return std::uniform_int_distribution<int>(low, high)(engine_); }
        bool chance(double probability) { return range(0.0, 1.0) < probability; }
        template<typename T> T pick(const std::vector<T>& items) { return items[integer(0, (int)items.size() - 1)]; }
        double sign() { return chance(0.5) ? 1.0 : -1.0; }

      private:
        std::mt19937 engine_;
    };

    double lfoGenQuantize(double value, int levels) {
      return std::round(value * levels) / levels;
    }

    bool lfoGenSine(LfoGenBuilder& b, LfoGenRandom& r) {
      int humps = r.pick<int>({ 1, 1, 2, 3, 4 });
      bool start_high = r.chance(0.3);
      for (int i = 0; i <= 2 * humps; ++i)
        b.add(i / (2.0 * humps), (i % 2 == 0) == start_high ? 1.0 : 0.0);
      return true;
    }

    bool lfoGenTriangle(LfoGenBuilder& b, LfoGenRandom& r) {
      int humps = r.pick<int>({ 1, 1, 2, 4 });
      double skew = r.range(0.15, 0.85);
      double curve = r.range(0.0, 4.0) * r.sign();
      for (int i = 0; i < humps; ++i) {
        double x0 = (double)i / humps;
        double width = 1.0 / humps;
        b.add(x0, 0.0, curve);
        b.add(x0 + width * skew, 1.0, -curve);
      }
      b.add(1.0, 0.0);
      return false;
    }

    bool lfoGenSaw(LfoGenBuilder& b, LfoGenRandom& r, bool up) {
      int ramps = r.pick<int>({ 1, 1, 2, 3, 4, 8 });
      double curve = r.range(0.0, 7.0) * r.sign();
      double low = r.chance(0.3) ? r.range(0.0, 0.3) : 0.0;
      for (int i = 0; i < ramps; ++i) {
        double x0 = (double)i / ramps;
        double x1 = (i + 1.0) / ramps;
        b.add(x0, up ? low : 1.0, curve);
        b.add(x1, up ? 1.0 : low);
        if (i + 1 < ramps)
          b.add(x1, up ? low : 1.0, curve);
      }
      return false;
    }

    bool lfoGenPulse(LfoGenBuilder& b, LfoGenRandom& r) {
      int cycles = r.pick<int>({ 1, 2, 4, 8 });
      double width = r.range(0.1, 0.9);
      for (int i = 0; i < cycles; ++i) {
        double x0 = (double)i / cycles;
        double x_off = x0 + width / cycles;
        b.add(x0, 1.0);
        b.add(x_off, 1.0);
        b.add(x_off, 0.0);
        b.add((i + 1.0) / cycles, 0.0);
      }
      return false;
    }

    bool lfoGenCurveRamp(LfoGenBuilder& b, LfoGenRandom& r) {
      double start = r.chance(0.5) ? 0.0 : r.range(0.0, 0.4);
      double end = r.range(0.7, 1.0);
      if (r.chance(0.5))
        std::swap(start, end);
      double curve = r.range(3.0, 12.0) * r.sign();
      if (r.chance(0.4)) {
        double middle_x = r.range(0.3, 0.7);
        double middle = r.range(0.2, 0.8);
        b.add(0.0, start, curve);
        b.add(middle_x, middle, -curve * 0.5);
        b.add(1.0, end);
      }
      else {
        b.add(0.0, start, curve);
        b.add(1.0, end);
      }
      return false;
    }

    bool lfoGenSidechain(LfoGenBuilder& b, LfoGenRandom& r) {
      int beats = r.pick<int>({ 1, 2, 4, 4, 8 });
      double depth = r.range(0.7, 1.0);
      double curve = -r.range(3.0, 9.0);
      double hold = r.chance(0.5) ? r.range(0.0, 0.15) : 0.0;
      for (int i = 0; i < beats; ++i) {
        double x0 = (double)i / beats;
        double x1 = (i + 1.0) / beats;
        double width = x1 - x0;
        if (hold > 0.0) {
          b.add(x0, 1.0 - depth);
          b.add(x0 + width * hold, 1.0 - depth, curve);
        }
        else
          b.add(x0, 1.0 - depth, curve);
        b.add(x1, 1.0);
        if (i + 1 < beats)
          b.add(x1, 1.0 - depth);
      }
      return false;
    }

    bool lfoGenTranceGate(LfoGenBuilder& b, LfoGenRandom& r) {
      int steps = r.pick<int>({ 8, 16, 16 });
      double density = r.range(0.45, 0.8);
      double length = r.range(0.35, 0.9);
      double decay_curve = r.range(0.0, 5.0);
      bool any = false;
      for (int i = 0; i < steps; ++i) {
        double x0 = (double)i / steps;
        double width = 1.0 / steps;
        bool on = r.chance(density) || (i == 0);
        any = any || on;
        if (on && !b.full(4)) {
          double level = r.chance(0.25) ? r.range(0.5, 0.75) : 1.0;
          double x_off = x0 + width * length;
          b.add(x0, level, decay_curve);
          b.add(x_off, level * r.range(0.6, 1.0));
          b.add(x_off, 0.0);
        }
        else if (!b.full(1))
          b.add(x0, 0.0);
      }
      b.add(1.0, 0.0);
      return false;
    }

    bool lfoGenEuclid(LfoGenBuilder& b, LfoGenRandom& r) {
      int steps = r.pick<int>({ 8, 12, 16 });
      int pulses = r.integer(3, steps - 3);
      int rotation = r.integer(0, steps - 1);
      double decay = r.range(0.0, 0.9);
      double curve = r.range(2.0, 8.0);
      for (int i = 0; i < steps; ++i) {
        int k = (i + rotation) % steps;
        bool on = ((k * pulses) % steps) < pulses;
        double x0 = (double)i / steps;
        double x1 = (i + 1.0) / steps;
        if (on && !b.full(3)) {
          b.add(x0, 1.0, curve);
          b.add(x1, 1.0 - decay);
          if (i + 1 < steps)
            b.add(x1, 0.0);
        }
        else if (!b.full(1))
          b.add(x0, 0.0);
      }
      b.add(1.0, 0.0);
      return false;
    }

    bool lfoGenRandomSteps(LfoGenBuilder& b, LfoGenRandom& r) {
      int steps = r.pick<int>({ 4, 8, 8, 16, 16, 32 });
      int levels = r.pick<int>({ 4, 8, 12, 16 });
      for (int i = 0; i < steps && !b.full(2); ++i) {
        double level = lfoGenQuantize(r.range(0.0, 1.0), levels);
        b.add((double)i / steps, level);
        b.add((i + 1.0) / steps, level);
      }
      return false;
    }

    bool lfoGenStairs(LfoGenBuilder& b, LfoGenRandom& r) {
      int steps = r.pick<int>({ 4, 6, 8, 12, 16 });
      int direction = r.integer(0, 2);  // up, down, up-down
      for (int i = 0; i < steps && !b.full(2); ++i) {
        double t = steps > 1 ? (double)i / (steps - 1) : 0.0;
        double level = direction == 0 ? t : direction == 1 ? 1.0 - t : 1.0 - std::abs(2.0 * t - 1.0);
        b.add((double)i / steps, level);
        b.add((i + 1.0) / steps, level);
      }
      return false;
    }

    bool lfoGenSmoothRandom(LfoGenBuilder& b, LfoGenRandom& r) {
      int points = r.integer(4, 12);
      double first = r.range(0.0, 1.0);
      b.add(0.0, first);
      for (int i = 1; i < points; ++i)
        b.add((double)i / points, r.range(0.0, 1.0));
      b.add(1.0, first);
      return true;
    }

    bool lfoGenBounce(LfoGenBuilder& b, LfoGenRandom& r) {
      int bounces = r.integer(3, 7);
      double loss = r.range(0.45, 0.75);
      double curve = r.range(2.0, 6.0);
      std::vector<double> widths;
      double total = 0.0;
      double width = 1.0;
      for (int i = 0; i < bounces; ++i) {
        widths.push_back(width);
        total += width;
        width *= std::sqrt(loss);
      }
      double x = 0.0;
      double height = 1.0;
      b.add(0.0, 0.0, -curve);
      for (int i = 0; i < bounces; ++i) {
        double w = widths[i] / total;
        b.add(x + w * 0.5, height, curve);
        x += w;
        b.add(x, 0.0, -curve);
        height *= loss;
      }
      return false;
    }

    bool lfoGenWobble(LfoGenBuilder& b, LfoGenRandom& r) {
      int humps = r.pick<int>({ 2, 3, 4, 6, 8 });
      double floor = r.range(0.0, 0.3);
      b.add(0.0, floor);
      for (int i = 0; i < humps; ++i) {
        double x0 = (double)i / humps;
        double width = 1.0 / humps;
        b.add(x0 + width * 0.5, r.range(0.4, 1.0));
        b.add(x0 + width, i + 1 < humps ? r.range(floor, floor + 0.3) : floor);
      }
      return true;
    }

    bool lfoGenChaos(LfoGenBuilder& b, LfoGenRandom& r) {
      int points = r.integer(8, 24);
      std::vector<double> xs;
      for (int i = 0; i < points - 2; ++i)
        xs.push_back(r.range(0.02, 0.98));
      std::sort(xs.begin(), xs.end());
      b.add(0.0, r.range(0.0, 1.0), r.range(-6.0, 6.0));
      for (double x : xs)
        b.add(x, r.range(0.0, 1.0), r.chance(0.5) ? r.range(-8.0, 8.0) : 0.0);
      b.add(1.0, r.range(0.0, 1.0));
      return false;
    }

    bool lfoGenAccents(LfoGenBuilder& b, LfoGenRandom& r) {
      int steps = r.pick<int>({ 8, 16, 16 });
      double curve = r.range(2.0, 7.0);
      std::vector<double> accents = { 1.0, 0.7, 0.45 };
      for (int i = 0; i < steps && !b.full(3); ++i) {
        double level = (i % 4 == 0) ? 1.0 : r.pick<double>(accents);
        double x0 = (double)i / steps;
        double x1 = (i + 1.0) / steps;
        b.add(x0, level, curve);
        b.add(x1, level * r.range(0.05, 0.4));
        if (i + 1 < steps)
          b.add(x1, 0.0);
      }
      return false;
    }

    struct LfoGenType {
      const char* name;
      std::function<bool(LfoGenBuilder&, LfoGenRandom&)> make;  // returns whether the shape is smooth
    };

    const std::vector<LfoGenType>& lfoGenTypes() {
      static const std::vector<LfoGenType> types = {
        { "Sine", lfoGenSine },
        { "Triangle", lfoGenTriangle },
        { "Saw Up", [](LfoGenBuilder& b, LfoGenRandom& r) { return lfoGenSaw(b, r, true); } },
        { "Saw Down", [](LfoGenBuilder& b, LfoGenRandom& r) { return lfoGenSaw(b, r, false); } },
        { "Pulse", lfoGenPulse },
        { "Curve Ramp", lfoGenCurveRamp },
        { "Sidechain", lfoGenSidechain },
        { "Trance Gate", lfoGenTranceGate },
        { "Euclid Gate", lfoGenEuclid },
        { "Random Steps", lfoGenRandomSteps },
        { "Stairs", lfoGenStairs },
        { "Smooth Random", lfoGenSmoothRandom },
        { "Bounce", lfoGenBounce },
        { "Wobble", lfoGenWobble },
        { "Chaos", lfoGenChaos },
        { "Accents", lfoGenAccents },
      };
      return types;
    }
  }

  std::vector<std::string> lfoTypes() {
    std::vector<std::string> names;
    for (const LfoGenType& type : lfoGenTypes())
      names.push_back(type.name);
    return names;
  }

  GeneratedLfo createLfo(const std::string& type, uint32_t seed) {
    const std::vector<LfoGenType>& types = lfoGenTypes();
    LfoGenRandom random(seed);
    const LfoGenType* chosen = &types[random.integer(0, (int)types.size() - 1)];
    for (const LfoGenType& candidate : types) {
      if (type == candidate.name)
        chosen = &candidate;
    }

    LfoGenBuilder builder;
    bool smooth = chosen->make(builder, random);

    GeneratedLfo lfo;
    lfo.type = chosen->name;
    lfo.name = std::string(chosen->name) + " " + loremWords(seed, 1);
    lfo.data = builder.toJson(lfo.name, smooth);
    return lfo;
  }

} // namespace gen
} // namespace corpuscular
