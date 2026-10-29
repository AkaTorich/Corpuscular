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
#include "synth_section.h"
#include "wavetable_generator.h"

// GEN tab: the built-in preset and wavetable generators. Everything made here goes straight into
// Corpuscular: a preset is loaded, a bank lands in the library and opens in the preset browser,
// a wavetable is loaded into an oscillator and joins the wavetable library.
class GeneratorSection : public SynthSection {
  public:
    static constexpr int kNumWavetableTargets = 3;

    GeneratorSection();

    // Loads a wavetable file into oscillator `index` and shows it
    void setWavetableLoader(std::function<void(int, const File&)> loader) { wavetable_loader_ = std::move(loader); }
    // Shows a freshly made bank (the first preset is already loaded)
    void setBankViewer(std::function<void()> viewer) { bank_viewer_ = std::move(viewer); }

    void paintBackground(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& e) override;

  private:
    enum Option { kScaleGroup, kScale, kMelody, kStep, kVocoder, kNumOptions };
    enum VocoderChoice { kVocoderOff, kVocoderOn, kVocoderSome, kNumVocoderChoices };
    enum TableOption { kTableType, kTableFrames, kTableSize, kTableSmooth, kTableFormat, kNumTableOptions };

    Rectangle<float> contentArea() const;
    Rectangle<float> band(float top, float bottom) const;
    Rectangle<float> typeCell(int index) const;
    Rectangle<float> optionBox(int option) const;
    Rectangle<float> generateButton() const;
    Rectangle<float> comboButton() const;
    Rectangle<float> similarButton() const;
    Rectangle<float> historyBackButton() const;
    Rectangle<float> historyNameBox() const;
    Rectangle<float> historyForwardButton() const;
    Rectangle<float> randomCountBox() const;
    Rectangle<float> bankNameBox() const;
    Rectangle<float> makeBankButton() const;
    Rectangle<float> tableOptionBox(int option) const;
    Rectangle<float> tableTargetButton(int index) const;
    Rectangle<float> tableExportButton() const;
    Rectangle<float> statusArea() const;
    int numTypeRows() const;

    String optionValue(int option) const;
    void stepOption(int option, int delta);
    String tableOptionValue(int option) const;
    void stepTableOption(int option, int delta);

    bool loadPreset(const File& file);
    corpuscular::gen::GeneratedWavetable makeWavetable() const;
    File saveWavetableToLibrary(corpuscular::gen::GeneratedWavetable& table);
    void generateOne(bool with_wavetables);
    // VOCODER option: ON gives every preset the vocoder, SOME about one in four
    void maybeAddVocoder(nlohmann::json& preset, Random& random) const;
    void makeSimilar();

    // History of the presets made here (also kept in a file, so it outlives the window)
    File historyFile() const;
    void loadHistory();
    void saveHistory();
    void addToHistory(const File& preset);
    void stepHistory(int delta);
    void makeBank();
    void wavetableToOscillator(int index);
    void exportWavetable();
    void newBankName();
    void setStatus(const String& text);

    void paintButton(Graphics& g, Rectangle<float> bounds, const String& text, bool lit, float font_height);
    void paintStepper(Graphics& g, Rectangle<float> bounds, const String& title, const String& value, float font_height);

    std::function<void(int, const File&)> wavetable_loader_;
    std::function<void()> bank_viewer_;

    std::vector<std::string> types_;
    std::vector<int> bank_counts_;
    int selected_type_;
    int random_count_;
    int scale_group_;
    int scale_;
    int melody_;
    int step_;
    int vocoder_;
    String bank_name_;

    int table_type_;
    int table_frames_;
    int table_size_;
    bool table_smooth_;
    int table_format_;

    std::vector<File> history_;
    int history_index_;

    String status_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneratorSection)
};
