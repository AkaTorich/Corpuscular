/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe.h"

#include "gen_kit.h"
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
    Kit::osc(c, 1, Kit::table(c, kind1, c.rangeInt(3, 6)), c.range(0.68, 0.78), c.pick({ -12, 0 }),
             c.rangeInt(1, 4), c.range(1.5, 3.5));
    p["osc_1_wave_frame"] = c.range(40, 220);
    std::vector<int> oscs = { 1 };
    if (c.chance(0.6)) {
      Kit::osc(c, 2, Kit::table(c, c.pick({ "Basic Shapes", "Hard Sync", "FM" }), c.rangeInt(2, 4)),
               c.range(0.4, 0.55), 12, 1, 1.5);
      p["osc_2_wave_frame"] = c.range(0, 200);
      oscs = { 1, 2 };
    }

    if (vocal) {
      Kit::filter(c, 1, FilterModel::kFormant, 0, c.range(60, 90), c.range(0.4, 0.8));
      p["filter_1_formant_x"] = c.range(0.2, 0.8);
      p["filter_1_formant_y"] = c.range(0.2, 0.8);
      p["filter_1_formant_resonance"] = c.range(0.6, 0.95);
      Kit::motion(c, "filter_1_formant_x", c.range(0.25, 0.5), c.pick({ "1/1", "2/1" }), true);
      c.tag = "Vocal";
    }
    else {
      Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kDirty, FilterModel::kDigital }), FilterStyle::kDb12,
                  c.range(58, 86), c.range(0.35, 0.7), c.range(0, 4), 0, c.range(0.2, 0.5));
    }
    Kit::env(c, 1, 0.0005, 1, 1, c.range(0.08, 0.2));

    Phrase phrase = c.arp.make(c.phrase_kind, c.scale);
    std::string tempo = c.sequenceTempo();
    SequenceEncoder::applyPitch(p, phrase, c.scale, oscs, tempo);
    int gate = SequenceEncoder::applyGate(p, phrase, oscs, tempo, c.range(0.4, 0.7), c.range(0.5, 0.85));
    p.mods.add("lfo_" + std::to_string(gate), "filter_1_cutoff", c.range(0.08, 0.18));
    if (c.tag.empty())
      c.tag = phrase.kind;

    p["polyphony"] = 8;
    Kit::delay(c, c.range(0.2, 0.35), c.range(0.35, 0.55), "1/8", c.chance(0.7), DelayStyle::kPingPong);
    Kit::reverb(c, c.range(0.18, 0.32), c.range(0.45, 0.75), c.range(1.5, 3.5));
    if (c.chance(0.6))
      Kit::distortion(c, DistortionType::kSoftClip, c.range(0, 6), c.range(0.4, 0.9));
    if (c.chance(0.5))
      Kit::phaser(c, c.range(0.3, 0.6), c.range(0.3, 0.6), c.pick({ "1/1", "2/1" }));
    if (vocal || c.chance(0.3))
      Kit::chorus(c, c.range(0.2, 0.45));
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(34, 46), c.range(-2, 0));

    p.comments = "Hold a note \xE2\x80\x94 the arp plays. Change the scale in the osc pitch quantization.";
    Kit::macro(c, 1, "FILTER", "filter_1_cutoff", 0.3);
    Kit::macro(c, 2, "DISTO", "distortion_drive", 0.4);
    p.mods.add("macro_control_2", "volume", -0.12);
    Kit::macro(c, 3, "DELAY", "delay_dry_wet", 0.35);
    p.mods.add("macro_control_3", "reverb_dry_wet", 0.3);
    Kit::macro(c, 4, "TEXTURE", "osc_1_wave_frame", 0.4);
    Kit::volume(c, c.range(-8, -6));
  }

} // namespace gen
} // namespace corpuscular
