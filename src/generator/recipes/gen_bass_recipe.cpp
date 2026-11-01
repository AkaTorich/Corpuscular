/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "gen_psy_style.h"
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

    // Packs: low-pass around note 40-57 with little resonance, a strong curved filter envelope (0.3-0.6)
    // that decays in 0.1-0.45 s to a low sustain
    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kAnalog, FilterModel::kDirty, FilterModel::kLadder }),
                c.pick({ FilterStyle::kDb24, FilterStyle::kDb12 }), c.range(38, 56), c.range(0.0, 0.25), c.range(0, 3));
    p["osc_2_destination"] = OscDest::kFilter1;
    Kit::env(c, 1, 0.0005, c.range(0.12, 0.6), c.range(0.7, 0.95), c.range(0.02, 0.08));
    Kit::env(c, 2, 0.0005, c.range(0.11, 0.45), c.range(0.0, 0.25), 0.05);
    p["env_2_decay_power"] = c.range(-6, -2);
    p.mods.add("env_2", "filter_1_cutoff", c.range(0.3, 0.6));

    // Packs: Sonicspore basses are mono in 96%
    p["polyphony"] = c.chance(0.85) ? 1 : 8;
    p["legato"] = 0;
    if (c.chance(0.7))
      Kit::distortion(c, c.pick({ DistortionType::kSoftClip, DistortionType::kSoftClip, DistortionType::kHardClip,
                                  DistortionType::kLinearFold }), c.range(2, 8), c.range(0.4, 1));
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

    PsyStyle::producerMacros(c, "Bass");
    Kit::volume(c, c.range(-7, -5));
  }

} // namespace gen
} // namespace corpuscular
