/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "../format/gen_patch.h"

namespace corpuscular {
namespace gen {

  void LeadRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    std::string kind1 = c.pick({ "FM", "Digital", "Wavefold", "Hard Sync", "Phase Distortion", "Basic Morph" });
    bool wide = c.chance(0.6);
    Kit::osc(c, 1, Kit::table(c, kind1, c.rangeInt(4, 8)), c.range(0.66, 0.76), c.pick({ -12, -12, 0 }),
             wide ? c.rangeInt(3, 7) : 1, c.range(2, 4.5));
    p["osc_1_wave_frame"] = c.range(0, 128);
    if (c.chance(0.75)) {
      std::string kind2 = c.pick({ "Basic Morph", "PWM", "Additive", "Hard Sync", "FM" });
      Kit::osc(c, 2, Kit::table(c, kind2, c.rangeInt(3, 6)), c.range(0.45, 0.62), c.pick({ 0, 7, 12, -12 }),
               c.rangeInt(1, 4), c.range(1.5, 3.5), OscDest::kFilter1);
      p["osc_2_wave_frame"] = c.range(0, 200);
    }
    if (c.chance(0.15))
      Kit::osc(c, 3, Kit::table(c, "Basic Shapes", 1), c.range(0.35, 0.5), -24, 1, 0, OscDest::kFilter1);

    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kDirty, FilterModel::kLadder, FilterModel::kDigital }),
                c.pick({ FilterStyle::kDb12, FilterStyle::kDb24 }), c.range(48, 82), c.range(0.25, 0.6), c.range(0, 6),
                0, c.range(0, 0.4));
    Kit::env(c, 1, 0.0005, c.range(0.4, 0.9), c.range(0.75, 1), c.range(0.06, 0.2));
    Kit::env(c, 2, 0.0005, c.range(0.15, 0.5), c.range(0.1, 0.4), 0.1);
    p.mods.add("env_2", "filter_1_cutoff", c.range(0.18, 0.32));

    if (c.chance(0.6))
      Kit::random(c, 1, "osc_1_wave_frame", c.range(0.3, 0.9), c.pick({ "1/16", "1/8" }), c.pick({ 0, 1 }));
    else
      Kit::motion(c, "osc_1_wave_frame", c.range(0.2, 0.5), c.pick({ "1/1", "2/1" }), true);
    Kit::motion(c, "filter_1_cutoff", c.range(0.05, 0.15), c.pick({ "2/1", "4/1" }), true);
    Kit::vibrato(c, c.range(0.06, 0.14), { 1, 2 });

    p["polyphony"] = c.pick({ 1, 8, 8 });
    if (p.get("polyphony") == 1.0) {
      p["portamento_time"] = c.range(-7, -5);
      p["legato"] = 1;
    }
    Kit::delay(c, c.range(0.15, 0.3), c.range(0.3, 0.5), "1/8", c.chance(0.6), DelayStyle::kPingPong);
    if (c.chance(0.55))
      Kit::reverb(c, c.range(0.12, 0.28), c.range(0.4, 0.7), c.range(1.2, 3));
    Kit::distortion(c, c.pick({ DistortionType::kSoftClip, DistortionType::kHardClip, DistortionType::kLinearFold }),
                    c.range(1, 8), c.range(0.5, 1));
    if (c.chance(0.3))
      Kit::phaser(c, c.range(0.3, 0.6), c.range(0.3, 0.6), c.pick({ "1/2", "1/1" }));
    else if (c.chance(0.4))
      Kit::flanger(c, c.range(0.2, 0.4), c.range(0.2, 0.6), c.pick({ "1/1", "2/1" }));
    if (c.chance(0.3))
      Kit::chorus(c, c.range(0.2, 0.4));
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(30, 42), c.range(-2, 0));

    p.comments = "Arp it.";
    Kit::macro(c, 1, "FILTER", "filter_1_cutoff", 0.3);
    Kit::macro(c, 2, "DISTO", "distortion_drive", 0.45);
    p.mods.add("macro_control_2", "volume", -0.15);
    Kit::macro(c, 3, "DELAY", "delay_dry_wet", 0.35);
    p.mods.add("macro_control_3", "reverb_dry_wet", 0.25);
    if (p.get("phaser_on") > 0.0)
      Kit::macro(c, 4, "PHASER", "phaser_dry_wet", 0.5);
    else
      Kit::macro(c, 4, "FOCUS", "osc_1_unison_detune", -0.2);
    Kit::volume(c, c.range(-8, -6));
  }

} // namespace gen
} // namespace corpuscular
