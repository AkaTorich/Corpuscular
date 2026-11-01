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

  void KeysRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    // Packs: keys use 1-4 slightly detuned voices, an octave down
    Kit::osc(c, 1, Kit::table(c, c.pick({ "FM", "Additive", "Phase Distortion" }), c.rangeInt(3, 6)),
             c.range(0.66, 0.76), c.pick({ -12, -12, 0 }), c.rangeInt(1, 4), c.range(0.8, 1.8));
    p["osc_1_wave_frame"] = c.range(0, 120);
    Kit::osc(c, 2, Kit::table(c, "Basic Shapes", 1), c.range(0.3, 0.45), 12, c.rangeInt(1, 2), c.range(0.5, 1.5));
    p["osc_2_wave_frame"] = c.pick({ 0.0, 85.0 });

    // Packs: cutoff around note 32-73 with more resonance (0.2-0.55)
    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kLadder }), FilterStyle::kDb12, c.range(48, 80),
                c.range(0.2, 0.5), 0, 0, c.range(0.4, 0.75));
    Kit::env(c, 1, 0.001, c.range(0.6, 1.4), c.range(0.25, 0.5), c.range(0.3, 0.8));
    Kit::env(c, 2, 0.0005, c.range(0.3, 0.8), 0, 0.3);
    p.mods.add("env_2", "osc_1_wave_frame", c.range(0.15, 0.35));
    PsyStyle::randomCutoff(c, 1, 1, 0.25);

    p["polyphony"] = 12;
    Kit::chorus(c, c.range(0.25, 0.45));
    Kit::reverb(c, c.range(0.08, 0.2), c.range(0.4, 0.7), c.range(1.5, 3));
    if (c.chance(0.75))
      Kit::delay(c, c.range(0.1, 0.2), c.range(0.2, 0.4), "1/8", true, DelayStyle::kPingPong);
    if (c.chance(0.5))
      Kit::distortion(c, DistortionType::kSoftClip, c.range(1, 5), c.range(0.3, 0.7));
    Kit::compressor(c, c.range(0.3, 0.6));
    Kit::eq(c, c.range(30, 40), c.range(-1, 0));

    PsyStyle::producerMacros(c, "Keys");
    Kit::volume(c, c.range(-8, -6));
  }

} // namespace gen
} // namespace corpuscular
