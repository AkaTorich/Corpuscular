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

class SynthBase;

// Preview on preset switch: the moment a preset is picked in the browser, by the arrows or in the generator,
// one note of it sounds for five seconds. Loading cuts the previous sound at once (the engine silences all
// voices on load), so switching quickly goes from sound to sound without a tail.
class PresetPreview : public Timer {
  public:
    static constexpr int kPlayMilliseconds = 5000;
    static constexpr float kVelocity = 0.8f;

    PresetPreview(SynthBase* synth);
    ~PresetPreview();

    // Call right after a preset has been loaded
    void play();
    void stop();

    void timerCallback() override;

  private:
    // The note the sound is made for: low for basses and sequences, middle C for pads, keys, plucks, stabs
    int noteForPreset() const;

    SynthBase* synth_;
    int note_;
};
