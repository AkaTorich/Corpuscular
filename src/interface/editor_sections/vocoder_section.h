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
#include "preset_selector.h"
#include "synth_section.h"

namespace wv {
  class Wavetable;
}

class SynthButton;
class TextSelector;
class VocoderBandViewer;
class VocoderTableViewer;

// The VOCODER effect on the FX tab: the voice from the input speaks through the synth or a wavetable
class VocoderSection : public SynthSection, public PresetSelector::Listener {
  public:
    VocoderSection(const String& name);
    ~VocoderSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void reset() override;
    void setActive(bool active) override;

    void prevClicked() override;
    void nextClicked() override;
    void textMouseDown(const MouseEvent& e) override;

    void loadFile(const File& file) override;
    File getCurrentFile() override { return current_file_; }

  private:
    void setTable(std::unique_ptr<wv::Wavetable> table);
    void updateTableName();

    File current_file_;

    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<TextSelector> carrier_;
    std::unique_ptr<TextSelector> pitch_mode_;
    std::unique_ptr<SynthSlider> note_;
    std::unique_ptr<SynthSlider> octave_;
    std::unique_ptr<SynthSlider> tilt_;
    std::unique_ptr<SynthSlider> spread_;
    std::unique_ptr<TextSelector> freeze_;

    std::unique_ptr<PresetSelector> table_selector_;
    std::unique_ptr<VocoderTableViewer> table_viewer_;
    std::unique_ptr<VocoderBandViewer> band_viewer_;

    std::unique_ptr<SynthSlider> position_;
    std::unique_ptr<SynthSlider> detune_;
    std::unique_ptr<SynthSlider> glide_;
    std::unique_ptr<SynthSlider> unison_;
    std::unique_ptr<SynthSlider> bands_;
    std::unique_ptr<SynthSlider> dry_wet_;
    std::unique_ptr<SynthSlider> level_;

    std::unique_ptr<SynthSlider> width_;
    std::unique_ptr<SynthSlider> formant_;
    std::unique_ptr<SynthSlider> attack_;
    std::unique_ptr<SynthSlider> release_;
    std::unique_ptr<SynthSlider> air_;
    std::unique_ptr<SynthSlider> noise_;
    std::unique_ptr<SynthSlider> gate_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocoderSection)
};
