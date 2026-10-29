/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "../format/gen_patch.h"

namespace corpuscular {
namespace gen {

  void SfxRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Chaos", "Spectral", "FM", "Digital", "Wavefold" }), c.rangeInt(4, 8)),
             c.range(0.62, 0.74), c.pick({ -12, 0, 12 }), c.rangeInt(1, 6), c.range(1.5, 6));
    p["osc_1_wave_frame"] = c.range(0, 256);
    if (c.chance(0.5))
      Kit::osc(c, 2, Kit::table(c, c.pick({ "Chaos", "Hard Sync", "Formant" }), c.rangeInt(3, 6)), c.range(0.4, 0.55),
               c.rangeInt(-24, 24), c.rangeInt(1, 3), c.range(1, 4));
    if (c.chance(0.5)) {
      p.sample.reset(new SampleData(SampleData::whiteNoise(44100, c.r.next())));
      p["sample_on"] = 1;
      p["sample_level"] = c.range(0.3, 0.55);
      p["sample_destination"] = OscDest::kFilter1;
    }

    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kComb, FilterModel::kPhaser, FilterModel::kDirty }), 0,
                c.range(40, 100), c.range(0.4, 0.85), c.range(0, 8), c.range(0, 1.2));
    bool riser = c.chance(0.5);
    Kit::env(c, 1, riser ? c.range(1, 4) : 0.001, c.range(1, 3), riser ? 1.0 : c.range(0, 0.6), c.range(1, 4));
    Kit::env(c, 3, riser ? c.range(2, 6) : 0.001, riser ? 2.0 : c.range(1, 4), riser ? 1.0 : 0.0, 1);
    p.mods.add("env_3", "osc_1_transpose", riser ? c.range(0.15, 0.3) : -c.range(0.15, 0.3));
    p.mods.add("env_3", "filter_1_cutoff", riser ? c.range(0.25, 0.45) : -c.range(0.2, 0.4));
    c.tag = riser ? "Rise" : "Fall";

    Kit::random(c, 1, "filter_1_cutoff", c.range(0.15, 0.35), c.pick({ "1/16", "1/8" }), c.pick({ 0, 1, 3 }));
    Kit::random(c, 2, "osc_1_wave_frame", c.range(0.4, 0.9), "1/16", 1);
    Kit::motion(c, "osc_1_distortion_amount", c.range(0.2, 0.5), "1/2", true);
    p["osc_1_distortion_type"] = c.pick({ OscDistortion::kBend, OscDistortion::kSqueeze, OscDistortion::kSync,
                                          OscDistortion::kFormant });

    p["polyphony"] = 8;
    Kit::delay(c, c.range(0.25, 0.45), c.range(0.5, 0.75), c.pick({ "1/8", "1/16" }), c.chance(0.5), DelayStyle::kPingPong);
    Kit::reverb(c, c.range(0.35, 0.7), c.range(0.7, 1), c.range(3, 8));
    if (c.chance(0.7))
      Kit::flanger(c, c.range(0.2, 0.45), c.range(0.4, 0.8), c.pick({ "1/4", "1/2" }));
    if (c.chance(0.4))
      Kit::phaser(c, c.range(0.3, 0.6), c.range(0.4, 0.8), "1/2");
    Kit::distortion(c, c.pick({ DistortionType::kSineFold, DistortionType::kBitCrush, DistortionType::kSoftClip }),
                    c.range(0, 10), c.range(0.4, 0.9));
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(30, 46), c.range(-3, 0));

    Kit::macro(c, 1, "FILTER", "filter_1_cutoff", 0.35);
    Kit::macro(c, 2, "DISTO", "distortion_drive", 0.45);
    p.mods.add("macro_control_2", "volume", -0.15);
    Kit::macro(c, 3, "SPACE", "reverb_dry_wet", 0.35);
    Kit::macro(c, 4, "CHAOS", "osc_1_distortion_amount", 0.4);
    Kit::volume(c, c.range(-10, -7));
  }

} // namespace gen
} // namespace corpuscular
