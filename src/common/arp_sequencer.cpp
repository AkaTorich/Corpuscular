/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "arp_sequencer.h"

#include <algorithm>
#include <map>

namespace {
  constexpr int kArpDefaultLength = 16;
  constexpr int kArpDefaultRoot = 60;
}

String ArpSequencer::rateName(int rate) {
  static const char* kNames[] = { "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4" };
  return kNames[jlimit(0, kNumRates - 1, rate)];
}

double ArpSequencer::rateFactor(int rate) {
  static const double kFactors[] = { 0.5, 2.0 / 3.0, 1.0, 4.0 / 3.0, 2.0, 4.0 };
  return kFactors[jlimit(0, kNumRates - 1, rate)];
}

ArpSequencer::ArpSequencer() : length_(kArpDefaultLength), root_(kArpDefaultRoot), ui_state_(json::object()),
                               enabled_(false), latch_(false), rate_(k16th), octave_(0), gate_(0.85f),
                               playhead_(-1.0f), pattern_changed_(false), length_mod_(1.0f), start_mod_(0.0f), latched_tonic_(-1), previous_transpose_(0), was_active_(false),
                               was_enabled_(false), free_position_(0.0), expected_host_position_(-1.0),
                               schedule_length_(kArpDefaultLength), loop_origin_(0.0), previous_length_(kArpDefaultLength) {
  output_.ensureSize(8192);
  held_.reserve(128);
  pending_offs_.reserve(kMaxNotes);
  std::fill(std::begin(active_notes_), std::end(active_notes_), false);

  // Until something is generated: a plain tonic-fifth-octave figure
  std::vector<Note> notes;
  const int figure[] = { 0, 7, 12, 7 };
  for (int i = 0; i < 16; ++i)
    notes.push_back({ kArpDefaultRoot + figure[i % 4], i, 1, 100 });
  setPattern(notes, kArpDefaultLength, kArpDefaultRoot);
}

void ArpSequencer::setPattern(const std::vector<Note>& notes, int length, int root) {
  ScopedLock lock(lock_);
  notes_.clear();
  for (const Note& note : notes) {
    if ((int)notes_.size() >= kMaxNotes)
      break;
    Note clean = note;
    clean.midi = jlimit(0, 127, clean.midi);
    clean.start = jlimit(0, kMaxSteps - 1, clean.start);
    clean.length = jlimit(1, kMaxSteps, clean.length);
    clean.velocity = jlimit(1, 127, clean.velocity);
    notes_.push_back(clean);
  }
  length_ = jlimit(1, kMaxSteps, length);
  root_ = jlimit(0, 127, root);
  rebuildSchedule(modulatedLength(length_));
  // Replaced while playing: the old pattern's sounding notes get their note offs on the next block
  pattern_changed_.store(true);
}

std::vector<ArpSequencer::Note> ArpSequencer::getNotes() const {
  ScopedLock lock(lock_);
  return notes_;
}

int ArpSequencer::getLength() const {
  ScopedLock lock(lock_);
  return length_;
}

int ArpSequencer::getModulatedLength() const {
  ScopedLock lock(lock_);
  return modulatedLength(length_);
}

int ArpSequencer::getModulatedStart() const {
  ScopedLock lock(lock_);
  return modulatedStart(modulatedLength(length_));
}

int ArpSequencer::modulatedStart(int length) const {
  float mod = start_mod_.load();
  if (mod <= 0.0f || length <= 1)
    return 0;
  return (int)std::round(mod * length) % length;
}

int ArpSequencer::modulatedLength(int length) const {
  float mod = length_mod_.load();
  if (mod >= 1.0f)
    return length;
  return jlimit(1, length, (int)std::round(length * mod));
}

int ArpSequencer::getRoot() const {
  ScopedLock lock(lock_);
  return root_;
}

void ArpSequencer::setRate(int rate) {
  rate_.store(jlimit(0, kNumRates - 1, rate));
}

void ArpSequencer::setGate(float gate) {
  ScopedLock lock(lock_);
  gate_.store(jlimit(0.05f, 1.0f, gate));
  rebuildSchedule(schedule_length_);
}

void ArpSequencer::setUiState(const json& state) {
  ScopedLock lock(lock_);
  ui_state_ = state.is_object() ? state : json::object();
}

json ArpSequencer::getUiState() const {
  ScopedLock lock(lock_);
  return ui_state_;
}

// Called under lock_: note on/off times in steps for a pattern `length` steps long; notes past its end are left out,
// a note never overlaps the next note of the same pitch
void ArpSequencer::rebuildSchedule(int length) {
  schedule_.clear();
  schedule_length_ = length;
  std::vector<Note> notes;
  for (const Note& note : notes_) {
    if (note.start < length)
      notes.push_back(note);
  }

  std::map<int, std::vector<int>> by_pitch;
  for (int i = 0; i < (int)notes.size(); ++i)
    by_pitch[notes[i].midi].push_back(i);
  for (auto& pitch : by_pitch) {
    std::vector<int>& indices = pitch.second;
    std::sort(indices.begin(), indices.end(), [&](int a, int b) { return notes[a].start < notes[b].start; });
    for (size_t j = 0; j + 1 < indices.size(); ++j) {
      Note& current = notes[indices[j]];
      const Note& next = notes[indices[j + 1]];
      if (current.start + current.length > next.start)
        current.length = std::max(1, next.start - current.start);
    }
  }

  float gate = gate_.load();
  for (const Note& note : notes) {
    double end = note.start + std::max(0.05, std::min<double>(note.length, length - note.start) * gate);
    schedule_.push_back({ (double)note.start, true, note.midi, note.velocity, end - note.start });
    schedule_.push_back({ end, false, note.midi, 0, 0.0 });
  }
  std::sort(schedule_.begin(), schedule_.end(), [](const Event& a, const Event& b) {
    if (a.time != b.time)
      return a.time < b.time;
    return (a.on ? 1 : 0) < (b.on ? 1 : 0);
  });
}

bool ArpSequencer::writeMidiFile(const File& file, const String& track_name) const {
  static constexpr int kTicksPerQuarter = 960;

  MidiMessageSequence track;
  int length_ticks = 0;
  {
    ScopedLock lock(lock_);
    // A step is a sixteenth times the rate factor
    double ticks_per_step = kTicksPerQuarter * rateFactor(rate_.load()) / 4.0;
    int transpose = 12 * octave_.load();
    for (const Event& event : schedule_) {
      int note = jlimit(0, 127, event.note + transpose);
      double time = std::round(event.time * ticks_per_step);
      if (event.on)
        track.addEvent(MidiMessage::noteOn(1, note, (uint8)jlimit(1, 127, event.velocity)), time);
      else
        track.addEvent(MidiMessage::noteOff(1, note), time);
    }
    length_ticks = roundToInt(length_ * ticks_per_step);
  }

  track.addEvent(MidiMessage::textMetaEvent(3, track_name), 0.0);
  track.addEvent(MidiMessage::timeSignatureMetaEvent(4, 4), 0.0);
  track.addEvent(MidiMessage::endOfTrack(), length_ticks);
  track.sort();
  track.updateMatchedPairs();

  MidiFile midi;
  midi.setTicksPerQuarterNote(kTicksPerQuarter);
  midi.addTrack(track);

  file.deleteFile();
  FileOutputStream stream(file);
  if (!stream.openedOk())
    return false;
  return midi.writeTo(stream, 1);
}

bool ArpSequencer::readMidiFile(const File& file, int rate, std::vector<Note>& notes, int& length) {
  static constexpr int kStepsPerBar = 16;

  FileInputStream stream(file);
  MidiFile midi;
  if (!stream.openedOk() || !midi.readFrom(stream) || midi.getTimeFormat() <= 0)
    return false;

  double ticks_per_step = midi.getTimeFormat() * rateFactor(rate) / 4.0;
  double end_ticks = 0.0;
  notes.clear();
  for (int t = 0; t < midi.getNumTracks(); ++t) {
    MidiMessageSequence track(*midi.getTrack(t));
    track.updateMatchedPairs();
    for (int i = 0; i < track.getNumEvents(); ++i) {
      const MidiMessage& message = track.getEventPointer(i)->message;
      double time = message.getTimeStamp();
      if (message.isEndOfTrackMetaEvent())
        end_ticks = std::max(end_ticks, time);
      if (!message.isNoteOn() || (int)notes.size() >= kMaxNotes)
        continue;

      double off = track.getTimeOfMatchingKeyUp(i);
      int start = roundToInt(time / ticks_per_step);
      int steps = off > time ? std::max(1, roundToInt((off - time) / ticks_per_step)) : 1;
      if (start >= kMaxSteps)
        continue;
      notes.push_back({ message.getNoteNumber(), start, steps, message.getVelocity() });
      end_ticks = std::max(end_ticks, off > time ? off : time + ticks_per_step);
    }
  }
  if (notes.empty())
    return false;

  // Whole bars that hold every note and the clip's own end
  int end_steps = (int)std::ceil(end_ticks / ticks_per_step - 1e-6);
  length = jlimit(kStepsPerBar, kMaxSteps, ((end_steps + kStepsPerBar - 1) / kStepsPerBar) * kStepsPerBar);
  return true;
}

json ArpSequencer::toJson() const {
  ScopedLock lock(lock_);
  json notes = json::array();
  for (const Note& note : notes_)
    notes.push_back({ note.midi, note.start, note.length, note.velocity });

  json data;
  data["enabled"] = enabled_.load();
  data["latch"] = latch_.load();
  data["rate"] = rate_.load();
  data["octave"] = octave_.load();
  data["gate"] = gate_.load();
  data["length"] = length_;
  data["root"] = root_;
  data["notes"] = notes;
  data["ui"] = ui_state_;
  return data;
}

void ArpSequencer::fromJson(const json& data) {
  if (!data.is_object())
    return;

  std::vector<Note> notes;
  if (data.count("notes") && data["notes"].is_array()) {
    for (const json& item : data["notes"]) {
      if (item.is_array() && item.size() >= 3) {
        Note note;
        note.midi = item[0].get<int>();
        note.start = item[1].get<int>();
        note.length = item[2].get<int>();
        note.velocity = item.size() > 3 ? item[3].get<int>() : 100;
        notes.push_back(note);
      }
    }
  }

  if (data.count("enabled"))
    enabled_.store(data["enabled"].get<bool>());
  if (data.count("latch"))
    latch_.store(data["latch"].get<bool>());
  if (data.count("rate"))
    setRate(data["rate"].get<int>());
  if (data.count("octave"))
    setOctave(data["octave"].get<int>());
  if (data.count("gate"))
    gate_.store(jlimit(0.05f, 1.0f, data["gate"].get<float>()));
  if (data.count("ui"))
    setUiState(data["ui"]);

  int length = data.count("length") ? data["length"].get<int>() : kArpDefaultLength;
  int root = data.count("root") ? data["root"].get<int>() : kArpDefaultRoot;
  setPattern(notes, length, root);
}

void ArpSequencer::flush(MidiBuffer& out, int sample) {
  pending_offs_.clear();
  for (int note = 0; note < 128; ++note) {
    if (active_notes_[note]) {
      out.addEvent(MidiMessage::noteOff(1, note), sample);
      active_notes_[note] = false;
    }
  }
}

void ArpSequencer::process(MidiBuffer& midi, int num_samples, double sample_rate, double bpm,
                           bool host_playing, double host_ppq) {
  bool enabled = enabled_.load();
  if (!enabled) {
    if (was_enabled_) {
      // Turned off: silence what the pattern still holds, then pass everything through as before
      output_.clear();
      flush(output_, 0);
      for (const MidiMessageMetadata message : midi)
        output_.addEvent(message.getMessage(), message.samplePosition);
      midi.swapWith(output_);
      held_.clear();
      latched_tonic_ = -1;
      was_active_ = false;
      was_enabled_ = false;
      playhead_.store(-1.0f);
    }
    return;
  }
  was_enabled_ = true;

  // Notes in are the tonic; everything else goes on to the synth
  output_.clear();
  for (const MidiMessageMetadata message : midi) {
    MidiMessage m = message.getMessage();
    if (m.isNoteOn()) {
      held_.erase(std::remove(held_.begin(), held_.end(), m.getNoteNumber()), held_.end());
      held_.push_back(m.getNoteNumber());
      // Remembered at once: a short tap can press and release within one block
      latched_tonic_ = m.getNoteNumber();
    }
    else if (m.isNoteOff())
      held_.erase(std::remove(held_.begin(), held_.end(), m.getNoteNumber()), held_.end());
    else if (m.isAllNotesOff() || m.isAllSoundOff()) {
      held_.clear();
      latched_tonic_ = -1;
      output_.addEvent(m, message.samplePosition);
    }
    else
      output_.addEvent(m, message.samplePosition);
  }

  if (!held_.empty())
    latched_tonic_ = held_.back();
  else if (!latch_.load())
    latched_tonic_ = -1;

  ScopedLock lock(lock_);
  int modulated = modulatedLength(length_);
  if (modulated != schedule_length_)
    rebuildSchedule(modulated);
  int tonic = held_.empty() ? latched_tonic_ : held_.back();
  bool active = tonic >= 0 && !schedule_.empty() && length_ > 0 && bpm > 0.0 && sample_rate > 0.0;
  if (!active) {
    if (was_active_)
      flush(output_, 0);
    was_active_ = false;
    expected_host_position_ = -1.0;
    playhead_.store(-1.0f);
    midi.swapWith(output_);
    return;
  }

  int transpose = tonic - root_ + 12 * octave_.load();
  bool pattern_changed = pattern_changed_.exchange(false);
  if (was_active_ && (transpose != previous_transpose_ || pattern_changed))
    flush(output_, 0);
  previous_transpose_ = transpose;

  double factor = rateFactor(rate_.load());
  double samples_per_step = (60.0 / bpm / 4.0) * factor * sample_rate;
  double block_steps = num_samples / samples_per_step;

  double block_start;
  if (host_playing) {
    // Locked to the host's bar grid; a jump or loop in the host silences what was playing
    block_start = host_ppq * 4.0 / factor;
    if (was_active_ && expected_host_position_ >= 0.0 && std::abs(block_start - expected_host_position_) > 0.01)
      flush(output_, 0);
    expected_host_position_ = block_start + block_steps;
  }
  else {
    if (!was_active_)
      free_position_ = 0.0;
    block_start = free_position_;
    expected_host_position_ = -1.0;
  }
  double block_end = block_start + block_steps;

  // A modulated length: the loop end moves, the position in the pattern runs on and wraps at the new end.
  // Not modulated, the pattern sits on the bar grid as before.
  if (!was_active_ || length_mod_.load() >= 1.0f)
    loop_origin_ = 0.0;
  else if (modulated != previous_length_) {
    double phase = std::fmod(std::max(0.0, block_start - loop_origin_), (double)previous_length_);
    if (phase >= modulated)
      phase = std::fmod(phase, (double)modulated);
    loop_origin_ = block_start - phase;
  }
  previous_length_ = modulated;
  double length = modulated;

  // "arp_pattern_start" shifts where in the pattern it plays from (whole steps)
  int start = modulatedStart(modulated);
  double origin = loop_origin_ - start;
  auto sampleAt = [&](double time) {
    return (int)jlimit(0.0, num_samples - 1.0, (time - block_start) * samples_per_step);
  };

  // Every note on knows when it ends (pending_offs_), so a jump of the start or the length never leaves
  // a note hanging and never cuts the one that just began
  for (const Event& event : schedule_) {
    if (!event.on)
      continue;
    int note = event.note + transpose;
    if (note < 0 || note > 127)
      continue;

    double cycle = std::ceil((block_start - origin - event.time) / length);
    for (;; cycle += 1.0) {
      double time = origin + event.time + cycle * length;
      if (time >= block_end)
        break;
      if (time < block_start)
        continue;

      int sample = sampleAt(time);
      // The same pitch still sounding ends here, or earlier when its own end comes first
      bool ended = false;
      for (size_t i = 0; i < pending_offs_.size(); ++i) {
        if (pending_offs_[i].note != note)
          continue;
        output_.addEvent(MidiMessage::noteOff(1, note), sampleAt(std::min(pending_offs_[i].time, time)));
        pending_offs_[i] = pending_offs_.back();
        pending_offs_.pop_back();
        ended = true;
        break;
      }
      if (active_notes_[note] && !ended)
        output_.addEvent(MidiMessage::noteOff(1, note), sample);
      output_.addEvent(MidiMessage::noteOn(1, note, (uint8)event.velocity), sample);
      active_notes_[note] = true;
      pending_offs_.push_back({ note, time + event.duration });
    }
  }

  for (size_t i = 0; i < pending_offs_.size();) {
    if (pending_offs_[i].time >= block_end) {
      ++i;
      continue;
    }
    int note = pending_offs_[i].note;
    if (active_notes_[note]) {
      output_.addEvent(MidiMessage::noteOff(1, note), sampleAt(pending_offs_[i].time));
      active_notes_[note] = false;
    }
    pending_offs_[i] = pending_offs_.back();
    pending_offs_.pop_back();
  }

  free_position_ = block_end;
  was_active_ = true;
  double phase = std::fmod(block_start - origin, length);
  playhead_.store((float)(phase < 0.0 ? phase + length : phase));
  midi.swapWith(output_);
}
