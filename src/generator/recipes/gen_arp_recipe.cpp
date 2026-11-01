/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
#include "gen_psy_style.h"
#include "../format/gen_patch.h"
#include "../music/gen_arp_algorithms.h"
#include "../music/gen_sequence_encoder.h"

namespace corpuscular {
namespace gen {

  void ArpRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    bool vocal = c.chance(0.25);
    std::string kind1 = vocal ? c.pick({ "Vocal", "Formant" })
                              : c.pick({ "Basic Shapes", "FM", "Digital", "Additive", "PWM" });
    // Packs: an arp sits an octave down (median register -12), mostly a single voice
    Kit::osc(c, 1, Kit::table(c, kind1, c.rangeInt(3, 6)), c.range(0.68, 0.78), c.pick({ -12, -12, -12, 0 }),
             c.chance(0.75) ? 1 : c.rangeInt(2, 4), c.range(1.5, 3.5));
    p["osc_1_wave_frame"] = c.range(40, 220);
    PsyStyle::oscWarp(c, 1, 0.45);
    std::vector<int> oscs = { 1 };
    if (c.chance(0.6)) {
      Kit::osc(c, 2, Kit::table(c, c.pick({ "Basic Shapes", "Hard Sync", "FM" }), c.rangeInt(2, 4)),
               c.range(0.4, 0.55), 12, 1, 1.5);
      p["osc_2_wave_frame"] = c.range(0, 200);
      oscs = { 1, 2 };
    }
    if (!vocal)
      PsyStyle::oscFm(c, 1, 0.15);

    if (vocal) {
      Kit::filter(c, 1, FilterModel::kFormant, 0, c.range(60, 90), c.range(0.4, 0.8));
      p["filter_1_formant_x"] = c.range(0.2, 0.8);
      p["filter_1_formant_y"] = c.range(0.2, 0.8);
      p["filter_1_formant_resonance"] = c.range(0.6, 0.95);
      Kit::motion(c, "filter_1_formant_x", c.range(0.25, 0.5), c.pick({ "1/1", "2/1" }), true);
      c.tag = "Vocal";
    }
    else {
      // Packs: cutoff around note 42-60, resonance 0.33-0.68, low-pass or band-pass
      Kit::filter(c, 1, PsyStyle::filterModel(c), FilterStyle::kDb12, c.range(42, 66), c.range(0.33, 0.68),
                  c.range(0, 4), PsyStyle::filterBlend(c, 0.4, 0.1), c.range(0.2, 0.5));
    }
    Kit::env(c, 1, 0.0005, 1, 1, c.range(0.02, 0.09));

    Phrase phrase = c.arp.make(c.phrase_kind, c.scale);
    std::string tempo = c.sequenceTempo();
    SequenceEncoder::applyPitch(p, phrase, c.scale, oscs, tempo);
    // Producers' arps are a held tone whose pitch steps (gated in only 20%)
    if (c.chance(0.25)) {
      int gate = SequenceEncoder::applyGate(p, phrase, oscs, tempo, c.range(0.4, 0.7), c.range(0.5, 0.85));
      p.mods.add("lfo_" + std::to_string(gate), "filter_1_cutoff", c.range(0.08, 0.18));
    }
    if (c.tag.empty())
      c.tag = phrase.kind;

    // Packs: the stepped random filter is in 64% of arps (0.29-0.59)
    PsyStyle::randomCutoff(c, 1, 1, 0.65);

    // Packs: 68% of arps are mono
    p["polyphony"] = c.chance(0.65) ? 1 : 8;
    Kit::delay(c, c.range(0.18, 0.32), c.range(0.3, 0.55), "1/8", c.chance(0.7), DelayStyle::kPingPong);
    // Packs: the arp room is mostly closed, a macro opens it
    if (c.chance(0.4))
      Kit::reverb(c, c.range(0.0, 0.12), c.range(0.35, 0.65), c.range(1.0, 2.5));
    if (c.chance(0.85))
      Kit::distortion(c, PsyStyle::distortionType(c), c.range(1, 7), c.range(0.4, 0.9));
    if (c.chance(0.55))
      Kit::phaser(c, c.range(0.3, 0.6), c.range(0.3, 0.6), c.pick({ "1/1", "2/1" }));
    if (vocal || c.chance(0.5))
      Kit::chorus(c, c.range(0.2, 0.45));
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(34, 46), c.range(-2, 0));

    p.comments = "Hold a note \xE2\x80\x94 the arp plays. Change the scale in the osc pitch quantization.";
    PsyStyle::producerMacros(c, "Arp");
    Kit::volume(c, c.range(-8, -6));
  }

} // namespace gen
} // namespace corpuscular
