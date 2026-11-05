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
#include "arp_sequencer.h"
#include "open_gl_image_component.h"

// Piano roll of the ARP tab, drawn into its own texture.
// Mouse: click an empty cell to add a note (drag to place it), drag a note to move it, drag its right edge to
// change its length, right-click or double-click to delete, Shift+drag on empty space to select a box,
// Shift+click adds to the selection, click or drag in the ruler to move the pattern end. "Arp Pattern Length" in the
// MATRIX destinations shortens it, "Arp Pattern Start"
// moves where it plays from; both are drawn as faint markers.
// Wheel scrolls notes, Shift+wheel scrolls time, Ctrl+wheel zooms time, Alt+wheel zooms notes.
// Keys: Delete, Ctrl+A, Ctrl+C / Ctrl+V, Ctrl+D (duplicate), Ctrl+Z / Ctrl+Y, arrows move (Shift: by octave or bar).
// Under the notes is the VELOCITY lane: a bar per note, as tall as the note is strong. Dragging in it sets the bars
// under the mouse (only the selected ones when the column has a selected note). A new note takes the last strength set.
class ArpPianoRoll : public Component {
  public:
    using Note = ArpSequencer::Note;

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void patternEdited() = 0;
    };

    ArpPianoRoll();

    void setListener(Listener* listener) { listener_ = listener; }
    void setPattern(const std::vector<Note>& notes, int length, bool undoable);
    const std::vector<Note>& getNotes() const { return notes_; }
    int getLength() const { return length_; }
    void setLength(int length);
    void setScale(int tonic_pitch_class, const Array<int>& steps);
    void setPlayhead(float step);
    // Where the pattern ends while something modulates its length (drawn faint next to the marker); <= 0 hides it
    void setModulatedLength(int length);
    // Where the pattern plays from while something modulates its start (drawn faint); 0 hides it
    void setModulatedStart(int step);
    float getRulerHeight() const { return rulerHeight(); }

    void zoomTime(float factor);
    void zoomPitch(float factor);
    void fitToPattern();
    void clearNotes();
    void undo();
    void redo();
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }

    OpenGlImageComponent* getImageComponent() { return &image_component_; }
    void redraw() { image_component_.redrawImage(true); }

    void paint(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDoubleClick(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    bool keyPressed(const KeyPress& key) override;

  private:
    enum DragMode { kNone, kMove, kResize, kSelectBox, kLength, kVelocity };
    struct Snapshot {
      std::vector<Note> notes;
      int length;
    };

    float keyWidth() const;
    float rulerHeight() const;
    float laneHeight() const;
    float gridBottom() const;
    bool inLane(Point<float> point) const;
    int laneToVelocity(float y) const;
    void setVelocityAt(Point<float> point);
    float stepWidth() const;
    float rowHeight() const;
    float stepToX(float step) const;
    float pitchToY(int pitch) const;
    float xToStep(float x) const;
    int yToPitch(float y) const;
    Rectangle<float> noteBounds(const Note& note) const;
    int noteAt(Point<float> point) const;
    bool onResizeEdge(int index, Point<float> point) const;
    bool inScale(int pitch) const;

    void pushUndo();
    void edited();
    void clampView();
    void deleteSelected();
    void moveSelected(int steps, int semitones);
    void duplicateSelected();

    OpenGlImageComponent image_component_;
    Listener* listener_;

    std::vector<Note> notes_;
    std::vector<bool> selected_;
    std::vector<Note> drag_origin_;
    std::vector<Note> clipboard_;
    std::vector<Snapshot> undo_;
    std::vector<Snapshot> redo_;
    int length_;
    int last_note_length_;
    int last_velocity_;

    float first_step_;
    float steps_visible_;
    float low_pitch_;
    float rows_visible_;
    float playhead_;
    int modulated_length_;
    int modulated_start_;

    int tonic_pitch_class_;
    bool scale_pitches_[12];

    DragMode drag_mode_;
    Point<float> drag_start_;
    Rectangle<float> select_box_;
    int length_origin_;
    bool drag_changed_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpPianoRoll)
};
