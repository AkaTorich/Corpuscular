/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_sanity.h"

#include "format/gen_patch.h"
#include "format/gen_units.h"

#include <algorithm>
#include <stdexcept>

namespace corpuscular {
namespace gen {

  namespace Sanity {
    // Sum of the positive modulation amounts into one destination
    static double sanityPositiveModulation(const Patch& patch, const std::string& destination) {
      double sum = 0.0;
      for (const ModConnection& connection : patch.mods.connections()) {
        if (connection.destination == destination)
          sum += std::max(0.0, connection.amount);
      }
      return sum;
    }

    static bool sanityHasModulation(const Patch& patch, const std::string& destination) {
      for (const ModConnection& connection : patch.mods.connections()) {
        if (connection.destination == destination)
          return true;
      }
      return false;
    }

    void apply(Patch& p, const std::string& type) {
      // At least one source is heard (level, or level modulation from the gate)
      bool audible = false;
      for (int o = 1; o <= Parameters::kOscCount; ++o) {
        std::string prefix = "osc_" + std::to_string(o) + "_";
        if (p.get(prefix + "on") < 0.5)
          continue;
        std::string level = prefix + "level";
        if (p.get(level) + sanityPositiveModulation(p, level) > 0.25)
          audible = true;
      }
      if (p.get("sample_on") > 0.5 && p.get("sample_level") > 0.25)
        audible = true;
      if (!audible) {
        p["osc_1_on"] = 1;
        if (p.get("osc_1_level") < 0.5 && !sanityHasModulation(p, "osc_1_level"))
          p["osc_1_level"] = 0.7;
      }

      // The low-pass filter is not shut completely
      for (int f = 1; f <= 2; ++f) {
        std::string n = "filter_" + std::to_string(f) + "_";
        if (p.get(n + "on") < 0.5 || p.get(n + "model") == FilterModel::kFormant || p.get(n + "blend") > 0.6)
          continue;
        double up = sanityPositiveModulation(p, n + "cutoff") * 128.0;
        if (p.get(n + "cutoff") + up < 52.0)
          p[n + "cutoff"] = 52.0 - up;
        if (p.get(n + "cutoff") < 24.0)
          p[n + "cutoff"] = 24.0;
      }

      // The amp envelope is not silent
      if (p.get("env_1_sustain") < 0.05 && p.get("env_1_decay") < ParamUnits::envSeconds(0.06))
        p["env_1_decay"] = ParamUnits::envSeconds(0.08);

      // Space and distortion within reason
      bool wide = type == "Pad" || type == "SFX";
      p["delay_dry_wet"] = std::min(p.get("delay_dry_wet"), wide ? 0.5 : 0.42);
      p["reverb_dry_wet"] = std::min(p.get("reverb_dry_wet"), wide ? 0.9 : 0.45);
      if (p.get("delay_feedback") > 0.8)
        p["delay_feedback"] = 0.8;
      if (p.get("distortion_on") > 0.5 && p.get("distortion_drive") > 12.0)
        p["volume"] = std::min(p.get("volume"), ParamUnits::volumeDb(-9));
      p["volume"] = std::max(ParamUnits::volumeDb(-13), std::min(ParamUnits::volumeDb(-4.5), p.get("volume")));

      // Filter FX stays off without a setting
      if (p.get("filter_fx_on") > 0.5 && p.get("filter_fx_cutoff") < 30.0)
        p["filter_fx_on"] = 0;

      for (const LfoShape& lfo : p.lfos) {
        if (lfo.numPoints() > LfoShape::kMaxPoints || lfo.numPoints() < 2)
          throw std::runtime_error("Invalid LFO shape");
      }
    }
  }

} // namespace gen
} // namespace corpuscular
