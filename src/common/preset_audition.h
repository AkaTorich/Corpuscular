/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "JuceHeader.h"
#include "json/json.h"
#include "preset_generator.h"

#include <functional>
#include <memory>

class AuditionSynth;

// Plays a generated preset in a hidden synth, the way an ear would check it: one held note has to
// sound (not silent, not broken, not exploding). A preset that passes gets its volume set so its loud
// part sits at one level, like a finished bank.
class PresetAudition {
  public:
    static constexpr float kTargetDb = -16.0f;    // loud part of the note (90th percentile of 46 ms windows)
    static constexpr float kCeilingDb = -1.0f;    // highest peak after the level is set
    static constexpr float kSilentDb = -50.0f;    // below this nothing is heard

    struct Result {
      bool audible = false;
      float loud_db = -120.0f;
      float peak_db = -120.0f;
      String reason;
    };

    PresetAudition();
    ~PresetAudition();

    // Judges the preset and, when it passes, rewrites its "volume" for the common level
    Result check(nlohmann::json& preset, const std::string& type);

  private:
    std::unique_ptr<AuditionSynth> synth_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetAudition)
};

namespace corpuscular {
  // Makes presets of the request (a new seed each try) until one passes the audition; `extra` changes each
  // try before it is heard (for example switches the vocoder on). After `tries` the last one is returned.
  gen::GeneratedPreset auditionedPreset(gen::PresetRequest request, PresetAudition& audition,
                                        std::function<void(nlohmann::json&)> extra = nullptr, int tries = 8);
}
