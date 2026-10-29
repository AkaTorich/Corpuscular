/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "../format/gen_patch.h"
#include "../music/gen_sequence_encoder.h"
#include "../wavetables/gen_wavetable_factory.h"

namespace corpuscular {
namespace gen {

  static const std::vector<std::vector<int>>& bassRecipeRollingPatterns() {
    static const std::vector<std::vector<int>> patterns = {
      { 0, 1, 1, 1 }, { 0, 1, 1, 1 }, { 0, 1, 1, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 }
    };
    return patterns;
  }

  void BassRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    bool basic = c.chance(0.5);
    WavetableData table1 = basic ? WavetableFactory::basicShapes()
                                 : Kit::table(c, c.pick({ "Psy Bass", "Psy Bass", "Basic Morph", "PWM" }), c.rangeInt(2, 4));
    Kit::osc(c, 1, std::move(table1), c.range(0.66, 0.76), -12, c.chance(0.75) ? 1 : 2, c.range(0.3, 1.2));
    p["osc_1_wave_frame"] = basic ? c.pick({ 171.0, 190.0, 220.0, 256.0 }) : c.range(0, 90);
    p["osc_1_random_phase"] = 0;

    std::vector<int> oscs = { 1 };
    if (c.chance(0.7)) {
      Kit::osc(c, 2, WavetableFactory::basicShapes(), c.range(0.45, 0.6), c.pick({ -24, -12 }), 1, 0);
      p["osc_2_wave_frame"] = c.pick({ 0.0, 0.0, 85.0 });
      p["osc_2_random_phase"] = 0;
      oscs = { 1, 2 };
    }

    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kDirty, FilterModel::kLadder }),
                c.pick({ FilterStyle::kDb24, FilterStyle::kDb12 }), c.range(32, 52), c.range(0.05, 0.35), c.range(1, 8));
    p["osc_2_destination"] = OscDest::kFilter1;
    Kit::env(c, 1, 0.0005, c.range(0.15, 0.3), c.range(0.6, 0.9), c.range(0.02, 0.05));
    Kit::env(c, 2, 0.0005, c.range(0.06, 0.16), 0, 0.05);
    p.mods.add("env_2", "filter_1_cutoff", c.range(0.22, 0.34));

    p["polyphony"] = 1;
    p["legato"] = 0;
    Kit::distortion(c, c.pick({ DistortionType::kSoftClip, DistortionType::kLinearFold, DistortionType::kHardClip }),
                    c.range(0, 8), c.range(0.6, 1));
    Kit::compressor(c, c.range(0.6, 1));
    Kit::eq(c, c.range(18, 26), c.range(-3, -1));

    if (c.chance(0.5)) {
      c.tag = "Rolling";
      const std::vector<std::vector<int>>& rolling = bassRecipeRollingPatterns();
      const std::vector<int>& pattern = rolling[c.r.next(static_cast<int>(rolling.size()))];
      int gate = SequenceEncoder::applyRollingGate(p, oscs, pattern, c.range(0.55, 0.8), c.range(0.4, 0.8));
      p.mods.add("lfo_" + std::to_string(gate), "filter_1_cutoff", c.range(0.12, 0.22));
      p["filter_1_cutoff"] = p.get("filter_1_cutoff") - 8;
      p.comments = "Hold one note \xE2\x80\x94 the bass rolls in 1/16 with a gap for the kick.";
    }

    Kit::macro(c, 1, "CUTOFF", "filter_1_cutoff", 0.25);
    Kit::macro(c, 2, "DISTO", "distortion_drive", 0.35);
    p.mods.add("macro_control_2", "volume", -0.12);
    Kit::macro(c, 3, "DECAY", "env_2_decay", 0.2);
    if (oscs.size() > 1)
      Kit::macro(c, 4, "SUB", "osc_2_level", 0.3);
    else
      Kit::macro(c, 4, "WAVE", "osc_1_wave_frame", 0.3);
    Kit::volume(c, c.range(-7, -5));
  }

} // namespace gen
} // namespace corpuscular
