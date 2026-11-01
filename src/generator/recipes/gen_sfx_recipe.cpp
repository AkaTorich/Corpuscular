/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "gen_psy_style.h"
#include "../format/gen_patch.h"
#include "../music/gen_sequence_encoder.h"

namespace corpuscular {
namespace gen {

  void SfxRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    // Producers' psy SFX: a held low tone (osc an octave or two down, mostly one voice) that LFOs sweep in pitch
    // (52%) and cutoff (52%), sometimes chopped by a gate (37%); dry, the delay comes in by macro
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Chaos", "Spectral", "FM", "Digital", "Wavefold" }), c.rangeInt(4, 8)),
             c.range(0.62, 0.74), c.pick({ -12, -24, -24 }), c.chance(0.65) ? 1 : c.rangeInt(2, 4), c.range(1.5, 4));
    p["osc_1_wave_frame"] = c.range(0, 256);
    PsyStyle::oscWarp(c, 1, 0.6);
    std::vector<int> oscs = { 1 };
    if (c.chance(0.4)) {
      Kit::osc(c, 2, Kit::table(c, c.pick({ "Chaos", "Hard Sync", "Formant" }), c.rangeInt(3, 6)), c.range(0.4, 0.55),
               c.pick({ -12, 0, 7 }), 1, 1.5);
      oscs = { 1, 2 };
    }
    PsyStyle::oscFm(c, 1, 0.3);
    if (c.chance(0.25)) {
      p.sample.reset(new SampleData(SampleData::whiteNoise(44100, c.r.next())));
      p["sample_on"] = 1;
      p["sample_level"] = c.range(0.25, 0.45);
      p["sample_destination"] = OscDest::kFilter1;
    }

    Kit::filter(c, 1, PsyStyle::filterModel(c), c.pick({ FilterStyle::kDb12, FilterStyle::kDb24 }), c.range(45, 75),
                c.range(0.4, 0.75), c.range(0, 6), PsyStyle::filterBlend(c, 0.4, 0.15));
    Kit::env(c, 1, 0.0005, 1, 1, c.range(0.05, 0.15));

    // The sweep: tempo LFOs on the pitch and the cutoff
    for (int osc : oscs)
      Kit::motion(c, "osc_" + std::to_string(osc) + "_transpose", c.range(0.2, 0.45), c.pick({ "1/2", "1/1", "2/1" }), true);
    Kit::motion(c, "filter_1_cutoff", c.range(0.25, 0.5), c.pick({ "1/4", "1/2", "1/1" }), true);
    c.tag = "Sweep";
    if (c.chance(0.37)) {
      const std::vector<std::vector<int>> rhythms = { { 1, 0, 1, 1 }, { 1, 1, 0, 1 }, { 1, 0, 0, 1 } };
      SequenceEncoder::applyRollingGate(p, oscs, rhythms[c.r.next(3)], c.range(0.4, 0.8), c.range(0.3, 0.7));
      c.tag = "Gated";
    }
    if (c.chance(0.3))
      Kit::motion(c, "osc_1_wave_frame", c.range(0.4, 0.9), c.pick({ "1/1", "2/1" }), true);
    PsyStyle::randomCutoff(c, 1, 1, 0.3);

    p["polyphony"] = c.chance(0.25) ? 1 : 8;
    Kit::delay(c, c.range(0.08, 0.2), c.range(0.35, 0.6), c.pick({ "1/8", "1/16" }), c.chance(0.5), DelayStyle::kPingPong);
    if (c.chance(0.35))
      Kit::reverb(c, c.range(0.1, 0.3), c.range(0.4, 0.8), c.range(1.5, 4));
    if (c.chance(0.3))
      Kit::flanger(c, c.range(0.15, 0.35), c.range(0.4, 0.8), c.pick({ "1/4", "1/2" }));
    if (c.chance(0.3))
      Kit::phaser(c, c.range(0.3, 0.6), c.range(0.4, 0.8), "1/2");
    if (c.chance(0.5))
      Kit::distortion(c, PsyStyle::distortionType(c), c.range(2, 10), c.range(0.4, 0.9));
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(30, 46), c.range(-3, 0));

    PsyStyle::producerMacros(c, "SFX");
    Kit::volume(c, c.range(-10, -7));
  }

} // namespace gen
} // namespace corpuscular
