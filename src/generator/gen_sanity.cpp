/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_sanity.h"

#include "format/gen_patch.h"
#include "format/gen_units.h"

#include <algorithm>
#include <cmath>
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

    // The note a type is usually played on (MIDI), where the producers' presets of that type sit
    static int sanityTypicalNote(const std::string& type) {
      if (type == "Bass")
        return 36;
      if (type == "Sequence")
        return 45;
      if (type == "Pad" || type == "Keys" || type == "Stab" || type == "Pluck")
        return 60;
      return 57;
    }

    static double sanitySeconds(const Patch& patch, const std::string& key) {
      return std::pow(std::max(0.0, patch.get(key)), 4.0);
    }

    static void sanityMaxSeconds(Patch& patch, const std::string& key, double seconds) {
      if (sanitySeconds(patch, key) > seconds)
        patch[key] = ParamUnits::envSeconds(seconds);
    }

    static void sanityMinSeconds(Patch& patch, const std::string& key, double seconds) {
      if (sanitySeconds(patch, key) < seconds)
        patch[key] = ParamUnits::envSeconds(seconds);
    }

    static void sanityLimit(Patch& patch, const std::string& key, double low, double high) {
      patch[key] = std::max(low, std::min(high, patch.get(key)));
    }

    // The sound keeps the shape that makes it its type, with the bounds of the producers' presets
    // (tools/preset_stats, type fingerprints): like EnsureAudible in the Virus TI2 generator, it fixes, never drops
    static void typeShape(Patch& p, const std::string& type) {
      if (type == "Lead") {
        // Starts at once, holds while the key is down, stops cleanly, little room
        sanityMaxSeconds(p, "env_1_attack", 0.01);
        sanityLimit(p, "env_1_sustain", 0.8, 1.0);
        sanityMaxSeconds(p, "env_1_release", 0.12);
        sanityLimit(p, "reverb_dry_wet", 0.0, 0.22);
      }
      else if (type == "Bass") {
        // Tight and dry: no attack, a short release, no reverb tail under the kick
        sanityMaxSeconds(p, "env_1_attack", 0.003);
        sanityMaxSeconds(p, "env_1_release", 0.1);
        sanityLimit(p, "reverb_dry_wet", 0.0, 0.06);
        sanityLimit(p, "delay_dry_wet", 0.0, 0.1);
      }
      else if (type == "Sequence" || type == "Arp") {
        // The pattern does the shaping: the envelope stays open and short-released
        sanityMaxSeconds(p, "env_1_attack", 0.003);
        sanityLimit(p, "env_1_sustain", 0.95, 1.0);
        sanityMaxSeconds(p, "env_1_release", 0.09);
        sanityLimit(p, "reverb_dry_wet", 0.0, 0.25);
      }
      else if (type == "Pluck") {
        // Dies away by itself with the key held
        sanityMaxSeconds(p, "env_1_attack", 0.008);
        sanityLimit(p, "env_1_sustain", 0.0, 0.45);
        sanityMaxSeconds(p, "env_1_decay", 0.9);
        sanityMaxSeconds(p, "env_1_release", 0.55);
        sanityLimit(p, "reverb_dry_wet", 0.0, 0.41);
      }
      else if (type == "Stab") {
        sanityMaxSeconds(p, "env_1_attack", 0.003);
        sanityLimit(p, "env_1_sustain", 0.0, 0.35);
        sanityMaxSeconds(p, "env_1_release", 0.4);
      }
      else if (type == "Keys") {
        sanityMaxSeconds(p, "env_1_attack", 0.006);
        sanityLimit(p, "env_1_sustain", 0.0, 0.65);
        sanityLimit(p, "reverb_dry_wet", 0.0, 0.4);
      }
      else if (type == "Pad") {
        // Swells in and fades out slowly, sits in a room
        sanityMinSeconds(p, "env_1_attack", 0.3);
        sanityLimit(p, "env_1_sustain", 0.75, 1.0);
        sanityMinSeconds(p, "env_1_release", 0.8);
        if (p.get("reverb_on") > 0.5)
          sanityLimit(p, "reverb_dry_wet", 0.2, 0.9);
      }
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

      // The low-pass filter is not shut completely, and never below the note the sound plays: the type's usual
      // note plus osc 1's transpose, half an octave higher for the steep 24 dB slope (a pad at C4 with the cutoff
      // an octave under it came out 30 dB down)
      int fundamental = sanityTypicalNote(type) + static_cast<int>(p.get("osc_1_transpose"));
      for (int f = 1; f <= 2; ++f) {
        std::string n = "filter_" + std::to_string(f) + "_";
        int model = static_cast<int>(p.get(n + "model"));
        if (p.get(n + "on") < 0.5 || model == FilterModel::kFormant || model == FilterModel::kComb ||
            model == FilterModel::kPhaser || p.get(n + "blend") > 0.6)
          continue;
        double up = sanityPositiveModulation(p, n + "cutoff") * 128.0;
        if (p.get(n + "cutoff") + up < 52.0)
          p[n + "cutoff"] = 52.0 - up;
        double lowest = fundamental + (p.get(n + "style") == FilterStyle::kDb24 ? 6.0 : 0.0);
        if (p.get(n + "cutoff") + 0.5 * up < lowest)
          p[n + "cutoff"] = lowest - 0.5 * up;
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

      typeShape(p, type);

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
