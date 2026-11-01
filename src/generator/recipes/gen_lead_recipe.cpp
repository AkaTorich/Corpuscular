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

  void LeadRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    std::string kind1 = c.pick({ "FM", "Digital", "Wavefold", "Hard Sync", "Phase Distortion", "Basic Morph" });
    // Packs: 65% single voice, the rest a few detuned voices; osc 1 mostly an octave down
    bool wide = c.chance(0.35);
    // (Sonicspore leads sit lower still: the lowest osc two octaves down)
    Kit::osc(c, 1, Kit::table(c, kind1, c.rangeInt(4, 8)), c.range(0.66, 0.76), c.pick({ -24, -12, -12, 0 }),
             wide ? c.rangeInt(3, 6) : 1, c.range(2, 4.5));
    p["osc_1_wave_frame"] = c.range(0, 128);
    PsyStyle::oscWarp(c, 1, 0.5);
    PsyStyle::spectralMorph(c, 1, 0.35);
    if (c.chance(0.6)) {
      std::string kind2 = c.pick({ "Basic Morph", "PWM", "Additive", "Hard Sync", "FM" });
      Kit::osc(c, 2, Kit::table(c, kind2, c.rangeInt(3, 6)), c.range(0.45, 0.62), c.pick({ 0, 12, -12, -12 }),
               c.rangeInt(1, 3), c.range(1.5, 3.5), OscDest::kFilter1);
      p["osc_2_wave_frame"] = c.range(0, 200);
    }
    if (c.chance(0.15))
      Kit::osc(c, 3, Kit::table(c, "Basic Shapes", 1), c.range(0.35, 0.5), -24, 1, 0, OscDest::kFilter1);
    PsyStyle::oscFm(c, 1, 0.25);

    // Packs: cutoff around note 40-64 with medium resonance (0.35-0.6), often band-pass
    Kit::filter(c, 1, PsyStyle::filterModel(c), c.pick({ FilterStyle::kDb12, FilterStyle::kDb24 }), c.range(40, 66),
                c.range(0.35, 0.62), c.range(0, 4), PsyStyle::filterBlend(c, 0.35, 0.1), c.range(0, 0.4));
    // A lead holds while the key is down and stops cleanly: instant attack, full sustain, short release
    Kit::env(c, 1, 0.0005, c.range(0.4, 0.9), c.range(0.8, 1), c.range(0.04, 0.12));
    // Producers' leads: the filter envelope only in 15%, an LFO on the cutoff in 25% (0.09-0.46), the stepped
    // random in 31%, the wave position by random (22%) or LFO (13%); no vibrato among their common modulations
    if (c.chance(0.2)) {
      Kit::env(c, 2, 0.0005, c.range(0.15, 0.5), c.range(0.1, 0.4), 0.1);
      p.mods.add("env_2", "filter_1_cutoff", c.range(0.12, 0.3));
    }
    if (c.chance(0.6))
      Kit::random(c, 1, "osc_1_wave_frame", c.range(0.5, 1.0), c.pick({ "1/16", "1/8" }), c.pick({ 0, 1 }));
    else
      Kit::motion(c, "osc_1_wave_frame", c.range(0.3, 0.8), c.pick({ "1/1", "2/1" }), true);
    PsyStyle::randomCutoff(c, 2, 1, 0.35);
    if (c.chance(0.3))
      Kit::motion(c, "filter_1_cutoff", c.range(0.1, 0.45), c.pick({ "2/1", "4/1" }), true);
    if (c.chance(0.15))
      Kit::vibrato(c, c.range(0.04, 0.1), { 1, 2 });
    // Sonicspore: a fifth of the leads are chopped by a gate
    if (c.chance(0.15)) {
      std::vector<int> oscs = { 1 };
      if (p.get("osc_2_on") > 0.0)
        oscs.push_back(2);
      const std::vector<std::vector<int>> rhythms = { { 1, 1, 0, 1 }, { 1, 0, 1, 1 }, { 1, 1, 1, 0 } };
      SequenceEncoder::applyRollingGate(p, oscs, rhythms[c.r.next(static_cast<int>(rhythms.size()))],
                                        c.range(0.6, 0.9), c.range(0.5, 0.85));
      c.tag = "Gated";
    }

    // Packs: mono in 44%, legato in 17%
    p["polyphony"] = c.chance(0.45) ? 1 : 8;
    if (p.get("polyphony") == 1.0) {
      p["portamento_time"] = c.range(-7, -5);
      p["legato"] = c.chance(0.4) ? 1 : 0;
    }
    Kit::delay(c, c.range(0.12, 0.25), c.range(0.2, 0.5), "1/8", c.chance(0.6), DelayStyle::kPingPong);
    if (c.chance(0.5))
      Kit::reverb(c, c.range(0.1, 0.22), c.range(0.35, 0.6), c.range(1.0, 2.5));
    if (c.chance(0.65))
      Kit::distortion(c, PsyStyle::distortionType(c), c.range(1.5, 10), c.range(0.55, 1));
    if (c.chance(0.3))
      Kit::phaser(c, c.range(0.3, 0.6), c.range(0.3, 0.6), c.pick({ "1/2", "1/1" }));
    else if (c.chance(0.4))
      Kit::flanger(c, c.range(0.2, 0.4), c.range(0.2, 0.6), c.pick({ "1/1", "2/1" }));
    if (c.chance(0.3))
      Kit::chorus(c, c.range(0.2, 0.4));
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(30, 42), c.range(-2, 0));

    p.comments = "Arp it.";
    PsyStyle::producerMacros(c, "Lead");
    Kit::volume(c, c.range(-8, -6));
  }

} // namespace gen
} // namespace corpuscular
