/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "../format/gen_patch.h"

namespace corpuscular {
namespace gen {

  void VocalRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Vocal", "Formant" }), c.rangeInt(4, 8)), c.range(0.66, 0.76), -12,
             c.rangeInt(1, 3), c.range(1, 2.5));
    p["osc_1_wave_frame"] = c.range(0, 200);
    if (c.chance(0.5)) {
      p["osc_1_spectral_morph_type"] = SpectralMorph::kVocode;
      p["osc_1_spectral_morph_amount"] = c.range(0.3, 0.7);
    }
    if (c.chance(0.5))
      Kit::osc(c, 2, Kit::table(c, "Vocal", c.rangeInt(3, 5)), c.range(0.4, 0.55), c.pick({ 0, 12 }),
               c.rangeInt(2, 3), c.range(1, 2));

    Kit::filter(c, 1, FilterModel::kFormant, 0, c.range(60, 90), c.range(0.4, 0.8));
    p["filter_1_formant_x"] = c.range(0.2, 0.8);
    p["filter_1_formant_y"] = c.range(0.2, 0.8);
    p["filter_1_formant_resonance"] = c.range(0.65, 0.95);
    p["filter_1_formant_transpose"] = c.range(-3, 3);
    Kit::env(c, 1, c.range(0.005, 0.15), c.range(0.5, 1.2), c.range(0.7, 1), c.range(0.3, 1));

    Kit::motion(c, "filter_1_formant_x", c.range(0.3, 0.6), c.pick({ "1/1", "2/1" }), true);
    Kit::motion(c, "filter_1_formant_y", c.range(0.2, 0.5), c.pick({ "2/1", "4/1" }), true);
    Kit::random(c, 1, "osc_1_wave_frame", c.range(0.2, 0.5), "1/8", 2);
    Kit::vibrato(c, c.range(0.05, 0.12), { 1, 2 });

    p["polyphony"] = 8;
    Kit::chorus(c, c.range(0.3, 0.5));
    Kit::reverb(c, c.range(0.3, 0.55), c.range(0.6, 0.9), c.range(2, 5));
    Kit::delay(c, c.range(0.15, 0.3), c.range(0.35, 0.55), "1/8", true, DelayStyle::kPingPong);
    if (c.chance(0.6))
      Kit::phaser(c, c.range(0.25, 0.5), c.range(0.3, 0.6), "2/1");
    Kit::compressor(c, c.range(0.4, 0.8));
    Kit::eq(c, c.range(36, 48), c.range(-2, 0));

    Kit::macro(c, 1, "VOWEL", "filter_1_formant_x", 0.5);
    Kit::macro(c, 2, "GENDER", "filter_1_formant_transpose", 0.25);
    Kit::macro(c, 3, "SPACE", "reverb_dry_wet", 0.35);
    Kit::macro(c, 4, "CHORUS", "chorus_dry_wet", 0.4);
    Kit::volume(c, c.range(-8, -6));
  }

} // namespace gen
} // namespace corpuscular
