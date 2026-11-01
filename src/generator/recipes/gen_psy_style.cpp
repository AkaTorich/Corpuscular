/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_psy_style.h"

#include "gen_kit.h"
#include "gen_recipe.h"
#include "../format/gen_patch.h"

#include <vector>

namespace corpuscular {
namespace gen {

  namespace PsyStyle {
    double filterBlend(RecipeContext& c, double band_pass, double high_pass) {
      double roll = c.r.nextDouble();
      if (roll < high_pass)
        return c.range(1.4, 1.8);
      if (roll < high_pass + band_pass)
        return c.range(0.8, 1.2);
      return c.range(0.0, 0.15);
    }

    int filterModel(RecipeContext& c) {
      // Packs: analog 54%, digital 18%, dirty 6%, ladder 5% (comb and phaser models left out: they ring and thin out)
      double roll = c.r.nextDouble();
      if (roll < 0.55)
        return FilterModel::kAnalog;
      if (roll < 0.73)
        return FilterModel::kDigital;
      if (roll < 0.83)
        return FilterModel::kDirty;
      if (roll < 0.93)
        return FilterModel::kLadder;
      return FilterModel::kDiode;
    }

    void oscWarp(RecipeContext& c, int osc, double chance) {
      if (!c.chance(chance))
        return;
      std::string o = "osc_" + std::to_string(osc) + "_";
      c.p[o + "distortion_type"] = c.pick({ OscDistortion::kSync, OscDistortion::kSync, OscDistortion::kFormant,
                                            OscDistortion::kFormant, OscDistortion::kBend, OscDistortion::kSqueeze,
                                            OscDistortion::kPulseWidth });
      c.p[o + "distortion_amount"] = c.range(0.3, 0.7);
    }

    void oscFm(RecipeContext& c, int osc, double chance) {
      // FM sources of an oscillator: A is the first of the other two, B the second
      std::vector<int> others;
      for (int other = 1; other <= 3; ++other) {
        if (other != osc)
          others.push_back(other);
      }
      std::vector<int> types;
      if (c.p.get("osc_" + std::to_string(others[0]) + "_on") > 0.0)
        types.push_back(OscDistortion::kFmOscA);
      if (c.p.get("osc_" + std::to_string(others[1]) + "_on") > 0.0)
        types.push_back(OscDistortion::kFmOscB);
      if (types.empty() || !c.chance(chance))
        return;
      std::string o = "osc_" + std::to_string(osc) + "_";
      c.p[o + "distortion_type"] = types[c.r.next(static_cast<int>(types.size()))];
      c.p[o + "distortion_amount"] = c.range(0.15, 0.45);
    }

    void spectralMorph(RecipeContext& c, int osc, double chance) {
      if (!c.chance(chance))
        return;
      std::string o = "osc_" + std::to_string(osc) + "_";
      c.p[o + "spectral_morph_type"] = c.pick({ SpectralMorph::kPhaseDisperse, SpectralMorph::kFormantScale,
                                                SpectralMorph::kSmear, SpectralMorph::kHarmonicStretch,
                                                SpectralMorph::kInharmonicStretch, SpectralMorph::kRandomAmplitudes,
                                                SpectralMorph::kSkew });
      c.p[o + "spectral_morph_amount"] = c.range(0.3, 0.7);
    }

    int distortionType(RecipeContext& c) {
      // Packs: soft clip 64%, hard clip 14%, downsample 9%, sine fold 7%, linear fold 6%
      double roll = c.r.nextDouble();
      if (roll < 0.62)
        return DistortionType::kSoftClip;
      if (roll < 0.78)
        return DistortionType::kHardClip;
      if (roll < 0.86)
        return DistortionType::kDownSample;
      if (roll < 0.93)
        return DistortionType::kSineFold;
      return DistortionType::kLinearFold;
    }

    namespace {
      // One macro target: name on the knob, destination, how often the producers use it for the type (%),
      // the amount range around their median, and the parameter that has to be on for it to mean something
      struct MacroTarget {
        const char* name;
        const char* destination;
        double weight;
        double low;
        double high;
        const char* needs;
      };

      // From the producers' top modulations per type (tools/preset_stats)
      std::vector<MacroTarget> macroTargets(const std::string& type) {
        if (type == "Lead")
          // Outerverse and Sonicspore leads also put a knob on both wave positions (59% / 41%) and open
          // the flanger, the phaser and the warp nearly all the way
          return { { "DELAY", "delay_dry_wet", 70, 0.35, 0.55, "delay_on" },
                   { "WAVE", "osc_1_wave_frame", 45, 0.5, 0.9, "osc_1_on" },
                   { "FILTER", "filter_1_cutoff", 42, 0.45, 0.7, "filter_1_on" },
                   { "WARP", "osc_1_distortion_amount", 38, 0.3, 0.7, "osc_1_distortion_type" },
                   { "SPACE", "reverb_dry_wet", 36, 0.3, 0.55, "reverb_on" },
                   { "WAVE 2", "osc_2_wave_frame", 30, 0.5, 0.9, "osc_2_on" },
                   { "FLANGER", "flanger_dry_wet", 30, 0.6, 1.0, "flanger_on" },
                   { "PHASER", "phaser_dry_wet", 25, 0.6, 1.0, "phaser_on" },
                   { "MORPH", "osc_1_spectral_morph_amount", 24, 0.25, 0.5, "osc_1_spectral_morph_type" },
                   { "CHORUS", "chorus_dry_wet", 22, 0.4, 0.8, "chorus_on" },
                   { "RESO", "filter_1_resonance", 13, 0.2, 0.35, "filter_1_on" } };
        if (type == "Sequence")
          return { { "DELAY", "delay_dry_wet", 50, 0.4, 0.6, "delay_on" },
                   { "FLANGER", "flanger_dry_wet", 50, 0.6, 1.0, "flanger_on" },
                   { "WARP", "osc_1_distortion_amount", 45, 0.5, 0.9, "osc_1_distortion_type" },
                   { "FILTER", "filter_1_cutoff", 32, 0.45, 0.7, "filter_1_on" },
                   { "SPACE", "reverb_dry_wet", 27, 0.35, 0.6, "reverb_on" },
                   { "FEEDBACK", "delay_feedback", 22, 0.15, 0.3, "delay_on" },
                   { "MORPH", "osc_1_spectral_morph_amount", 22, 0.3, 0.7, "osc_1_spectral_morph_type" },
                   { "PHASER", "phaser_dry_wet", 20, 0.5, 0.8, "phaser_on" },
                   { "CHORUS", "chorus_dry_wet", 16, 0.3, 0.5, "chorus_on" } };
        if (type == "Arp")
          return { { "DELAY", "delay_dry_wet", 76, 0.2, 0.4, "delay_on" },
                   { "PHASER", "phaser_dry_wet", 44, 0.5, 0.8, "phaser_on" },
                   { "DRIVE", "distortion_mix", 44, 0.4, 0.7, "distortion_on" },
                   { "SPACE", "reverb_dry_wet", 32, 0.3, 0.5, "reverb_on" },
                   { "FILTER", "filter_1_cutoff", 30, 0.35, 0.6, "filter_1_on" },
                   { "WARP", "osc_1_distortion_amount", 20, 0.25, 0.5, "osc_1_distortion_type" },
                   { "CHORUS", "chorus_dry_wet", 20, 0.3, 0.5, "chorus_on" } };
        if (type == "Pad")
          return { { "SPACE", "reverb_dry_wet", 48, 0.3, 0.55, "reverb_on" },
                   { "DELAY", "delay_dry_wet", 48, 0.25, 0.45, "delay_on" },
                   { "WARP", "osc_1_distortion_amount", 45, 0.4, 0.8, "osc_1_distortion_type" },
                   { "FILTER", "filter_1_cutoff", 30, 0.3, 0.6, "filter_1_on" },
                   { "MORPH", "osc_1_spectral_morph_amount", 30, 0.2, 0.5, "osc_1_spectral_morph_type" },
                   { "CHORUS", "chorus_dry_wet", 26, 0.4, 0.7, "chorus_on" },
                   { "WAVE", "osc_1_wave_frame", 20, 0.4, 0.8, "osc_1_on" } };
        if (type == "Keys")
          return { { "DELAY", "delay_dry_wet", 55, 0.2, 0.35, "delay_on" },
                   { "SPACE", "reverb_dry_wet", 42, 0.2, 0.35, "reverb_on" },
                   { "FILTER", "filter_1_cutoff", 36, 0.25, 0.45, "filter_1_on" },
                   { "WARP", "osc_1_distortion_amount", 30, 0.25, 0.45, "osc_1_distortion_type" },
                   { "CHORUS", "chorus_dry_wet", 27, 0.3, 0.5, "chorus_on" },
                   { "WAVE", "osc_1_wave_frame", 15, 0.3, 0.6, "osc_1_on" } };
        if (type == "Stab")
          return { { "DELAY", "delay_dry_wet", 100, 0.35, 0.5, "delay_on" },
                   { "SPACE", "reverb_dry_wet", 60, 0.4, 0.6, "reverb_on" },
                   { "FILTER", "filter_1_cutoff", 40, 0.5, 0.8, "filter_1_on" },
                   { "FLANGER", "flanger_dry_wet", 40, 0.5, 0.8, "flanger_on" },
                   { "CHORUS", "chorus_dry_wet", 20, 0.3, 0.5, "chorus_on" } };
        if (type == "Pluck")
          return { { "DELAY", "delay_dry_wet", 41, 0.35, 0.55, "delay_on" },
                   { "SPACE", "reverb_dry_wet", 35, 0.3, 0.45, "reverb_on" },
                   { "TAIL", "reverb_decay_time", 35, 0.4, 0.6, "reverb_on" },
                   { "FILTER", "filter_1_cutoff", 29, 0.5, 0.8, "filter_1_on" },
                   { "WARP", "osc_1_distortion_amount", 29, 0.25, 0.4, "osc_1_distortion_type" },
                   { "WIDTH", "osc_1_unison_detune", 15, 0.2, 0.35, "osc_1_on" } };
        if (type == "Bass")
          return { { "DECAY", "env_2_decay", 52, 0.06, 0.12, "filter_1_on" },
                   { "PUNCH", "env_2_sustain", 49, 0.15, 0.35, "filter_1_on" },
                   { "FILTER", "filter_1_cutoff", 17, 0.15, 0.35, "filter_1_on" },
                   { "WARP", "osc_1_distortion_amount", 18, 0.1, 0.3, "osc_1_distortion_type" },
                   { "SUB", "osc_2_level", 11, 0.2, 0.3, "osc_2_on" },
                   { "DRIVE", "distortion_mix", 10, 0.2, 0.4, "distortion_on" },
                   { "WAVE", "osc_1_wave_frame", 10, 0.3, 0.6, "osc_1_on" } };
        // SFX
        return { { "DELAY", "delay_dry_wet", 62, 0.3, 0.5, "delay_on" },
                 { "SPACE", "reverb_dry_wet", 40, 0.35, 0.6, "reverb_on" },
                 { "FILTER", "filter_1_cutoff", 35, 0.35, 0.6, "filter_1_on" },
                 { "WARP", "osc_1_distortion_amount", 31, 0.3, 0.6, "osc_1_distortion_type" },
                 { "CHORUS", "chorus_dry_wet", 25, 0.4, 0.7, "chorus_on" },
                 { "FLANGER", "flanger_dry_wet", 20, 0.5, 0.8, "flanger_on" },
                 { "PHASER", "phaser_dry_wet", 20, 0.5, 0.8, "phaser_on" } };
      }
    }

    void producerMacros(RecipeContext& c, const std::string& type) {
      std::vector<MacroTarget> targets;
      for (const MacroTarget& target : macroTargets(type)) {
        if (c.p.get(target.needs) > 0.0)
          targets.push_back(target);
      }

      // Four distinct knobs, the common ones first more often (weighted draw without repeats)
      for (int knob = 1; knob <= 4 && !targets.empty(); ++knob) {
        double total = 0.0;
        for (const MacroTarget& target : targets)
          total += target.weight;
        double roll = c.r.nextDouble() * total;
        size_t chosen = 0;
        for (; chosen + 1 < targets.size(); ++chosen) {
          roll -= targets[chosen].weight;
          if (roll < 0.0)
            break;
        }
        const MacroTarget& target = targets[chosen];
        Kit::macro(c, knob, target.name, target.destination, c.range(target.low, target.high));
        targets.erase(targets.begin() + chosen);
      }
    }

    void randomCutoff(RecipeContext& c, int random_n, int filter, double chance) {
      if (!c.chance(chance))
        return;
      Kit::random(c, random_n, "filter_" + std::to_string(filter) + "_cutoff", c.range(0.2, 0.45),
                  c.pick({ "1/16", "1/16", "1/8" }), 1);
    }
  }

} // namespace gen
} // namespace corpuscular
