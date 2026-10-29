/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "../format/gen_patch.h"
#include "../wavetables/gen_wavetable_factory.h"

namespace corpuscular {
namespace gen {

  void PercussionRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    std::string kind = c.pick({ "Kick", "Kick", "Zap", "Hat", "Perc" });
    c.tag = kind;
    p["polyphony"] = 4;
    Kit::env(c, 1, 0.0005, 0.25, 0, 0.05);

    if (kind == "Kick" || kind == "Zap") {
      bool kick = kind == "Kick";
      Kit::osc(c, 1, WavetableFactory::basicShapes(), c.range(0.72, 0.8), kick ? -24 : 0, 1, 0, OscDest::kDirect);
      p["osc_1_wave_frame"] = kick ? 0.0 : c.pick({ 0.0, 85.0 });
      p["osc_1_random_phase"] = 0;
      p["osc_1_phase"] = 0.25;
      Kit::env(c, 1, 0.0005, kick ? c.range(0.25, 0.45) : c.range(0.08, 0.2), 0, 0.05);
      Kit::env(c, 2, 0.0005, kick ? c.range(0.03, 0.08) : c.range(0.05, 0.15), 0, 0.03);
      p["env_2_decay_power"] = c.range(-6, -3);
      p.mods.add("env_2", "osc_1_transpose", kick ? c.range(0.3, 0.45) : c.range(0.4, 0.5));
      Kit::distortion(c, DistortionType::kSoftClip, kick ? c.range(3, 10) : c.range(0, 6), 1);
      if (kind == "Zap")
        Kit::delay(c, c.range(0.1, 0.25), c.range(0.2, 0.4), "1/16", false, DelayStyle::kPingPong);
      Kit::macro(c, 1, "PITCH", "osc_1_transpose", 0.1);
      Kit::macro(c, 2, "PUNCH", "env_2_decay", 0.2);
    }
    else {
      p["osc_1_on"] = 0;
      p.sample.reset(new SampleData(SampleData::whiteNoise(44100, c.r.next())));
      p["sample_on"] = 1;
      p["sample_level"] = c.range(0.6, 0.75);
      p["sample_destination"] = OscDest::kFilter1;
      bool hat = kind == "Hat";
      Kit::filter(c, 1, hat ? FilterModel::kDigital : FilterModel::kAnalog, FilterStyle::kDb12,
                  hat ? c.range(96, 118) : c.range(70, 96), c.range(0.2, 0.6), 0, hat ? 2 : 1);
      Kit::env(c, 1, 0.0005, hat ? c.range(0.03, 0.12) : c.range(0.08, 0.25), 0, hat ? 0.03 : 0.08);
      if (!hat) {
        Kit::osc(c, 1, Kit::table(c, c.pick({ "FM", "Digital" }), 2), c.range(0.45, 0.6), c.rangeInt(0, 12), 1, 0);
        Kit::env(c, 2, 0.0005, 0.05, 0, 0.03);
        p.mods.add("env_2", "osc_1_transpose", c.range(0.15, 0.3));
      }
      Kit::reverb(c, c.range(0.1, 0.25), c.range(0.2, 0.5), c.range(0.5, 1.5));
      Kit::macro(c, 1, "TONE", "filter_1_cutoff", 0.25);
      Kit::macro(c, 2, "LENGTH", "env_1_decay", 0.25);
    }

    Kit::compressor(c, c.range(0.6, 1));
    Kit::macro(c, 3, "DRIVE", "distortion_drive", 0.35);
    Kit::macro(c, 4, "SPACE", "reverb_dry_wet", 0.3);
    Kit::volume(c, c.range(-7, -5));
  }

} // namespace gen
} // namespace corpuscular
