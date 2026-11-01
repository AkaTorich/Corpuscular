/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "gen_psy_style.h"
#include "../format/gen_patch.h"

namespace corpuscular {
namespace gen {

  void StabRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Basic Shapes", "Hard Sync", "Wavefold", "Digital" }), c.rangeInt(3, 6)),
             c.range(0.66, 0.76), c.pick({ -12, 0 }), c.chance(0.4) ? 16 : c.rangeInt(4, 7), c.range(2.5, 5));
    p["osc_1_wave_frame"] = c.range(100, 230);
    if (c.chance(0.4))
      p["osc_1_stack_style"] = c.pick({ 3, 5, 8 });
    if (c.chance(0.6))
      Kit::osc(c, 2, Kit::table(c, c.pick({ "Basic Shapes", "PWM" }), 3), c.range(0.4, 0.55), c.pick({ 7, 12 }),
               c.rangeInt(2, 4), c.range(2, 4));

    // Packs: a closed filter (around note 38-56, resonance 0.4-0.5) thrown open by a strong envelope (0.5-0.8)
    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kDirty }), c.pick({ FilterStyle::kDb12, FilterStyle::kDb24 }),
                c.range(38, 56), c.range(0.35, 0.55), c.range(0, 6));
    Kit::env(c, 1, 0.0005, c.range(0.15, 0.3), c.range(0.05, 0.25), c.range(0.15, 0.35));
    Kit::env(c, 2, 0.0005, c.range(0.1, 0.25), 0, 0.15);
    p.mods.add("env_2", "filter_1_cutoff", c.range(0.5, 0.8));

    p["polyphony"] = 12;
    Kit::distortion(c, c.pick({ DistortionType::kSoftClip, DistortionType::kHardClip }), c.range(2, 9), c.range(0.5, 1));
    // Packs: stabs are dry, the room and the delay come in by macro
    Kit::reverb(c, c.range(0.0, 0.08), c.range(0.5, 0.85), c.range(1.5, 4));
    if (c.chance(0.9))
      Kit::delay(c, c.range(0.15, 0.3), c.range(0.3, 0.5), "1/8", true, DelayStyle::kPingPong);
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(34, 46), c.range(-2, 0));

    PsyStyle::producerMacros(c, "Stab");
    Kit::volume(c, c.range(-9, -7));
  }

} // namespace gen
} // namespace corpuscular
