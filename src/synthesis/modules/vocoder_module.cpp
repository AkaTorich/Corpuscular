/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "vocoder_module.h"

#include "qv_synth_vocoder.h"

namespace vital {

  VocoderModule::VocoderModule(const Output* sidechain, const Output* keytrack) :
      SynthModule(0, 1), sidechain_(sidechain), keytrack_(keytrack),
      carrier_(nullptr), pitch_mode_(nullptr), note_(nullptr), octave_(nullptr), unison_(nullptr), bands_(nullptr),
      freeze_(nullptr), mix_(nullptr), position_(nullptr), detune_(nullptr), glide_(nullptr), width_(nullptr),
      formant_(nullptr), attack_(nullptr), release_(nullptr), air_(nullptr), noise_(nullptr), gate_(nullptr),
      tilt_(nullptr), spread_(nullptr), level_(nullptr) {
    // Room for the largest oversampled block, so the audio thread never allocates
    static constexpr int kMaxSamples = kMaxBufferSize * 16;
    vocoder_ = std::make_shared<wv::SynthVocoder>();
    voice_ = std::make_shared<std::vector<float>>(kMaxSamples);
    left_ = std::make_shared<std::vector<float>>(kMaxSamples);
    right_ = std::make_shared<std::vector<float>>(kMaxSamples);
  }

  VocoderModule::~VocoderModule() { }

  void VocoderModule::init() {
    carrier_ = createBaseControl("vocoder_carrier");
    pitch_mode_ = createBaseControl("vocoder_pitch_mode");
    note_ = createBaseControl("vocoder_note");
    octave_ = createBaseControl("vocoder_octave");
    unison_ = createBaseControl("vocoder_unison");
    bands_ = createBaseControl("vocoder_bands");
    freeze_ = createBaseControl("vocoder_freeze");

    mix_ = createMonoModControl("vocoder_dry_wet");
    position_ = createMonoModControl("vocoder_position");
    detune_ = createMonoModControl("vocoder_detune");
    glide_ = createMonoModControl("vocoder_glide");
    width_ = createMonoModControl("vocoder_width");
    formant_ = createMonoModControl("vocoder_formant");
    attack_ = createMonoModControl("vocoder_attack");
    release_ = createMonoModControl("vocoder_release");
    air_ = createMonoModControl("vocoder_air");
    noise_ = createMonoModControl("vocoder_noise");
    gate_ = createMonoModControl("vocoder_gate");
    tilt_ = createMonoModControl("vocoder_tilt");
    spread_ = createMonoModControl("vocoder_spread");
    level_ = createMonoModControl("vocoder_level");

    // Until a table is chosen the carrier reads the first built-in one
    table_ = std::shared_ptr<const wv::Wavetable>(wv::Wavetable::builtIn(0).release());
    vocoder_->setTable(table_.get());

    SynthModule::init();
  }

  void VocoderModule::setTable(std::shared_ptr<const wv::Wavetable> table) {
    if (table == nullptr)
      return;
    table_ = table;
    vocoder_->setTable(table_.get());
  }

  void VocoderModule::hardReset() {
    vocoder_->reset();
  }

  void VocoderModule::enable(bool enable) {
    SynthModule::enable(enable);
    process(1);
    if (enable)
      vocoder_->reset();
  }

  void VocoderModule::setSampleRate(int sample_rate) {
    SynthModule::setSampleRate(sample_rate);
    vocoder_->prepare(sample_rate);
  }

  void VocoderModule::processWithInput(const poly_float* audio_in, int num_samples) {
    SynthModule::process(num_samples);

    if ((int)voice_->size() < num_samples) {
      voice_->resize(num_samples);
      left_->resize(num_samples);
      right_->resize(num_samples);
    }

    // Lanes hold (left, right, left, right); the voice is the mono sum of the sidechain
    const poly_float* sidechain = sidechain_->buffer;
    for (int i = 0; i < num_samples; ++i) {
      (*left_)[i] = audio_in[i][0];
      (*right_)[i] = audio_in[i][1];
      (*voice_)[i] = 0.5f * (sidechain[i][0] + sidechain[i][1]);
    }

    wv::SynthVocoderParams params;
    params.tableCarrier = carrier_->value() >= 0.5f;
    params.pitchMode = (int)std::round(pitch_mode_->value());
    params.note = note_->value();
    params.octave = (int)std::round(octave_->value());
    params.unison = (int)std::round(unison_->value());
    params.bands = (int)std::round(bands_->value());
    params.freeze = freeze_->value() >= 0.5f;
    params.mix = mix_->buffer[0][0];
    params.position = position_->buffer[0][0];
    params.detuneCents = detune_->buffer[0][0];
    params.glideMs = glide_->buffer[0][0];
    params.width = width_->buffer[0][0];
    params.formantOct = formant_->buffer[0][0];
    params.attackMs = attack_->buffer[0][0];
    params.releaseMs = release_->buffer[0][0];
    params.air = air_->buffer[0][0];
    params.noise = noise_->buffer[0][0];
    params.gateDb = gate_->buffer[0][0];
    params.tilt = tilt_->buffer[0][0];
    params.spread = spread_->buffer[0][0];
    params.outputDb = level_->buffer[0][0];

    // The last played note, from the voice handler's offset to middle C
    float key_note = keytrack_ ? kMidiTrackCenter + keytrack_->buffer[0][0] : -1.0f;

    vocoder_->process(voice_->data(), left_->data(), right_->data(), num_samples, params, key_note);

    poly_float* audio_out = output()->buffer;
    for (int i = 0; i < num_samples; ++i) {
      float left = (*left_)[i];
      float right = (*right_)[i];
      audio_out[i] = poly_float(left, right, left, right);
    }
  }
} // namespace vital
