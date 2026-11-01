/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>

namespace corpuscular {
namespace gen {

  struct RecipeContext;

  // Character choices learned from 1017 commercial psytrance Vital presets (tools/preset_stats):
  // which options the packs use, how often, and where their knobs sit. Recipes call these instead of
  // picking freely, so the presets lean the way the packs do and stay musical.
  namespace PsyStyle {
    // Filter blend for the analog/digital/dirty/ladder/diode models: 0 low-pass, about 1 band-pass, towards 2 high-pass
    double filterBlend(RecipeContext& c, double band_pass, double high_pass);

    // Filter model as the packs weight it for leads, sequences and arps (analog first, then digital, dirty, ladder, diode)
    int filterModel(RecipeContext& c);

    // Oscillator warp (sync, formant, bend, squeeze, pulse width) with the amount around the middle
    void oscWarp(RecipeContext& c, int osc, double chance);

    // FM from another sounding oscillator (Outerverse and Sonicspore leads and sequences, 13-25%)
    void oscFm(RecipeContext& c, int osc, double chance);

    // Spectral morph (phase disperse, formant scale, smear, harmonic and inharmonic stretch, random amplitudes, skew)
    void spectralMorph(RecipeContext& c, int osc, double chance);

    // Distortion type as the packs weight it: soft clip most, then hard clip, downsample, sine fold, linear fold
    int distortionType(RecipeContext& c);

    // Stepped random to the cutoff, the classic psy filter flicker (amount from the packs)
    void randomCutoff(RecipeContext& c, int random_n, int filter, double chance);

    // The four macros the way the producers set them for this type: performance knobs that open the delay,
    // the room, the filter, the warp, the flanger... chosen by how often the packs use them, with their
    // amounts, and only for modules that are on
    void producerMacros(RecipeContext& c, const std::string& type);
  }

} // namespace gen
} // namespace corpuscular
