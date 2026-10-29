/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "synth_constants.h"
#include "synth_module.h"

#include <memory>

namespace wv {
  class SynthVocoder;
  class Wavetable;
}

namespace vital {

  // Quantum Vocoder in the effect chain: the voice comes from the engine's sidechain input,
  // the synth (or a wavetable carrier) speaks it
  class VocoderModule : public SynthModule {
    public:
      VocoderModule(const Output* sidechain, const Output* keytrack);
      virtual ~VocoderModule();

      void init() override;
      void hardReset() override;
      void enable(bool enable) override;
      void setSampleRate(int sample_rate) override;
      void processWithInput(const poly_float* audio_in, int num_samples) override;
      Processor* clone() const override { return new VocoderModule(*this); }

      // Message thread, with the audio lock held by the caller
      void setTable(std::shared_ptr<const wv::Wavetable> table);
      std::shared_ptr<const wv::Wavetable> getTable() const { return table_; }
      std::shared_ptr<wv::SynthVocoder> getVocoder() const { return vocoder_; }

    protected:
      const Output* sidechain_;
      const Output* keytrack_;

      Value* carrier_;
      Value* pitch_mode_;
      Value* note_;
      Value* octave_;
      Value* unison_;
      Value* bands_;
      Value* freeze_;
      Output* mix_;
      Output* position_;
      Output* detune_;
      Output* glide_;
      Output* width_;
      Output* formant_;
      Output* attack_;
      Output* release_;
      Output* air_;
      Output* noise_;
      Output* gate_;
      Output* tilt_;
      Output* spread_;
      Output* level_;

      std::shared_ptr<wv::SynthVocoder> vocoder_;
      std::shared_ptr<const wv::Wavetable> table_;
      std::shared_ptr<std::vector<float>> voice_;
      std::shared_ptr<std::vector<float>> left_;
      std::shared_ptr<std::vector<float>> right_;

      JUCE_LEAK_DETECTOR(VocoderModule)
  };
} // namespace vital
