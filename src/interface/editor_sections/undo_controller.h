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

#include <map>
#include <string>
#include <vector>

class SynthBase;

// Undo and redo for every knob, switch and list choice of the synth. A finished change made in the window
// (a drag, a click, a wheel turn, a pick from a list) is one step. Changes from the DAW (automation) or MIDI
// are not steps. Loading a preset starts a new history.
class UndoController : public Timer, public MouseListener {
  public:
    static constexpr int kMaxSteps = 200;
    static constexpr int kPollMilliseconds = 150;

    UndoController(Component* window, SynthBase* synth);
    ~UndoController();

    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }
    bool undo();
    bool redo();

    void timerCallback() override;
    void mouseDown(const MouseEvent& e) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;

  private:
    using Snapshot = std::map<std::string, float>;
    // Control name -> value before and after the step
    using Step = std::map<std::string, std::pair<float, float>>;

    Snapshot readControls() const;
    void poll(bool force);
    void rebase();
    void apply(const Step& step, bool forward);
    void touched(const MouseEvent& e);

    Component* window_;
    SynthBase* synth_;
    Snapshot committed_;
    Snapshot last_seen_;
    int state_loads_;
    bool user_touched_;
    uint32 touch_time_;
    std::vector<Step> undo_;
    std::vector<Step> redo_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoController)
};
