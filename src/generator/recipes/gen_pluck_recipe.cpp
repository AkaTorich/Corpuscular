/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "../format/gen_patch.h"

namespace corpuscular {
namespace gen {

  void PluckRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Basic Shapes", "FM", "Additive", "Digital" }), c.rangeInt(3, 6)),
             c.range(0.68, 0.78), c.pick({ -12, 0 }), c.rangeInt(2, 5), c.range(1.5, 3));
    p["osc_1_wave_frame"] = c.range(60, 220);
    if (c.chance(0.5)) {
      Kit::osc(c, 2, Kit::table(c, "Basic Shapes", 1), c.range(0.35, 0.5), 12, 1, 0);
      p["osc_2_wave_frame"] = c.pick({ 0.0, 85.0, 171.0 });
    }
    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kLadder, FilterModel::kDigital }),
                c.pick({ FilterStyle::kDb12, FilterStyle::kDb24 }), c.range(42, 66), c.range(0.2, 0.5), 0, 0,
                c.range(0.3, 0.7));
    Kit::env(c, 1, 0.0005, c.range(0.15, 0.4), 0, c.range(0.1, 0.3));
    Kit::env(c, 2, 0.0005, c.range(0.08, 0.2), 0, 0.1);
    p.mods.add("env_2", "filter_1_cutoff", c.range(0.35, 0.55));
    p.mods.add("velocity", "filter_1_cutoff", c.range(0.08, 0.16));
    Kit::random(c, 1, "osc_1_wave_frame", c.range(0.15, 0.4), "1/16", 1);

    p["polyphony"] = 8;
    Kit::delay(c, c.range(0.25, 0.4), c.range(0.4, 0.6), "1/8", c.chance(0.6), DelayStyle::kPingPong);
    Kit::reverb(c, c.range(0.2, 0.35), c.range(0.4, 0.7), c.range(1.2, 3));
    if (c.chance(0.4))
      Kit::chorus(c, c.range(0.2, 0.4));
    Kit::compressor(c, c.range(0.4, 0.8));
    Kit::eq(c, c.range(36, 48), c.range(-1.5, 0));

    Kit::macro(c, 1, "FILTER", "filter_1_cutoff", 0.3);
    Kit::macro(c, 2, "DECAY", "env_1_decay", 0.25);
    p.mods.add("macro_control_2", "env_2_decay", 0.2);
    Kit::macro(c, 3, "SPACE", "delay_dry_wet", 0.3);
    p.mods.add("macro_control_3", "reverb_dry_wet", 0.3);
    Kit::macro(c, 4, "WIDTH", "osc_1_unison_detune", 0.25);
    Kit::volume(c, c.range(-8, -6));
  }

} // namespace gen
} // namespace corpuscular
