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

using json = nlohmann::json;

// The ARP tab's sequencer. It sits in front of the synth: a held note (from the DAW, a MIDI keyboard or the
// on-screen keys) becomes the tonic and the pattern plays from it into the synth itself. In a DAW that plays
// it follows the host tempo and bar grid, otherwise it runs from the moment the note is pressed.
// When it is off the MIDI passes through untouched.
class ArpSequencer {
  public:
    struct Note {
      int midi = 60;
      int start = 0;        // in steps (a step is a sixteenth at rate 1/16)
      int length = 1;
      int velocity = 100;
    };

    enum Rate { k32nd, k16thTriplet, k16th, k8thTriplet, k8th, kQuarter, kNumRates };
    static constexpr int kMaxSteps = 16 * 64;
    // Saved arps: Documents\Corpuscular\Arps\*.corparp
    static constexpr const char* kFolderName = "Arps";
    static constexpr const char* kExtension = "corparp";
    static constexpr int kMaxNotes = 2048;

    static String rateName(int rate);

    ArpSequencer();

    // Message thread
    void setPattern(const std::vector<Note>& notes, int length, int root);
    std::vector<Note> getNotes() const;
    int getLength() const;
    int getRoot() const;

    void setEnabled(bool enabled) { enabled_.store(enabled); }
    bool isEnabled() const { return enabled_.load(); }
    void setLatch(bool latch) { latch_.store(latch); }
    bool isLatched() const { return latch_.load(); }
    void setRate(int rate);
    int getRate() const { return rate_.load(); }
    void setOctave(int octave) { octave_.store(jlimit(-3, 3, octave)); }
    int getOctave() const { return octave_.load(); }
    void setGate(float gate);
    float getGate() const { return gate_.load(); }

    // Pattern position in steps while it sounds, -1 when silent
    float getPlayhead() const { return playhead_.load(); }

    // Free-form settings of the ARP tab (mode, key, scale…) kept with the pattern
    void setUiState(const json& state);
    json getUiState() const;

    json toJson() const;
    void fromJson(const json& data);

    // Message thread: the pattern as it plays (rate, gate, octave) as a standard MIDI file,
    // one track, the clip as long as the pattern
    bool writeMidiFile(const File& file, const String& track_name) const;
    // A MIDI file (all tracks) as a pattern on the step grid of `rate`; the length is whole bars
    static bool readMidiFile(const File& file, int rate, std::vector<Note>& notes, int& length);

    // Audio thread: rewrites `midi` (note input → generated notes, other messages kept)
    void process(MidiBuffer& midi, int num_samples, double sample_rate, double bpm,
                 bool host_playing, double host_ppq);

  private:
    struct Event {
      double time;
      bool on;
      int note;
      int velocity;
    };

    void rebuildSchedule();
    void flush(MidiBuffer& out, int sample);
    static double rateFactor(int rate);

    mutable CriticalSection lock_;
    std::vector<Note> notes_;
    std::vector<Event> schedule_;
    int length_;
    int root_;
    json ui_state_;

    std::atomic<bool> enabled_;
    std::atomic<bool> latch_;
    std::atomic<int> rate_;
    std::atomic<int> octave_;
    std::atomic<float> gate_;
    std::atomic<float> playhead_;
    std::atomic<bool> pattern_changed_;

    // Audio thread state
    MidiBuffer output_;
    std::vector<int> held_;
    int latched_tonic_;
    int previous_transpose_;
    bool was_active_;
    bool was_enabled_;
    double free_position_;
    double expected_host_position_;
    bool active_notes_[128];
};
