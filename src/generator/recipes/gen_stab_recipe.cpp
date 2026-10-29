/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "../format/gen_patch.h"

namespace corpuscular {
namespace gen {

  void StabRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Basic Shapes", "Hard Sync", "Wavefold", "Digital" }), c.rangeInt(3, 6)),
             c.range(0.66, 0.76), c.pick({ -12, 0 }), c.rangeInt(4, 7), c.range(2.5, 5));
    p["osc_1_wave_frame"] = c.range(100, 230);
    if (c.chance(0.4))
      p["osc_1_stack_style"] = c.pick({ 3, 5, 8 });
    if (c.chance(0.6))
      Kit::osc(c, 2, Kit::table(c, c.pick({ "Basic Shapes", "PWM" }), 3), c.range(0.4, 0.55), c.pick({ 7, 12 }),
               c.rangeInt(2, 4), c.range(2, 4));

    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kDirty }), c.pick({ FilterStyle::kDb12, FilterStyle::kDb24 }),
                c.range(48, 70), c.range(0.3, 0.6), c.range(0, 6));
    Kit::env(c, 1, 0.0005, c.range(0.15, 0.3), c.range(0.05, 0.25), c.range(0.15, 0.35));
    Kit::env(c, 2, 0.0005, c.range(0.1, 0.22), 0, 0.15);
    p.mods.add("env_2", "filter_1_cutoff", c.range(0.35, 0.55));

    p["polyphony"] = 12;
    Kit::distortion(c, c.pick({ DistortionType::kSoftClip, DistortionType::kHardClip }), c.range(2, 9), c.range(0.5, 1));
    Kit::reverb(c, c.range(0.2, 0.4), c.range(0.5, 0.85), c.range(1.5, 4));
    if (c.chance(0.5))
      Kit::delay(c, c.range(0.15, 0.3), c.range(0.3, 0.5), "1/8", true, DelayStyle::kPingPong);
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(34, 46), c.range(-2, 0));

    Kit::macro(c, 1, "FILTER", "filter_1_cutoff", 0.3);
    Kit::macro(c, 2, "DISTO", "distortion_drive", 0.4);
    p.mods.add("macro_control_2", "volume", -0.12);
    Kit::macro(c, 3, "LENGTH", "env_1_decay", 0.3);
    Kit::macro(c, 4, "SPACE", "reverb_dry_wet", 0.35);
    Kit::volume(c, c.range(-9, -7));
  }

} // namespace gen
} // namespace corpuscular
