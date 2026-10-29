/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "../format/gen_units.h"
#include "../format/gen_wavetable.h"

#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  struct RecipeContext;

  // Shared building steps: oscillators, filters, envelopes, effects, macros, all in raw parameter units
  namespace Kit {
    void osc(RecipeContext& c, int n, WavetableData table, double level, int transpose,
             int voices, double detune, int destination = OscDest::kFilter1);

    WavetableData table(RecipeContext& c, const std::string& kind, int keyframes);

    void filter(RecipeContext& c, int n, int model, int style, double cutoff, double resonance,
                double drive = 0.0, double blend = 0.0, double keytrack = 0.0);

    // Envelope in seconds
    void env(RecipeContext& c, int n, double attack, double decay, double sustain, double release);

    void volume(RecipeContext& c, double db);

    void delay(RecipeContext& c, double mix, double feedback, const std::string& tempo, bool dotted, int style);
    void reverb(RecipeContext& c, double mix, double size, double decay_seconds);
    void phaser(RecipeContext& c, double mix, double feedback, const std::string& tempo);
    void flanger(RecipeContext& c, double mix, double feedback, const std::string& tempo);
    void chorus(RecipeContext& c, double mix);
    void distortion(RecipeContext& c, int type, double drive, double mix);
    void compressor(RecipeContext& c, double mix);

    // Cut the low rumble and soften the top a little
    void eq(RecipeContext& c, double low_cut_note, double high_gain);

    // Psytrance chain: distortion, filter, EQ, compressor, phaser, flanger, chorus, delay, reverb
    void psyChain(RecipeContext& c);

    // Delayed drifting vibrato: an LFO in hertz to the oscillator tune
    void vibrato(RecipeContext& c, double amount, const std::vector<int>& oscs);

    // Slow timbre motion: a tempo LFO to the wave position or cutoff
    int motion(RecipeContext& c, const std::string& destination, double amount, const std::string& tempo, bool bipolar);

    void random(RecipeContext& c, int n, const std::string& destination, double amount, const std::string& tempo, int style);

    void macro(RecipeContext& c, int n, const std::string& name, const std::string& destination, double amount);
  }

} // namespace gen
} // namespace corpuscular
