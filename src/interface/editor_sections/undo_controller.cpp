/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "undo_controller.h"

#include "synth_base.h"
#include "synth_gui_interface.h"

namespace {
  // Played or display-only values, not sound settings
  bool undoIgnores(const std::string& name) {
    return name == "bypass" || name == "pitch_wheel" || name == "mod_wheel" || name == "view_spectrogram";
  }

  // A click in the window that changed nothing still marks the next change as the user's for this long
  constexpr uint32 kTouchMemoryMilliseconds = 1500;
}

UndoController::UndoController(Component* window, SynthBase* synth) :
    window_(window), synth_(synth), state_loads_(-1), user_touched_(false), touch_time_(0) {
  Desktop::getInstance().addGlobalMouseListener(this);
  rebase();
  startTimer(kPollMilliseconds);
}

UndoController::~UndoController() {
  stopTimer();
  Desktop::getInstance().removeGlobalMouseListener(this);
}

UndoController::Snapshot UndoController::readControls() const {
  Snapshot snapshot;
  for (auto& control : synth_->getControls()) {
    if (!undoIgnores(control.first))
      snapshot[control.first] = control.second->value();
  }
  return snapshot;
}

void UndoController::rebase() {
  committed_ = readControls();
  last_seen_ = committed_;
  state_loads_ = synth_->getStateLoads();
}

void UndoController::touched(const MouseEvent& e) {
  if (e.eventComponent == nullptr || (e.eventComponent != window_ && !window_->isParentOf(e.eventComponent)))
    return;
  user_touched_ = true;
  touch_time_ = Time::getMillisecondCounter();
}

void UndoController::mouseDown(const MouseEvent& e) {
  touched(e);
}

void UndoController::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  touched(e);
}

void UndoController::timerCallback() {
  poll(false);
}

void UndoController::poll(bool force) {
  // A preset (or the DAW project) was loaded: the old steps no longer apply
  if (synth_->getStateLoads() != state_loads_) {
    undo_.clear();
    redo_.clear();
    user_touched_ = false;
    rebase();
    return;
  }

  // A drag is still going: the whole drag becomes one step when the button is released
  if (!force && ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown())
    return;

  // Values still moving (a wheel turn): wait until they stay put for one poll
  Snapshot now = readControls();
  bool settled = now == last_seen_;
  last_seen_ = now;
  if (!settled && !force)
    return;

  Step step;
  for (auto& value : now) {
    auto before = committed_.find(value.first);
    if (before != committed_.end() && before->second != value.second)
      step[value.first] = { before->second, value.second };
  }

  if (step.empty()) {
    if (user_touched_ && Time::getMillisecondCounter() - touch_time_ > kTouchMemoryMilliseconds)
      user_touched_ = false;
    return;
  }

  committed_ = now;
  if (!user_touched_)
    return;

  user_touched_ = false;
  undo_.push_back(step);
  if ((int)undo_.size() > kMaxSteps)
    undo_.erase(undo_.begin());
  redo_.clear();
}

void UndoController::apply(const Step& step, bool forward) {
  for (auto& change : step) {
    if (synth_->getControls().count(change.first))
      synth_->valueChangedInternal(change.first, forward ? change.second.second : change.second.first);
  }

  SynthGuiInterface* gui = window_->findParentComponentOfClass<SynthGuiInterface>();
  if (gui)
    gui->updateFullGui();
  user_touched_ = false;
  rebase();
}

bool UndoController::undo() {
  // A change not yet taken in (undo clicked right after a turn) becomes a step first
  poll(true);
  if (undo_.empty())
    return false;

  Step step = undo_.back();
  undo_.pop_back();
  apply(step, false);
  redo_.push_back(step);
  return true;
}

bool UndoController::redo() {
  if (redo_.empty())
    return false;

  Step step = redo_.back();
  redo_.pop_back();
  apply(step, true);
  undo_.push_back(step);
  return true;
}
