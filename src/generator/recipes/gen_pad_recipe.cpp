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

  void PadRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Additive", "Spectral", "Formant", "Basic Morph" }), c.rangeInt(4, 8)),
             c.range(0.6, 0.72), c.pick({ -12, -12, 0, -24 }), c.chance(0.3) ? 16 : c.rangeInt(2, 7), c.range(1.2, 2.5));
    p["osc_1_wave_frame"] = c.range(0, 200);
    Kit::osc(c, 2, Kit::table(c, c.pick({ "Additive", "Vocal", "PWM", "Spectral" }), c.rangeInt(3, 6)),
             c.range(0.45, 0.6), c.pick({ 0, 7, 12 }), c.rangeInt(2, 3), c.range(1, 2.2));
    p["osc_2_wave_frame"] = c.range(0, 200);
    std::vector<int> oscs = { 1, 2 };
    // Packs: a sub an octave or two down (Sonicspore pads reach -24)
    if (c.chance(0.64)) {
      Kit::osc(c, 3, Kit::table(c, "Basic Shapes", 1), c.range(0.35, 0.5), c.pick({ -12, -24 }), 1, 0);
      oscs = { 1, 2, 3 };
    }

    // Packs: cutoff around note 38-79, resonance 0.1-0.55; a slow swell (0.6-1.6 s) and a long tail (1-2.2 s)
    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kDigital, FilterModel::kLadder }), FilterStyle::kDb24,
                c.range(44, 80), c.range(0.12, 0.5), 0, PsyStyle::filterBlend(c, 0.2, 0.0), c.range(0, 0.3));
    Kit::env(c, 1, c.range(0.6, 1.6), c.range(1, 2), c.range(0.8, 1), c.range(1.0, 2.2));
    Kit::motion(c, "osc_1_wave_frame", c.range(0.25, 0.5), c.pick({ "4/1", "8/1" }), true);
    Kit::motion(c, "osc_2_wave_frame", c.range(0.2, 0.45), c.pick({ "8/1", "16/1" }), true);
    // Packs: the cutoff moves by LFO or smooth random (0.28-0.4); no random pan
    if (c.chance(0.5))
      Kit::motion(c, "filter_1_cutoff", c.range(0.1, 0.3), c.pick({ "4/1", "8/1" }), true);
    else
      Kit::random(c, 1, "filter_1_cutoff", c.range(0.15, 0.35), c.pick({ "1/2", "1/1" }), 2);

    PsyStyle::oscWarp(c, 1, 0.35);
    // Outerverse pads: random amplitudes / harmonic stretch / smear, its amount wandering by a smooth random
    if (c.chance(0.45)) {
      p["osc_1_spectral_morph_type"] = c.pick({ SpectralMorph::kRandomAmplitudes, SpectralMorph::kRandomAmplitudes,
                                                SpectralMorph::kHarmonicStretch, SpectralMorph::kSmear });
      p["osc_1_spectral_morph_amount"] = c.range(0.2, 0.5);
      if (c.chance(0.6))
        Kit::random(c, 2, "osc_1_spectral_morph_amount", c.range(0.3, 0.6), c.pick({ "1/4", "1/2" }), 2);
    }

    // Sonicspore pads: three in four are chopped by a trance gate on the osc levels
    if (c.chance(0.45)) {
      const std::vector<std::vector<int>> rhythms = { { 1, 1, 0, 1 }, { 1, 0, 1, 1 }, { 1, 0, 1, 0 }, { 1, 1, 1, 0 } };
      SequenceEncoder::applyRollingGate(p, oscs, rhythms[c.r.next(static_cast<int>(rhythms.size()))],
                                        c.range(0.55, 0.85), c.range(0.5, 0.85));
      c.tag = "Gated";
    }

    p["polyphony"] = 8;
    if (c.chance(0.6))
      Kit::chorus(c, c.range(0.35, 0.6));
    // Packs: pad reverb mix 0.24 at Outerverse, 0.44 at Sonicspore; the macro opens it further
    Kit::reverb(c, c.range(0.25, 0.5), c.range(0.6, 0.9), c.range(2.5, 6));
    Kit::delay(c, c.range(0.2, 0.35), c.range(0.4, 0.6), "1/4", c.chance(0.5), DelayStyle::kPingPong);
    if (c.chance(0.55))
      Kit::flanger(c, c.range(0.15, 0.3), c.range(0.2, 0.5), c.pick({ "4/1", "8/1" }));
    if (c.chance(0.35))
      Kit::phaser(c, c.range(0.2, 0.4), c.range(0.2, 0.5), "4/1");
    Kit::distortion(c, DistortionType::kSoftClip, c.range(-3, 0), c.range(0.3, 0.6));
    Kit::eq(c, c.range(30, 44), c.range(-3, -1));

    PsyStyle::producerMacros(c, "Pad");
    Kit::volume(c, c.range(-9, -7));
  }

} // namespace gen
} // namespace corpuscular
