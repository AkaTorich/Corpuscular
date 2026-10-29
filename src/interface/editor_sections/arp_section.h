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
#include "arp_piano_roll.h"
#include "save_section.h"
#include "synth_section.h"

// ARP tab: the Melody Atoms generators write a melody, the piano roll edits it and the built-in sequencer plays it
// from the note held in the DAW (or on a keyboard) as the tonic. Generating works while it plays.
// The pattern with all ARP settings can be saved to Documents\Corpuscular\Arps and picked back from there.
class ArpSection : public SynthSection, public ArpPianoRoll::Listener, public Timer, public SaveSection::Listener {
  public:
    enum Mode { kPsyArp, kArpRiff, kArpStyled, kBass, kPad, kNumModes };

    ArpSection();

    void paintBackground(Graphics& g) override;
    void resized() override;
    void reset() override;
    void visibilityChanged() override;
    void mouseDown(const MouseEvent& e) override;
    void timerCallback() override;
    void patternEdited() override;

    // Saved arps
    void loadFile(const File& file) override;
    File getCurrentFile() override { return current_arp_file_; }
    void save(File file) override;

  private:
    enum Control {
      kOn, kLatch, kMode, kKey, kScaleGroup, kScale, kParam1, kParam2, kParam3, kParam4, kGenerate,
      kRate, kOctave, kGate, kBars, kZoomTimeOut, kZoomTimeIn, kZoomNotesOut, kZoomNotesIn, kFit,
      kUndo, kRedo, kClear, kSave, kSavedArp, kNumControls
    };

    bool hasList(int control) const;
    PopupItems listItems(int control) const;
    void selectFromList(int control, int index);
    void showList(int control);
    void saveArp();
    void browseArps();
    void stepSavedArp(int delta);
    File arpFolder() const;

    ArpSequencer* sequencer();
    void loadFromSequencer();
    void saveUiState();
    void pushPattern();

    Rectangle<float> contentArea() const;
    Rectangle<float> controlBounds(int control) const;
    Rectangle<float> rollBounds() const;
    Rectangle<float> infoBounds() const;

    String controlTitle(int control) const;
    String controlValue(int control) const;
    bool isButton(int control) const;
    bool isLit(int control) const;
    void stepControl(int control, int delta);
    void clickButton(int control);

    void setModeDefaults();
    int paramMin(int param) const;
    int paramMax(int param) const;
    StringArray paramNames(int param) const;
    void generate();
    void updateScaleHighlight();

    ArpPianoRoll piano_roll_;
    bool loaded_;

    int mode_;
    int key_;
    int scale_group_;
    int scale_;
    int params_[4];
    String info_;
    File current_arp_file_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpSection)
};
