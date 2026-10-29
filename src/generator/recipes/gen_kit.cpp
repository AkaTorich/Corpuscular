/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_kit.h"

#include "gen_recipe.h"
#include "../format/gen_patch.h"
#include "../wavetables/gen_wavetable_factory.h"

#include <algorithm>
#include <cmath>

namespace corpuscular {
namespace gen {

  namespace Kit {
    void osc(RecipeContext& c, int n, WavetableData table, double level, int transpose,
             int voices, double detune, int destination) {
      Patch& p = c.p;
      std::string o = "osc_" + std::to_string(n) + "_";
      p[o + "on"] = 1;
      p.wavetables[n - 1].reset(new WavetableData(std::move(table)));
      p[o + "level"] = level;
      p[o + "transpose"] = transpose;
      p[o + "unison_voices"] = voices;
      p[o + "unison_detune"] = detune;
      p[o + "unison_blend"] = voices > 1 ? c.range(0.6, 0.9) : 0.8;
      p[o + "stereo_spread"] = voices > 1 ? c.range(0.7, 1.0) : 1.0;
      p[o + "destination"] = destination;
      p[o + "random_phase"] = voices > 1 ? 1.0 : c.pick({ 0.0, 1.0 });
      p[o + "wave_frame"] = 0;
    }

    WavetableData table(RecipeContext& c, const std::string& kind, int keyframes) {
      if (kind == "Basic Shapes")
        return WavetableFactory::basicShapes();
      if (kind == "Init")
        return WavetableFactory::init();
      return WavetableFactory::make(kind, c.r, keyframes, kind);
    }

    void filter(RecipeContext& c, int n, int model, int style, double cutoff, double resonance,
                double drive, double blend, double keytrack) {
      std::string f = "filter_" + std::to_string(n) + "_";
      c.p[f + "on"] = 1;
      c.p[f + "model"] = model;
      c.p[f + "style"] = style;
      c.p[f + "cutoff"] = cutoff;
      c.p[f + "resonance"] = resonance;
      c.p[f + "drive"] = drive;
      c.p[f + "blend"] = blend;
      c.p[f + "keytrack"] = keytrack;
      c.p[f + "mix"] = 1;
    }

    void env(RecipeContext& c, int n, double attack, double decay, double sustain, double release) {
      std::string e = "env_" + std::to_string(n) + "_";
      c.p[e + "attack"] = ParamUnits::envSeconds(attack);
      c.p[e + "decay"] = ParamUnits::envSeconds(decay);
      c.p[e + "sustain"] = sustain;
      c.p[e + "release"] = ParamUnits::envSeconds(release);
    }

    void volume(RecipeContext& c, double db) {
      c.p["volume"] = ParamUnits::volumeDb(db);
    }

    void delay(RecipeContext& c, double mix, double feedback, const std::string& tempo, bool dotted, int style) {
      Patch& p = c.p;
      p["delay_on"] = 1;
      p["delay_dry_wet"] = mix;
      p["delay_feedback"] = feedback;
      p["delay_sync"] = dotted ? LfoSync::kDotted : LfoSync::kTempo;
      p["delay_tempo"] = ParamUnits::tempo(tempo);
      p["delay_aux_sync"] = p["delay_sync"];
      p["delay_aux_tempo"] = ParamUnits::tempo(c.pick({ tempo.c_str(), "1/16", "1/8" }));
      p["delay_style"] = style;
      p["delay_filter_cutoff"] = c.range(64, 88);
      p["delay_filter_spread"] = c.range(0.4, 0.9);
    }

    void reverb(RecipeContext& c, double mix, double size, double decay_seconds) {
      Patch& p = c.p;
      p["reverb_on"] = 1;
      p["reverb_dry_wet"] = mix;
      p["reverb_size"] = size;
      p["reverb_decay_time"] = std::log2(std::max(0.1, decay_seconds));
      p["reverb_pre_low_cutoff"] = c.range(20, 45);
      p["reverb_pre_high_cutoff"] = c.range(95, 115);
      p["reverb_high_shelf_cutoff"] = c.range(80, 100);
      p["reverb_high_shelf_gain"] = c.range(-3, -0.5);
      p["reverb_chorus_amount"] = c.range(0.15, 0.35);
    }

    void phaser(RecipeContext& c, double mix, double feedback, const std::string& tempo) {
      Patch& p = c.p;
      p["phaser_on"] = 1;
      p["phaser_dry_wet"] = mix;
      p["phaser_feedback"] = feedback;
      p["phaser_sync"] = LfoSync::kTempo;
      p["phaser_tempo"] = ParamUnits::tempo(tempo);
      p["phaser_center"] = c.range(60, 90);
      p["phaser_mod_depth"] = c.range(12, 30);
      p["phaser_blend"] = c.range(0.6, 1.4);
    }

    void flanger(RecipeContext& c, double mix, double feedback, const std::string& tempo) {
      Patch& p = c.p;
      p["flanger_on"] = 1;
      p["flanger_dry_wet"] = mix;
      p["flanger_feedback"] = feedback;
      p["flanger_sync"] = LfoSync::kTempo;
      p["flanger_tempo"] = ParamUnits::tempo(tempo);
      p["flanger_center"] = c.range(56, 84);
      p["flanger_mod_depth"] = c.range(0.3, 0.7);
    }

    void chorus(RecipeContext& c, double mix) {
      Patch& p = c.p;
      p["chorus_on"] = 1;
      p["chorus_dry_wet"] = mix;
      p["chorus_voices"] = c.rangeInt(2, 4);
      p["chorus_feedback"] = c.range(0.1, 0.4);
      p["chorus_frequency"] = c.range(-4, -2);
      p["chorus_sync"] = LfoSync::kSeconds;
      p["chorus_mod_depth"] = c.range(0.3, 0.7);
      p["chorus_cutoff"] = c.range(70, 100);
      p["chorus_spread"] = c.range(0.6, 1);
    }

    void distortion(RecipeContext& c, int type, double drive, double mix) {
      Patch& p = c.p;
      p["distortion_on"] = 1;
      p["distortion_type"] = type;
      p["distortion_drive"] = drive;
      p["distortion_mix"] = mix;
    }

    void compressor(RecipeContext& c, double mix) {
      c.p["compressor_on"] = 1;
      c.p["compressor_mix"] = mix;
      c.p["compressor_enabled_bands"] = c.pick({ 0, 0, 3 });
    }

    void eq(RecipeContext& c, double low_cut_note, double high_gain) {
      Patch& p = c.p;
      p["eq_on"] = 1;
      p["eq_low_mode"] = 1;
      p["eq_low_cutoff"] = low_cut_note;
      p["eq_low_resonance"] = 0.3;
      p["eq_high_mode"] = 0;
      p["eq_high_cutoff"] = c.range(100, 112);
      p["eq_high_gain"] = high_gain;
    }

    void psyChain(RecipeContext& c) {
      c.p["effect_chain_order"] = ParamUnits::effectOrder({
        Fx::kDistortion, Fx::kFilterFx, Fx::kEq, Fx::kCompressor, Fx::kPhaser,
        Fx::kFlanger, Fx::kChorus, Fx::kDelay, Fx::kReverb
      });
    }

    void vibrato(RecipeContext& c, double amount, const std::vector<int>& oscs) {
      if (!c.p.hasFreeLfo())
        return;
      int l = c.p.takeLfo();
      c.p.lfos[l - 1] = LfoShape::sine();
      std::string n = "lfo_" + std::to_string(l) + "_";
      c.p[n + "sync"] = LfoSync::kSeconds;
      c.p[n + "frequency"] = ParamUnits::lfoHz(c.range(4.5, 6.5));
      c.p[n + "delay_time"] = c.range(0.15, 0.4);
      c.p[n + "fade_time"] = c.range(0.3, 0.8);
      c.p[n + "sync_type"] = 0;
      for (int o : oscs)
        c.p.mods.add("lfo_" + std::to_string(l), "osc_" + std::to_string(o) + "_tune", amount, true);
    }

    int motion(RecipeContext& c, const std::string& destination, double amount, const std::string& tempo, bool bipolar) {
      if (!c.p.hasFreeLfo())
        return 0;
      int l = c.p.takeLfo();
      c.p.lfos[l - 1] = c.chance(0.5) ? LfoShape::triangle() : LfoShape::sine();
      std::string n = "lfo_" + std::to_string(l) + "_";
      c.p[n + "sync"] = LfoSync::kTempo;
      c.p[n + "tempo"] = ParamUnits::tempo(tempo);
      c.p[n + "sync_type"] = LfoTrigger::kSync;
      c.p.mods.add("lfo_" + std::to_string(l), destination, amount, bipolar);
      return l;
    }

    void random(RecipeContext& c, int n, const std::string& destination, double amount, const std::string& tempo, int style) {
      std::string r = "random_" + std::to_string(n) + "_";
      c.p[r + "style"] = style;
      c.p[r + "sync"] = LfoSync::kTempo;
      c.p[r + "tempo"] = ParamUnits::tempo(tempo);
      c.p.mods.add("random_" + std::to_string(n), destination, amount);
    }

    void macro(RecipeContext& c, int n, const std::string& name, const std::string& destination, double amount) {
      c.p.macros[n - 1] = name;
      c.p.mods.add("macro_control_" + std::to_string(n), destination, amount);
    }
  }

} // namespace gen
} // namespace corpuscular
