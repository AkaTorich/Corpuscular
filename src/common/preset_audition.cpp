/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "preset_audition.h"

#include "sound_engine.h"
#include "synth_base.h"

#include <algorithm>
#include <cmath>

// A headless synth that loads a preset and renders one held note into memory
class AuditionSynth : public HeadlessSynth {
  public:
    static constexpr int kSampleRate = 44100;
    static constexpr int kBufferSize = 64;

    bool load(const json& data) {
      try {
        return loadFromJson(data);
      }
      catch (const std::exception&) {
        return false;
      }
    }

    // Left and right mixed to one channel
    void render(int note, double held_seconds, double release_seconds, std::vector<float>& out) {
      static constexpr int kSettleSamples = kSampleRate / 5;

      ScopedLock lock(getCriticalSection());
      processModulationChanges();
      engine_->setSampleRate(kSampleRate);
      engine_->setBpm(140.0f);
      engine_->updateAllModulationSwitches();

      double sample_time = 1.0 / kSampleRate;
      double time = 0.0;
      for (int samples = 0; samples < kSettleSamples; samples += kBufferSize) {
        engine_->correctToTime(time);
        time += kBufferSize * sample_time;
        engine_->process(kBufferSize);
      }

      int held = (int)(held_seconds * kSampleRate);
      int total = held + (int)(release_seconds * kSampleRate);
      out.clear();
      out.reserve(total);
      const vital::mono_float* output = (const vital::mono_float*)engine_->output(0)->buffer;
      engine_->noteOn(note, 0.8f, 0, 0);
      for (int samples = 0; samples < total; samples += kBufferSize) {
        engine_->correctToTime(time);
        time += kBufferSize * sample_time;
        if (samples <= held && held < samples + kBufferSize)
          engine_->noteOff(note, 0.5f, 0, 0);
        engine_->process(kBufferSize);
        for (int i = 0; i < kBufferSize; ++i)
          out.push_back(0.5f * (output[vital::poly_float::kSize * i] + output[vital::poly_float::kSize * i + 1]));
      }
      engine_->allSoundsOff();
    }
};

namespace {
  float auditionDb(double amplitude) {
    return (float)(20.0 * std::log10(std::max(amplitude, 1e-6)));
  }

  // The note each type is played on, where the producers' presets of that type live
  int auditionNote(const std::string& type) {
    if (type == "Bass" || type == "Sequence")
      return 45;
    if (type == "Pad" || type == "Keys" || type == "Stab" || type == "Pluck")
      return 60;
    return 57;
  }
}

PresetAudition::PresetAudition() : synth_(std::make_unique<AuditionSynth>()) { }

PresetAudition::~PresetAudition() = default;

PresetAudition::Result PresetAudition::check(nlohmann::json& preset, const std::string& type) {
  static constexpr int kWindow = 2048;
  static constexpr float kExplodedDb = 12.0f;

  Result result;
  if (!synth_->load(preset)) {
    result.reason = "does not load";
    return result;
  }

  // Pads swell slowly: give them longer before the key is let go
  double held = type == "Pad" ? 2.6 : 1.6;
  std::vector<float> audio;
  synth_->render(auditionNote(type), held, 0.4, audio);

  double peak = 0.0;
  for (float sample : audio) {
    if (!std::isfinite(sample)) {
      result.reason = "broken (not a number)";
      return result;
    }
    peak = std::max(peak, (double)std::abs(sample));
  }

  // Loudness while the key is held: the 90th percentile of short windows, so gated patterns count by their notes
  int held_samples = std::min((int)audio.size(), (int)(held * AuditionSynth::kSampleRate));
  std::vector<float> windows;
  for (int start = 0; start + kWindow <= held_samples; start += kWindow) {
    double sum = 0.0;
    for (int i = start; i < start + kWindow; ++i)
      sum += audio[i] * audio[i];
    windows.push_back(auditionDb(std::sqrt(sum / kWindow)));
  }
  std::sort(windows.begin(), windows.end());
  // Sounds that die away by themselves are judged by their loudest moment, held ones by most of the note
  bool decaying = type == "Percussion" || type == "Pluck" || type == "Stab" || type == "Keys";
  if (windows.empty())
    result.loud_db = -120.0f;
  else
    result.loud_db = decaying ? windows.back() : windows[(windows.size() * 9) / 10];
  result.peak_db = auditionDb(peak);

  if (result.peak_db > kExplodedDb) {
    result.reason = "explodes";
    return result;
  }
  if (result.loud_db < kSilentDb) {
    result.reason = "silent";
    return result;
  }

  // Common level: the loud part to the target, the peak under the ceiling, within the volume knob's range
  json& settings = preset["settings"];
  double volume = settings.count("volume") ? settings["volume"].get<double>() : 5473.0;
  double volume_db = std::sqrt(std::max(0.0, volume)) - 80.0;
  double change = std::min(kTargetDb - result.loud_db, kCeilingDb - result.peak_db);
  double new_db = std::max(-40.0, std::min(4.0, volume_db + change));
  settings["volume"] = (new_db + 80.0) * (new_db + 80.0);

  // Even at full volume it stays a whisper: not a usable preset
  if (result.loud_db + (new_db - volume_db) < kSilentDb + 15.0f) {
    result.reason = "too quiet";
    return result;
  }

  result.audible = true;
  return result;
}

namespace corpuscular {
  gen::GeneratedPreset auditionedPreset(gen::PresetRequest request, PresetAudition& audition,
                                        std::function<void(nlohmann::json&)> extra, int tries) {
    gen::GeneratedPreset preset;
    Random random((int64)request.seed);
    for (int attempt = 0; attempt < tries; ++attempt) {
      if (attempt > 0)
        request.seed = static_cast<uint32_t>(random.nextInt());
      preset = gen::createPreset(request);
      if (extra)
        extra(preset.data);
      if (audition.check(preset.data, preset.type).audible)
        break;
    }
    return preset;
  }
}
