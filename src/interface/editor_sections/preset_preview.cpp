/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "preset_preview.h"

#include "synth_base.h"

namespace {
  constexpr int kMidiChannel = 1;
  constexpr int kLowNote = 45;
  constexpr int kMiddleNote = 57;
  constexpr int kHighNote = 60;

  bool startsWithWord(const String& name, const char* prefix) {
    String upper = name.toUpperCase();
    if (!upper.startsWith(prefix))
      return false;
    int length = static_cast<int>(strlen(prefix));
    return upper.length() == length || !CharacterFunctions::isLetter(upper[length]);
  }
}

PresetPreview::PresetPreview(SynthBase* synth) : synth_(synth), note_(-1) { }

PresetPreview::~PresetPreview() {
  stopTimer();
}

int PresetPreview::noteForPreset() const {
  String style = synth_->getStyle().toLowerCase();
  String name = synth_->getPresetName();

  if (style.contains("bass") || style.contains("sequence") ||
      startsWithWord(name, "BS") || startsWithWord(name, "BASS") || startsWithWord(name, "SQ") ||
      startsWithWord(name, "SEQ"))
    return kLowNote;
  if (style.contains("pad") || style.contains("keys") || style.contains("pluck") || style.contains("stab") ||
      startsWithWord(name, "PD") || startsWithWord(name, "PAD") || startsWithWord(name, "KY") ||
      startsWithWord(name, "KEYS") || startsWithWord(name, "PL") || startsWithWord(name, "PLUCK") ||
      startsWithWord(name, "ST") || startsWithWord(name, "STAB"))
    return kHighNote;
  return kMiddleNote;
}

void PresetPreview::play() {
  MidiKeyboardState* keyboard = synth_->getKeyboardState();
  if (keyboard == nullptr)
    return;

  // The load already silenced the voices; this only clears the key the last preview still holds
  if (note_ >= 0)
    keyboard->noteOff(kMidiChannel, note_, 0.0f);

  note_ = noteForPreset();
  keyboard->noteOn(kMidiChannel, note_, kVelocity);
  startTimer(kPlayMilliseconds);
}

void PresetPreview::stop() {
  stopTimer();
  MidiKeyboardState* keyboard = synth_->getKeyboardState();
  if (keyboard && note_ >= 0)
    keyboard->noteOff(kMidiChannel, note_, 0.0f);
  note_ = -1;
}

void PresetPreview::timerCallback() {
  stop();
}
