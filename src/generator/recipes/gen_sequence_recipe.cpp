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

  // Packs: a single voice in a half, a few in a third; Outerverse stacks the full 16 in a sixth
  static int sequenceVoices(RecipeContext& c) {
    double roll = c.r.nextDouble();
    if (roll < 0.5)
      return 1;
    if (roll < 0.83)
      return c.rangeInt(2, 5);
    return 16;
  }

  void SequenceRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    // Packs: a psy sequence sits low (osc 1 an octave or two down), mostly a single voice
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Basic Shapes", "Digital", "FM", "Wavefold", "PWM" }), c.rangeInt(3, 6)),
             c.range(0.66, 0.76), c.pick({ -12, -24, -24 }), sequenceVoices(c), c.range(1.5, 3));
    p["osc_1_wave_frame"] = c.range(80, 220);
    PsyStyle::oscWarp(c, 1, 0.55);
    PsyStyle::spectralMorph(c, 1, 0.4);
    std::vector<int> oscs = { 1 };
    if (c.chance(0.6)) {
      Kit::osc(c, 2, Kit::table(c, c.pick({ "Basic Shapes", "Additive", "Hard Sync" }), c.rangeInt(2, 5)),
               c.range(0.4, 0.58), c.pick({ 0, 12, -12 }), c.rangeInt(1, 2), c.range(1, 2.5));
      p["osc_2_wave_frame"] = c.range(0, 220);
      oscs = { 1, 2 };
    }
    PsyStyle::oscFm(c, 1, 0.2);

    // Packs: cutoff around note 33-65, resonance 0.35-0.63, low-pass or band-pass, sometimes high-pass
    Kit::filter(c, 1, PsyStyle::filterModel(c), c.pick({ FilterStyle::kDb12, FilterStyle::kDb24 }), c.range(36, 66),
                c.range(0.35, 0.63), c.range(0, 5), PsyStyle::filterBlend(c, 0.4, 0.12), c.range(0.2, 0.5));
    Kit::env(c, 1, 0.0005, c.range(0.6, 1), 1, c.range(0.03, 0.09));

    Phrase phrase = c.arp.make(c.phrase_kind, c.scale);
    std::string tempo = c.sequenceTempo();
    // Producers' sequences: chopped by a gate in 86%, a pitch pattern only in half (the rest is one note in rhythm)
    bool gated = c.chance(0.9);
    // Without a gate the pattern has to come from the pitch
    if (!gated || c.chance(0.5))
      SequenceEncoder::applyPitch(p, phrase, c.scale, oscs, tempo);
    if (gated) {
      int gate = SequenceEncoder::applyGate(p, phrase, oscs, tempo, c.range(0.5, 0.8), c.range(0.3, 0.7));
      p.mods.add("lfo_" + std::to_string(gate), "filter_1_cutoff", c.range(0.08, 0.16));
    }
    c.tag = phrase.kind;

    // Packs: the stepped random filter flicker is in 42% of sequences, at 0.25-0.55
    PsyStyle::randomCutoff(c, 1, 1, 0.75);
    if (p.get("osc_1_distortion_type") > 0.0 && c.chance(0.35))
      Kit::random(c, 2, "osc_1_distortion_amount", c.range(0.25, 0.55), "1/16", 1);
    Kit::motion(c, "osc_1_wave_frame", c.range(0.3, 0.7), c.pick({ "2/1", "4/1" }), true);

    // Packs: mono in 27% overall, 67% at Sonicspore (legato in a third of those); the room mostly dry
    p["polyphony"] = c.chance(0.4) ? 1 : 8;
    if (p.get("polyphony") == 1.0)
      p["legato"] = c.chance(0.35) ? 1 : 0;
    Kit::delay(c, c.range(0.08, 0.2), c.range(0.25, 0.5), c.pick({ "1/8", "1/16" }), c.chance(0.5), DelayStyle::kPingPong);
    if (c.chance(0.4))
      Kit::reverb(c, c.range(0.0, 0.12), c.range(0.3, 0.6), c.range(1.0, 2.5));
    if (c.chance(0.8))
      Kit::distortion(c, PsyStyle::distortionType(c), c.range(2, 10), c.range(0.6, 1));
    if (c.chance(0.3))
      Kit::phaser(c, c.range(0.3, 0.6), c.range(0.4, 0.7), c.pick({ "1/1", "2/1" }));
    if (c.chance(0.5))
      Kit::flanger(c, c.range(0.15, 0.35), c.range(0.3, 0.7), c.pick({ "2/1", "4/1" }));
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(30, 44), c.range(-2, 0));

    p.comments = "Hold a note \xE2\x80\x94 the sequence plays. Change the scale in the osc pitch quantization.";
    PsyStyle::producerMacros(c, "Sequence");
    Kit::volume(c, c.range(-8, -6));
  }

} // namespace gen
} // namespace corpuscular
