/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "arp_piano_roll.h"

#include "fonts.h"
#include "skin.h"

namespace {
  constexpr int kRollMaxUndo = 64;
  constexpr float kRollMinStepsVisible = 4.0f;
  constexpr float kRollMaxStepsVisible = 512.0f;
  constexpr float kRollMinRowsVisible = 8.0f;
  constexpr float kRollMaxRowsVisible = 88.0f;
  constexpr float kRollResizeEdge = 7.0f;
  const char* kRollNoteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

ArpPianoRoll::ArpPianoRoll() : image_component_("arp_piano_roll"), listener_(nullptr), length_(16),
                               last_note_length_(1), first_step_(0.0f), steps_visible_(32.0f), low_pitch_(48.0f),
                               rows_visible_(30.0f), playhead_(-1.0f), tonic_pitch_class_(0),
                               drag_mode_(kNone), length_origin_(16), drag_changed_(false) {
  image_component_.setComponent(this);
  image_component_.setInterceptsMouseClicks(false, false);
  std::fill(std::begin(scale_pitches_), std::end(scale_pitches_), true);
  setWantsKeyboardFocus(true);
}

void ArpPianoRoll::setPattern(const std::vector<Note>& notes, int length, bool undoable) {
  if (undoable)
    pushUndo();
  notes_ = notes;
  selected_.assign(notes_.size(), false);
  length_ = jlimit(1, ArpSequencer::kMaxSteps, length);
  clampView();
  redraw();
}

void ArpPianoRoll::setLength(int length) {
  length = jlimit(1, ArpSequencer::kMaxSteps, length);
  if (length == length_)
    return;
  pushUndo();
  length_ = length;
  edited();
}

void ArpPianoRoll::setScale(int tonic_pitch_class, const Array<int>& steps) {
  tonic_pitch_class_ = ((tonic_pitch_class % 12) + 12) % 12;
  std::fill(std::begin(scale_pitches_), std::end(scale_pitches_), steps.isEmpty());
  int pitch = 0;
  scale_pitches_[0] = true;
  for (int step : steps) {
    pitch += step;
    scale_pitches_[pitch % 12] = true;
  }
  redraw();
}

void ArpPianoRoll::setPlayhead(float step) {
  if (std::abs(step - playhead_) < 0.01f)
    return;
  playhead_ = step;
  redraw();
}

// Geometry
float ArpPianoRoll::keyWidth() const { return std::max(30.0f, getWidth() * 0.045f); }
float ArpPianoRoll::rulerHeight() const { return std::max(14.0f, getHeight() * 0.045f); }
float ArpPianoRoll::stepWidth() const { return (getWidth() - keyWidth()) / steps_visible_; }
float ArpPianoRoll::rowHeight() const { return (getHeight() - rulerHeight()) / rows_visible_; }
float ArpPianoRoll::stepToX(float step) const { return keyWidth() + (step - first_step_) * stepWidth(); }

float ArpPianoRoll::pitchToY(int pitch) const {
  return rulerHeight() + (low_pitch_ + rows_visible_ - 1.0f - pitch) * rowHeight();
}

float ArpPianoRoll::xToStep(float x) const { return first_step_ + (x - keyWidth()) / stepWidth(); }

int ArpPianoRoll::yToPitch(float y) const {
  return (int)std::floor(low_pitch_ + rows_visible_ - (y - rulerHeight()) / rowHeight());
}

Rectangle<float> ArpPianoRoll::noteBounds(const Note& note) const {
  return Rectangle<float>(stepToX((float)note.start), pitchToY(note.midi),
                          note.length * stepWidth(), rowHeight());
}

int ArpPianoRoll::noteAt(Point<float> point) const {
  for (int i = (int)notes_.size() - 1; i >= 0; --i) {
    if (noteBounds(notes_[i]).contains(point))
      return i;
  }
  return -1;
}

bool ArpPianoRoll::onResizeEdge(int index, Point<float> point) const {
  if (index < 0)
    return false;
  Rectangle<float> bounds = noteBounds(notes_[index]);
  float edge = std::min(kRollResizeEdge, bounds.getWidth() * 0.35f);
  return point.x >= bounds.getRight() - edge;
}

bool ArpPianoRoll::inScale(int pitch) const {
  return scale_pitches_[((pitch - tonic_pitch_class_) % 12 + 12) % 12];
}

void ArpPianoRoll::clampView() {
  steps_visible_ = jlimit(kRollMinStepsVisible, kRollMaxStepsVisible, steps_visible_);
  rows_visible_ = jlimit(kRollMinRowsVisible, kRollMaxRowsVisible, rows_visible_);
  first_step_ = jlimit(0.0f, std::max(0.0f, (float)ArpSequencer::kMaxSteps - steps_visible_), first_step_);
  low_pitch_ = jlimit(0.0f, 128.0f - rows_visible_, low_pitch_);
}

void ArpPianoRoll::zoomTime(float factor) {
  float center = first_step_ + steps_visible_ * 0.5f;
  steps_visible_ *= factor;
  clampView();
  first_step_ = center - steps_visible_ * 0.5f;
  clampView();
  redraw();
}

void ArpPianoRoll::zoomPitch(float factor) {
  float center = low_pitch_ + rows_visible_ * 0.5f;
  rows_visible_ *= factor;
  clampView();
  low_pitch_ = center - rows_visible_ * 0.5f;
  clampView();
  redraw();
}

// Shows the whole pattern and all its notes with a little room around them
void ArpPianoRoll::fitToPattern() {
  first_step_ = 0.0f;
  steps_visible_ = (float)std::max(16, length_);
  int low = 127, high = 0;
  for (const Note& note : notes_) {
    low = std::min(low, note.midi);
    high = std::max(high, note.midi);
  }
  if (notes_.empty()) {
    low = 48;
    high = 72;
  }
  rows_visible_ = (float)std::max(18, high - low + 7);
  low_pitch_ = (float)(low - 3);
  clampView();
  redraw();
}

// Editing
void ArpPianoRoll::pushUndo() {
  undo_.push_back({ notes_, length_ });
  if ((int)undo_.size() > kRollMaxUndo)
    undo_.erase(undo_.begin());
  redo_.clear();
}

void ArpPianoRoll::edited() {
  selected_.resize(notes_.size(), false);
  redraw();
  if (listener_)
    listener_->patternEdited();
}

void ArpPianoRoll::undo() {
  if (undo_.empty())
    return;
  redo_.push_back({ notes_, length_ });
  notes_ = undo_.back().notes;
  length_ = undo_.back().length;
  undo_.pop_back();
  selected_.assign(notes_.size(), false);
  edited();
}

void ArpPianoRoll::redo() {
  if (redo_.empty())
    return;
  undo_.push_back({ notes_, length_ });
  notes_ = redo_.back().notes;
  length_ = redo_.back().length;
  redo_.pop_back();
  selected_.assign(notes_.size(), false);
  edited();
}

void ArpPianoRoll::clearNotes() {
  if (notes_.empty())
    return;
  pushUndo();
  notes_.clear();
  selected_.clear();
  edited();
}

void ArpPianoRoll::deleteSelected() {
  std::vector<Note> kept;
  for (size_t i = 0; i < notes_.size(); ++i) {
    if (!selected_[i])
      kept.push_back(notes_[i]);
  }
  if (kept.size() == notes_.size())
    return;
  pushUndo();
  notes_ = kept;
  selected_.assign(notes_.size(), false);
  edited();
}

void ArpPianoRoll::moveSelected(int steps, int semitones) {
  bool any = false;
  for (size_t i = 0; i < notes_.size(); ++i)
    any = any || selected_[i];
  if (!any)
    return;
  pushUndo();
  for (size_t i = 0; i < notes_.size(); ++i) {
    if (!selected_[i])
      continue;
    notes_[i].start = jlimit(0, ArpSequencer::kMaxSteps - 1, notes_[i].start + steps);
    notes_[i].midi = jlimit(0, 127, notes_[i].midi + semitones);
  }
  edited();
}

void ArpPianoRoll::duplicateSelected() {
  int first = ArpSequencer::kMaxSteps, last = 0;
  std::vector<Note> copies;
  for (size_t i = 0; i < notes_.size(); ++i) {
    if (selected_[i]) {
      first = std::min(first, notes_[i].start);
      last = std::max(last, notes_[i].start + notes_[i].length);
      copies.push_back(notes_[i]);
    }
  }
  if (copies.empty())
    return;
  pushUndo();
  int shift = last - first;
  selected_.assign(notes_.size(), false);
  for (Note copy : copies) {
    copy.start += shift;
    if (copy.start >= ArpSequencer::kMaxSteps)
      continue;
    notes_.push_back(copy);
    selected_.push_back(true);
  }
  edited();
}

// Mouse
void ArpPianoRoll::mouseDown(const MouseEvent& e) {
  grabKeyboardFocus();
  Point<float> point = e.position;
  drag_start_ = point;
  drag_changed_ = false;
  drag_mode_ = kNone;
  selected_.resize(notes_.size(), false);

  // Ruler: clicking or dragging in it moves the pattern end
  if (point.y < rulerHeight()) {
    pushUndo();
    drag_mode_ = kLength;
    length_origin_ = length_;
    mouseDrag(e);
    return;
  }
  if (point.x < keyWidth())
    return;

  int index = noteAt(point);
  if (e.mods.isPopupMenu()) {
    if (index >= 0) {
      pushUndo();
      notes_.erase(notes_.begin() + index);
      selected_.erase(selected_.begin() + index);
      edited();
    }
    return;
  }

  if (index >= 0) {
    if (e.mods.isShiftDown())
      selected_[index] = !selected_[index];
    else if (!selected_[index])
      selected_.assign(notes_.size(), false);
    selected_[index] = true;
    pushUndo();
    drag_mode_ = onResizeEdge(index, point) ? kResize : kMove;
    drag_origin_ = notes_;
    last_note_length_ = notes_[index].length;
    redraw();
    return;
  }

  if (e.mods.isShiftDown()) {
    drag_mode_ = kSelectBox;
    select_box_ = Rectangle<float>(point, point);
    return;
  }

  // Empty cell: a new note of the last used length, dragging places it
  pushUndo();
  Note note;
  note.start = jlimit(0, ArpSequencer::kMaxSteps - 1, (int)std::floor(xToStep(point.x)));
  note.midi = jlimit(0, 127, yToPitch(point.y));
  note.length = last_note_length_;
  note.velocity = 100;
  notes_.push_back(note);
  selected_.assign(notes_.size(), false);
  selected_.back() = true;
  drag_mode_ = kMove;
  drag_origin_ = notes_;
  drag_changed_ = true;
  edited();
}

void ArpPianoRoll::mouseDrag(const MouseEvent& e) {
  Point<float> point = e.position;

  if (drag_mode_ == kLength) {
    int length = jlimit(1, ArpSequencer::kMaxSteps, (int)std::round(xToStep(point.x)));
    if (length != length_) {
      length_ = length;
      edited();
    }
    return;
  }

  if (drag_mode_ == kSelectBox) {
    select_box_ = Rectangle<float>(drag_start_, point);
    for (size_t i = 0; i < notes_.size(); ++i)
      selected_[i] = (e.mods.isCtrlDown() && selected_[i]) || select_box_.intersects(noteBounds(notes_[i]));
    redraw();
    return;
  }

  if (drag_mode_ != kMove && drag_mode_ != kResize)
    return;

  int steps = (int)std::round((point.x - drag_start_.x) / stepWidth());
  int semitones = (int)std::round(-(point.y - drag_start_.y) / rowHeight());
  bool changed = false;
  for (size_t i = 0; i < notes_.size() && i < drag_origin_.size(); ++i) {
    if (!selected_[i])
      continue;
    Note note = drag_origin_[i];
    if (drag_mode_ == kMove) {
      note.start = jlimit(0, ArpSequencer::kMaxSteps - 1, note.start + steps);
      note.midi = jlimit(0, 127, note.midi + semitones);
    }
    else {
      note.length = jlimit(1, ArpSequencer::kMaxSteps, note.length + steps);
      last_note_length_ = note.length;
    }
    if (note.start != notes_[i].start || note.midi != notes_[i].midi || note.length != notes_[i].length) {
      notes_[i] = note;
      changed = true;
    }
  }
  if (changed) {
    drag_changed_ = true;
    edited();
  }
}

void ArpPianoRoll::mouseUp(const MouseEvent& e) {
  // A click on a note that moved nothing leaves no undo step behind
  if ((drag_mode_ == kMove || drag_mode_ == kResize) && !drag_changed_ && !undo_.empty())
    undo_.pop_back();
  if (drag_mode_ == kLength && length_ == length_origin_ && !undo_.empty())
    undo_.pop_back();
  drag_mode_ = kNone;
  select_box_ = Rectangle<float>();
  redraw();
}

void ArpPianoRoll::mouseDoubleClick(const MouseEvent& e) {
  int index = noteAt(e.position);
  if (index < 0 || e.mods.isShiftDown())
    return;
  pushUndo();
  notes_.erase(notes_.begin() + index);
  selected_.erase(selected_.begin() + index);
  edited();
}

void ArpPianoRoll::mouseMove(const MouseEvent& e) {
  int index = noteAt(e.position);
  if (e.position.y < rulerHeight())
    setMouseCursor(MouseCursor::LeftRightResizeCursor);
  else if (onResizeEdge(index, e.position))
    setMouseCursor(MouseCursor::RightEdgeResizeCursor);
  else if (index >= 0)
    setMouseCursor(MouseCursor::DraggingHandCursor);
  else
    setMouseCursor(MouseCursor::NormalCursor);
}

void ArpPianoRoll::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  float delta = wheel.deltaY != 0.0f ? wheel.deltaY : wheel.deltaX;
  if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
    // Zoom time around the mouse
    float anchor = xToStep(e.position.x);
    steps_visible_ *= delta > 0.0f ? 0.8f : 1.25f;
    clampView();
    first_step_ = anchor - (e.position.x - keyWidth()) / stepWidth();
    clampView();
  }
  else if (e.mods.isAltDown()) {
    float anchor = low_pitch_ + rows_visible_ - (e.position.y - rulerHeight()) / rowHeight();
    rows_visible_ *= delta > 0.0f ? 0.8f : 1.25f;
    clampView();
    low_pitch_ = anchor - rows_visible_ + (e.position.y - rulerHeight()) / rowHeight();
    clampView();
  }
  else if (e.mods.isShiftDown() || wheel.deltaX != 0.0f)
    first_step_ -= delta * steps_visible_ * 0.25f;
  else
    low_pitch_ += delta * rows_visible_ * 0.25f;
  clampView();
  redraw();
}

bool ArpPianoRoll::keyPressed(const KeyPress& key) {
  bool command = key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown();
  bool shift = key.getModifiers().isShiftDown();
  int code = key.getKeyCode();

  if (code == KeyPress::deleteKey || code == KeyPress::backspaceKey) {
    deleteSelected();
    return true;
  }
  if (command && (code == 'Z' || code == 'z')) {
    shift ? redo() : undo();
    return true;
  }
  if (command && (code == 'Y' || code == 'y')) {
    redo();
    return true;
  }
  if (command && (code == 'A' || code == 'a')) {
    selected_.assign(notes_.size(), true);
    redraw();
    return true;
  }
  if (command && (code == 'C' || code == 'c')) {
    clipboard_.clear();
    for (size_t i = 0; i < notes_.size(); ++i) {
      if (selected_[i])
        clipboard_.push_back(notes_[i]);
    }
    return true;
  }
  if (command && (code == 'V' || code == 'v')) {
    if (clipboard_.empty())
      return true;
    // Pastes at the playhead if it runs, otherwise right after the copied notes
    int first = ArpSequencer::kMaxSteps, last = 0;
    for (const Note& note : clipboard_) {
      first = std::min(first, note.start);
      last = std::max(last, note.start + note.length);
    }
    int target = playhead_ >= 0.0f ? (int)playhead_ : last;
    pushUndo();
    selected_.assign(notes_.size(), false);
    for (Note note : clipboard_) {
      note.start = note.start - first + target;
      if (note.start < ArpSequencer::kMaxSteps) {
        notes_.push_back(note);
        selected_.push_back(true);
      }
    }
    edited();
    return true;
  }
  if (command && (code == 'D' || code == 'd')) {
    duplicateSelected();
    return true;
  }
  if (code == KeyPress::upKey || code == KeyPress::downKey) {
    int semitones = (code == KeyPress::upKey ? 1 : -1) * (shift ? 12 : 1);
    moveSelected(0, semitones);
    return true;
  }
  if (code == KeyPress::leftKey || code == KeyPress::rightKey) {
    int steps = (code == KeyPress::rightKey ? 1 : -1) * (shift ? 16 : 1);
    moveSelected(steps, 0);
    return true;
  }
  return false;
}

void ArpPianoRoll::resized() {
  clampView();
  redraw();
}

void ArpPianoRoll::paint(Graphics& g) {
  Colour background = findColour(Skin::kWidgetBackground, true);
  Colour body = findColour(Skin::kBody, true);
  Colour text = findColour(Skin::kBodyText, true);
  Colour line = findColour(Skin::kLightenScreen, true);
  Colour note_colour = findColour(Skin::kRotaryArc, true);

  float width = (float)getWidth();
  float height = (float)getHeight();
  float key_width = keyWidth();
  float ruler = rulerHeight();
  float step_width = stepWidth();
  float row_height = rowHeight();

  g.setColour(background);
  g.fillRect(0.0f, 0.0f, width, height);

  // Rows: scale notes lighter, the tonic row marked
  int top_pitch = (int)std::ceil(low_pitch_ + rows_visible_);
  for (int pitch = (int)std::floor(low_pitch_); pitch <= top_pitch; ++pitch) {
    float y = pitchToY(pitch);
    if (y > height || y + row_height < ruler)
      continue;
    if (inScale(pitch)) {
      g.setColour(line.withMultipliedAlpha(((pitch - tonic_pitch_class_) % 12 + 12) % 12 == 0 ? 0.9f : 0.45f));
      g.fillRect(key_width, y, width - key_width, row_height);
    }
    g.setColour(line.withMultipliedAlpha(0.35f));
    g.fillRect(key_width, y + row_height - 1.0f, width - key_width, 1.0f);
  }

  // Columns: steps, beats and bars; past the pattern end the grid goes dark
  int first = (int)std::floor(first_step_);
  int last = (int)std::ceil(first_step_ + steps_visible_);
  bool thin = step_width < 6.0f;
  for (int step = first; step <= last; ++step) {
    float x = stepToX((float)step);
    if (x < key_width)
      continue;
    float alpha = step % 16 == 0 ? 1.0f : step % 4 == 0 ? 0.6f : thin ? 0.0f : 0.25f;
    if (alpha > 0.0f) {
      g.setColour(line.withMultipliedAlpha(alpha));
      g.fillRect(x, ruler, step % 16 == 0 ? 2.0f : 1.0f, height - ruler);
    }
  }
  float end_x = stepToX((float)length_);
  if (end_x < width) {
    g.setColour(Colours::black.withAlpha(0.45f));
    g.fillRect(std::max(key_width, end_x), ruler, width - std::max(key_width, end_x), height - ruler);
  }

  // Notes
  for (size_t i = 0; i < notes_.size(); ++i) {
    Rectangle<float> bounds = noteBounds(notes_[i]).reduced(1.0f, 1.0f);
    if (bounds.getRight() < key_width || bounds.getX() > width || bounds.getBottom() < ruler || bounds.getY() > height)
      continue;
    bool selected = i < selected_.size() && selected_[i];
    bool past_end = notes_[i].start >= length_;
    Colour fill = past_end ? note_colour.withMultipliedAlpha(0.3f) : note_colour.withMultipliedAlpha(0.55f + notes_[i].velocity / 290.0f);
    g.setColour(selected ? fill.brighter(0.5f) : fill);
    g.fillRoundedRectangle(bounds, std::min(3.0f, bounds.getHeight() * 0.3f));
    g.setColour(selected ? text : fill.darker(0.6f));
    g.drawRoundedRectangle(bounds, std::min(3.0f, bounds.getHeight() * 0.3f), selected ? 1.5f : 1.0f);
  }

  if (!select_box_.isEmpty()) {
    g.setColour(text.withAlpha(0.15f));
    g.fillRect(select_box_);
    g.setColour(text.withAlpha(0.7f));
    g.drawRect(select_box_, 1.0f);
  }

  // Playhead
  if (playhead_ >= 0.0f) {
    float x = stepToX(playhead_);
    if (x >= key_width && x <= width) {
      g.setColour(text.withAlpha(0.85f));
      g.fillRect(x, ruler, 2.0f, height - ruler);
    }
  }

  // Keys column
  g.setColour(body);
  g.fillRect(0.0f, ruler, key_width, height - ruler);
  Font font = Fonts::instance()->proportional_regular().withPointHeight(std::min(row_height * 0.75f, 12.0f));
  g.setFont(font);
  for (int pitch = (int)std::floor(low_pitch_); pitch <= top_pitch; ++pitch) {
    float y = pitchToY(pitch);
    if (y > height || y + row_height < ruler)
      continue;
    int pitch_class = pitch % 12;
    bool black = pitch_class == 1 || pitch_class == 3 || pitch_class == 6 || pitch_class == 8 || pitch_class == 10;
    g.setColour(black ? background.darker(0.3f) : text.withAlpha(0.18f));
    g.fillRect(0.0f, y + 0.5f, key_width * (black ? 0.6f : 1.0f), row_height - 1.0f);
    if (pitch_class == 0 || (row_height >= 11.0f && inScale(pitch))) {
      g.setColour(text.withAlpha(pitch_class == 0 ? 1.0f : 0.6f));
      g.drawText(String(kRollNoteNames[pitch_class]) + String(pitch / 12 - 1),
                 Rectangle<float>(2.0f, y, key_width - 4.0f, row_height), Justification::centredRight, false);
    }
  }

  // Ruler: bar numbers and the pattern end marker
  g.setColour(body);
  g.fillRect(0.0f, 0.0f, width, ruler);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(ruler * 0.6f));
  for (int bar = first / 16; bar * 16 <= last; ++bar) {
    float x = stepToX(bar * 16.0f);
    if (x < key_width || x > width)
      continue;
    g.setColour(text.withAlpha(0.8f));
    g.drawText(String(bar + 1), Rectangle<float>(x + 3.0f, 0.0f, 40.0f, ruler), Justification::centredLeft, false);
  }
  if (end_x >= key_width && end_x <= width) {
    g.setColour(note_colour);
    Path marker;
    marker.addTriangle(end_x - ruler * 0.45f, 0.0f, end_x, ruler, end_x, 0.0f);
    g.fillPath(marker);
    g.fillRect(end_x - 1.0f, 0.0f, 2.0f, height);
  }
  g.setColour(line);
  g.fillRect(0.0f, ruler - 1.0f, width, 1.0f);
}
