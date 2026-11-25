/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "retrigger_test.h"

#include "synth_base.h"
#include "sound_engine.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
  constexpr int kRate = 44100;
  constexpr int kBlock = vital::kMaxBufferSize;
  constexpr int kNote = 60;
  constexpr float kVelocity = 0.7f;
  constexpr int kLoop = kRate / 2;
  constexpr int kRepeats = 8;
  constexpr int kStart = 300;
  constexpr int kWindow = kRate / 10;

  enum Order { kOffThenOn, kOnThenOff, kGap };

  struct Event {
    int time;
    bool on;
  };

  struct Metrics {
    double overshoot = 0.0;
    double rms_max = 0.0;
    double rms_min = 0.0;
    double jump = 0.0;
  };

  double rmsOf(const std::vector<float>& x, int from, int length) {
    double sum = 0.0;
    for (int i = from; i < from + length; ++i)
      sum += x[i] * x[i];
    return std::sqrt(sum / length);
  }

  double peakOf(const std::vector<float>& x, int from, int length) {
    double peak = 0.0;
    for (int i = from; i < from + length; ++i)
      peak = std::max(peak, (double)std::abs(x[i]));
    return peak;
  }

  double jumpOf(const std::vector<float>& x, int from, int length) {
    double jump = 0.0;
    for (int i = std::max(1, from); i < from + length; ++i)
      jump = std::max(jump, (double)std::abs(x[i] - x[i - 1]));
    return jump;
  }

  Metrics measure(const std::vector<float>& x, int point) {
    int steady = point + kLoop / 2;
    double steady_peak = std::max(peakOf(x, steady, kWindow), 1e-9);
    double steady_rms = std::max(rmsOf(x, steady, kWindow), 1e-9);
    double steady_jump = std::max(jumpOf(x, steady, kWindow), 1e-9);

    Metrics result;
    result.overshoot = peakOf(x, point, kWindow) / steady_peak;
    result.jump = jumpOf(x, point - kRate / 200, kRate / 50) / steady_jump;

    int rms_length = kRate / 200;
    result.rms_min = 1e9;
    for (int i = point + kRate / 100; i < point + 3 * kWindow / 2; i += kRate / 1000) {
      double rms = rmsOf(x, i, rms_length) / steady_rms;
      result.rms_max = std::max(result.rms_max, rms);
      result.rms_min = std::min(result.rms_min, rms);
    }
    return result;
  }

  std::vector<float> render(int voices, Order order, int& max_voices) {
    HeadlessSynth synth;
    synth.loadInitPreset();
    synth.valueChanged("polyphony", voices);

    vital::SoundEngine* engine = synth.getEngine();
    engine->setSampleRate(kRate);
    engine->setBpm(120.0f);
    engine->updateAllModulationSwitches();

    double sample_time = 1.0 / kRate;
    double time = -1.0;
    for (int i = 0; i < kRate; i += kBlock) {
      engine->correctToTime(time);
      time += kBlock * sample_time;
      engine->process(kBlock);
    }

    std::vector<Event> events;
    events.push_back({ kStart, true });
    for (int k = 1; k < kRepeats; ++k) {
      int point = kStart + k * kLoop;
      if (order == kOffThenOn) {
        events.push_back({ point, false });
        events.push_back({ point, true });
      }
      else if (order == kOnThenOff) {
        events.push_back({ point, true });
        events.push_back({ point, false });
      }
      else {
        events.push_back({ point - kRate / 100, false });
        events.push_back({ point, true });
      }
    }
    events.push_back({ kStart + kRepeats * kLoop, false });

    int total = kStart + (kRepeats + 1) * kLoop;
    std::vector<float> out(total + kBlock, 0.0f);
    const vital::mono_float* engine_out = (const vital::mono_float*)engine->output(0)->buffer;
    size_t next_event = 0;
    max_voices = 0;
    for (int block = 0; block < total; block += kBlock) {
      while (next_event < events.size() && events[next_event].time < block + kBlock) {
        const Event& event = events[next_event++];
        if (event.on)
          engine->noteOn(kNote, kVelocity, event.time - block, 0);
        else
          engine->noteOff(kNote, 0.5f, event.time - block, 0);
      }
      engine->correctToTime(time);
      time += kBlock * sample_time;
      engine->process(kBlock);
      max_voices = std::max(max_voices, engine->getNumActiveVoices());
      for (int i = 0; i < kBlock; ++i)
        out[block + i] = engine_out[vital::poly_float::kSize * i];
    }
    out.resize(total);
    return out;
  }

  void writeWav(const File& file, const std::vector<float>& samples) {
    file.deleteFile();
    std::unique_ptr<FileOutputStream> stream = file.createOutputStream();
    if (stream == nullptr)
      return;
    WavAudioFormat format;
    std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(stream.get(), kRate, 1, 24, {}, 0));
    if (writer == nullptr)
      return;
    stream.release();
    const float* channels[1] = { samples.data() };
    writer->writeFromFloatArrays(channels, 1, (int)samples.size());
  }

  // A held note, a preset loaded in the middle of it: the level after the load against the level before
  double renderPresetSwitch(const File& folder, bool host_playing) {
    HeadlessSynth synth;
    synth.loadInitPreset();
    synth.setHostPlaying(host_playing);
    File preset = folder.getChildFile("switch_preset.vital");
    preset.replaceWithText(synth.getStateJson().dump());

    vital::SoundEngine* engine = synth.getEngine();
    engine->setSampleRate(kRate);
    engine->setBpm(120.0f);
    engine->updateAllModulationSwitches();

    const vital::mono_float* engine_out = (const vital::mono_float*)engine->output(0)->buffer;
    std::vector<float> out;
    double time = 0.0;
    auto run = [&](int samples) {
      for (int i = 0; i < samples; i += kBlock) {
        engine->correctToTime(time);
        time += kBlock * (1.0 / kRate);
        engine->process(kBlock);
        for (int s = 0; s < kBlock; ++s)
          out.push_back(engine_out[vital::poly_float::kSize * s]);
      }
    };

    run(kRate / 10);
    engine->noteOn(kNote, kVelocity, 0, 0);
    run(kLoop);
    double before = std::max(rmsOf(out, (int)out.size() - kWindow, kWindow), 1e-9);
    std::string error;
    synth.loadFromFile(preset, error);
    run(kLoop);
    double after = rmsOf(out, (int)out.size() - kWindow, kWindow);
    preset.deleteFile();
    writeWav(folder.getChildFile(host_playing ? "switch_daw_playing.wav" : "switch_daw_stopped.wav"), out);
    return after / before;
  }
}

void runRetriggerTest(const StringArray& args) {
  String folder_name;
  int index = args.indexOf("--retrigger-test");
  if (index >= 0 && index + 1 < args.size() && !args[index + 1].startsWith("-"))
    folder_name = args[index + 1];
  File folder = File::getCurrentWorkingDirectory();
  if (folder_name.isNotEmpty())
    folder = folder.getChildFile(folder_name);
  folder.createDirectory();

  struct Case { int voices; Order order; const char* name; };
  const Case cases[] = {
    { 1, kOffThenOn, "voices1_off_on" },
    { 1, kOnThenOff, "voices1_on_off" },
    { 1, kGap, "voices1_gap10ms" },
    { 8, kOffThenOn, "voices8_off_on" },
    { 8, kOnThenOff, "voices8_on_off" },
    { 8, kGap, "voices8_gap10ms" },
  };

  std::cout << "case              voices  first: over rmsmax jump | repeats: over rmsmax rmsmin jump (worst)" << std::endl;
  for (const Case& c : cases) {
    int max_voices = 0;
    std::vector<float> samples = render(c.voices, c.order, max_voices);
    writeWav(folder.getChildFile(String(c.name) + ".wav"), samples);

    Metrics first = measure(samples, kStart);
    Metrics worst;
    worst.rms_min = 1e9;
    for (int k = 1; k < kRepeats; ++k) {
      Metrics m = measure(samples, kStart + k * kLoop);
      worst.overshoot = std::max(worst.overshoot, m.overshoot);
      worst.rms_max = std::max(worst.rms_max, m.rms_max);
      worst.rms_min = std::min(worst.rms_min, m.rms_min);
      worst.jump = std::max(worst.jump, m.jump);
    }

    std::cout << String(c.name).paddedRight(' ', 18) << String(max_voices).paddedRight(' ', 8)
              << String(first.overshoot, 2) << " " << String(first.rms_max, 2) << " " << String(first.jump, 2)
              << "   |   " << String(worst.overshoot, 2) << " " << String(worst.rms_max, 2) << " "
              << String(worst.rms_min, 2) << " " << String(worst.jump, 2) << std::endl;
  }

  // Preset switch under a held note: while the DAW plays the note goes on (near 1.00), stopped it is cut (0.00)
  std::cout << "preset switch, held note, level after/before: DAW playing "
            << String(renderPresetSwitch(folder, true), 2) << ", DAW stopped "
            << String(renderPresetSwitch(folder, false), 2) << std::endl;
}
