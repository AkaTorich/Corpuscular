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

  void SequenceRecipe::build(RecipeContext& c) const {
    Patch& p = c.p;
    Kit::osc(c, 1, Kit::table(c, c.pick({ "Basic Shapes", "Digital", "FM", "Wavefold", "PWM" }), c.rangeInt(3, 6)),
             c.range(0.66, 0.76), -12, c.rangeInt(1, 3), c.range(1.5, 3));
    p["osc_1_wave_frame"] = c.range(80, 220);
    std::vector<int> oscs = { 1 };
    if (c.chance(0.67)) {
      Kit::osc(c, 2, Kit::table(c, c.pick({ "Basic Shapes", "Additive", "Hard Sync" }), c.rangeInt(2, 5)),
               c.range(0.4, 0.58), c.pick({ 0, 12 }), c.rangeInt(1, 2), c.range(1, 2.5));
      p["osc_2_wave_frame"] = c.range(0, 220);
      oscs = { 1, 2 };
    }

    Kit::filter(c, 1, c.pick({ FilterModel::kAnalog, FilterModel::kDirty, FilterModel::kLadder }),
                c.pick({ FilterStyle::kDb12, FilterStyle::kDb24 }), c.range(52, 80), c.range(0.3, 0.7), c.range(0, 5),
                0, c.range(0.2, 0.5));
    Kit::env(c, 1, 0.0005, c.range(0.6, 1), 1, c.range(0.06, 0.15));

    Phrase phrase = c.arp.make(c.phrase_kind, c.scale);
    std::string tempo = c.sequenceTempo();
    SequenceEncoder::applyPitch(p, phrase, c.scale, oscs, tempo);
    if (c.chance(0.6)) {
      int gate = SequenceEncoder::applyGate(p, phrase, oscs, tempo, c.range(0.5, 0.8), c.range(0.3, 0.7));
      p.mods.add("lfo_" + std::to_string(gate), "filter_1_cutoff", c.range(0.08, 0.16));
    }
    c.tag = phrase.kind;

    Kit::random(c, 1, "filter_1_cutoff", c.range(0.06, 0.14), "1/16", 1);
    Kit::motion(c, "osc_1_wave_frame", c.range(0.15, 0.35), c.pick({ "2/1", "4/1" }), true);

    p["polyphony"] = 8;
    Kit::delay(c, c.range(0.12, 0.25), c.range(0.3, 0.5), c.pick({ "1/8", "1/16" }), c.chance(0.5), DelayStyle::kPingPong);
    if (c.chance(0.8))
      Kit::reverb(c, c.range(0.15, 0.3), c.range(0.4, 0.7), c.range(1.5, 3.5));
    Kit::distortion(c, c.pick({ DistortionType::kSoftClip, DistortionType::kHardClip }), c.range(1, 7), c.range(0.5, 1));
    if (c.chance(0.55))
      Kit::phaser(c, c.range(0.3, 0.6), c.range(0.3, 0.6), c.pick({ "1/1", "2/1" }));
    if (c.chance(0.6))
      Kit::flanger(c, c.range(0.15, 0.35), c.range(0.2, 0.6), c.pick({ "2/1", "4/1" }));
    Kit::compressor(c, c.range(0.5, 0.9));
    Kit::eq(c, c.range(30, 44), c.range(-2, 0));

    p.comments = "Hold a note \xE2\x80\x94 the sequence plays. Change the scale in the osc pitch quantization.";
    Kit::macro(c, 1, "FILTER", "filter_1_cutoff", 0.3);
    Kit::macro(c, 2, "DISTO", "distortion_drive", 0.45);
    p.mods.add("macro_control_2", "volume", -0.15);
    Kit::macro(c, 3, "DELAY", "delay_dry_wet", 0.35);
    Kit::macro(c, 4, "TEXTURE", "osc_1_wave_frame", 0.35);
    Kit::volume(c, c.range(-8, -6));
  }

} // namespace gen
} // namespace corpuscular
