/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

// Preset generator (namespace corpuscular::gen), paths relative to this file.

#include "../generator/format/gen_base64.cpp"
#include "../generator/format/gen_parameters.cpp"
#include "../generator/format/gen_parameters_table.cpp"
#include "../generator/format/gen_units.cpp"
#include "../generator/format/gen_lfo_shape.cpp"
#include "../generator/format/gen_wavetable.cpp"
#include "../generator/format/gen_sample.cpp"
#include "../generator/format/gen_patch.cpp"
#include "../generator/format/gen_writer.cpp"

#include "../generator/modulation/gen_mod_matrix.cpp"

#include "../generator/wavetables/gen_wave_math.cpp"
#include "../generator/wavetables/gen_table_types.cpp"
#include "../generator/wavetables/gen_wavetable_factory.cpp"

#include "../generator/music/gen_scale_manager.cpp"
#include "../generator/music/gen_scale_manager_data.cpp"
#include "../generator/music/gen_scales.cpp"
#include "../generator/music/gen_phrase.cpp"
#include "../generator/music/gen_midi_arp_patterns.cpp"
#include "../generator/music/gen_midi_arp_patterns_data.cpp"
#include "../generator/music/gen_arp_algorithms.cpp"
#include "../generator/music/gen_sequence_encoder.cpp"
#include "../generator/music/gen_lorem_generator.cpp"

#include "../generator/recipes/gen_kit.cpp"
#include "../generator/recipes/gen_recipe_catalog.cpp"
#include "../generator/recipes/gen_lead_recipe.cpp"
#include "../generator/recipes/gen_bass_recipe.cpp"
#include "../generator/recipes/gen_arp_recipe.cpp"
#include "../generator/recipes/gen_sequence_recipe.cpp"
#include "../generator/recipes/gen_pluck_recipe.cpp"
#include "../generator/recipes/gen_stab_recipe.cpp"
#include "../generator/recipes/gen_keys_recipe.cpp"
#include "../generator/recipes/gen_pad_recipe.cpp"
#include "../generator/recipes/gen_vocal_recipe.cpp"
#include "../generator/recipes/gen_sfx_recipe.cpp"
#include "../generator/recipes/gen_percussion_recipe.cpp"

#include "../generator/gen_sanity.cpp"
#include "../generator/gen_preset_factory.cpp"
#include "../generator/preset_generator.cpp"

#include "../generator/music/gen_name_generator.cpp"
#include "../generator/wavetables/gen_table_generator.cpp"
#include "../generator/wavetables/gen_wavetable_encoders.cpp"
#include "../generator/wavetable_generator.cpp"
#include "../generator/lfo_generator.cpp"
#include "../generator/preset_mutator.cpp"
#include "../generator/skin_generator.cpp"
