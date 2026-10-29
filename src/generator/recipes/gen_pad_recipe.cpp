/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "../format/gen_patch.h"

namespace corpuscular {
namespace gen {

  void PadRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Additive", "Spectral", "Formant", "Basic Morph" }), c.rangeInt(4, 8)),
             c.range(0.6, 0.72), c.pick({ -12, 0 }), c.rangeInt(2, 4), c.range(1.2, 2.5));
    p["osc_1_wave_frame"] = c.range(0, 200);
    Kit::osc(c, 2, Kit::table(c, c.pick({ "Additive", "Vocal", "PWM", "Spectral" }), c.rangeInt(3, 6)),
             c.range(0.45, 0.6), c.pick({ 0, 7, 12 }), c.rangeInt(2, 3), c.range(1, 2.2));
    p["osc_2_wave_frame"] = c.range(0, 200);
    if (c.chance(0.64))
      Kit::osc(c, 3, Kit::table(c, "Basic Shapes", 1), c.range(0.35, 0.5), -12, 1, 0);

    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kDigital, FilterModel::kLadder }), FilterStyle::kDb24,
                c.range(56, 90), c.range(0.2, 0.5), 0, 0, c.range(0, 0.3));
    Kit::env(c, 1, c.range(0.6, 2), c.range(1, 2), c.range(0.8, 1), c.range(1.2, 3.5));
    Kit::motion(c, "osc_1_wave_frame", c.range(0.25, 0.5), c.pick({ "4/1", "8/1" }), true);
    Kit::motion(c, "osc_2_wave_frame", c.range(0.2, 0.45), c.pick({ "8/1", "16/1" }), true);
    Kit::motion(c, "filter_1_cutoff", c.range(0.06, 0.14), c.pick({ "4/1", "8/1" }), true);
    Kit::random(c, 1, "osc_1_pan", c.range(0.1, 0.25), "1/4", 2);

    p["polyphony"] = 16;
    Kit::chorus(c, c.range(0.35, 0.6));
    Kit::reverb(c, c.range(0.55, 0.9), c.range(0.7, 1), c.range(3, 8));
    Kit::delay(c, c.range(0.2, 0.35), c.range(0.4, 0.6), "1/4", c.chance(0.5), DelayStyle::kPingPong);
    if (c.chance(0.55))
      Kit::flanger(c, c.range(0.15, 0.3), c.range(0.2, 0.5), c.pick({ "4/1", "8/1" }));
    if (c.chance(0.35))
      Kit::phaser(c, c.range(0.2, 0.4), c.range(0.2, 0.5), "4/1");
    Kit::distortion(c, DistortionType::kSoftClip, c.range(-3, 0), c.range(0.3, 0.6));
    Kit::eq(c, c.range(30, 44), c.range(-3, -1));

    Kit::macro(c, 1, "FILTER", "filter_1_cutoff", 0.3);
    Kit::macro(c, 2, "EVOLVE", "osc_1_wave_frame", 0.5);
    p.mods.add("macro_control_2", "osc_2_wave_frame", 0.4);
    Kit::macro(c, 3, "SPACE", "reverb_size", 0.3);
    Kit::macro(c, 4, "ATTACK", "env_1_attack", 0.25);
    Kit::volume(c, c.range(-9, -7));
  }

} // namespace gen
} // namespace corpuscular
