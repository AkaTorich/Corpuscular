/* Corpuscular preset mutator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "preset_mutator.h"

#include "format/gen_parameters.h"

#include <algorithm>
#include <random>
#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  namespace {
    // Knobs that change pitch, tempo or loudness of the whole patch stay as they are
    bool mutatorKeepsParameter(const std::string& name) {
      static const char* kKept[] = { "transpose", "tune", "beats_per_minute", "volume", "pitch_bend",
                                     "pitch_wheel", "mod_wheel", "polyphony", "velocity_track", "voice_" };
      for (const char* kept : kKept) {
        if (name.find(kept) != std::string::npos)
          return true;
      }
      return false;
    }

    // Groups of neighbouring points with one level are the steps of a stepped LFO
    void mutatorShuffleSteps(nlohmann::json& lfo, std::mt19937& engine) {
      if (!lfo.is_object() || !lfo.count("points") || !lfo["points"].is_array())
        return;
      nlohmann::json& points = lfo["points"];
      int num_points = (int)points.size() / 2;

      std::vector<std::vector<int>> groups;
      for (int i = 0; i < num_points; ++i) {
        double y = points[2 * i + 1].get<double>();
        if (!groups.empty() && points[2 * groups.back().front() + 1].get<double>() == y)
          groups.back().push_back(i);
        else
          groups.push_back({ i });
      }

      int steps = 0;
      for (const std::vector<int>& group : groups)
        steps += group.size() >= 2 ? 1 : 0;
      if (steps < 4)
        return;

      std::vector<int> stepped;
      for (int g = 0; g < (int)groups.size(); ++g) {
        if (groups[g].size() >= 2)
          stepped.push_back(g);
      }

      std::uniform_int_distribution<int> pick(0, (int)stepped.size() - 1);
      int swaps = std::uniform_int_distribution<int>(1, 3)(engine);
      for (int s = 0; s < swaps; ++s) {
        const std::vector<int>& a = groups[stepped[pick(engine)]];
        const std::vector<int>& b = groups[stepped[pick(engine)]];
        double level_a = points[2 * a.front() + 1].get<double>();
        double level_b = points[2 * b.front() + 1].get<double>();
        for (int i : a)
          points[2 * i + 1] = level_b;
        for (int i : b)
          points[2 * i + 1] = level_a;
      }
    }
  }

  nlohmann::json mutatePreset(const nlohmann::json& preset, uint32_t seed, double amount) {
    nlohmann::json result = preset;
    if (!result.is_object() || !result.count("settings") || !result["settings"].is_object())
      return result;

    std::mt19937 engine(seed);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::normal_distribution<double> nudge(0.0, 1.0);
    nlohmann::json& settings = result["settings"];

    for (auto it = settings.begin(); it != settings.end(); ++it) {
      const std::string& name = it.key();
      if (!it.value().is_number())
        continue;

      double value = it.value().get<double>();
      bool modulation_amount = name.rfind("modulation_", 0) == 0 &&
                               name.size() > 7 && name.compare(name.size() - 7, 7, "_amount") == 0;
      if (modulation_amount) {
        if (value != 0.0 && unit(engine) < 0.5)
          it.value() = std::max(-1.0, std::min(1.0, value + nudge(engine) * amount));
        continue;
      }

      if (!Parameters::exists(name) || mutatorKeepsParameter(name))
        continue;
      const ParamSpec& spec = Parameters::get(name);
      if (spec.indexed || spec.range() <= 0.0 || unit(engine) >= 0.5)
        continue;

      it.value() = spec.clamp(value + nudge(engine) * amount * spec.range());
    }

    if (settings.count("lfos") && settings["lfos"].is_array()) {
      for (nlohmann::json& lfo : settings["lfos"]) {
        if (unit(engine) < 0.7)
          mutatorShuffleSteps(lfo, engine);
      }
    }
    return result;
  }

} // namespace gen
} // namespace corpuscular
