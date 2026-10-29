/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "vocoder_section.h"

#include "bar_renderer.h"
#include "fonts.h"
#include "load_save.h"
#include "open_gl_line_renderer.h"
#include "qv_synth_vocoder.h"
#include "skin.h"
#include "sound_engine.h"
#include "synth_button.h"
#include "synth_gui_interface.h"
#include "synth_slider.h"
#include "synth_strings.h"
#include "text_look_and_feel.h"
#include "text_selector.h"
#include "vocoder_module.h"

namespace {
  const std::string kVocoderTableExtensions = "*.vitaltable;*.wav;*.flac;*.wt";

  // Indexed by MIDI note, so the lookup matches the vocoder_note value directly
  const std::string* noteNames() {
    static std::string names[128];
    if (names[0].empty()) {
      static const char* kLetters[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
      for (int i = 0; i < 128; ++i)
        names[i] = std::string(kLetters[i % 12]) + std::to_string(i / 12 - 1);
    }
    return names;
  }

  vital::VocoderModule* findVocoder(Component* component) {
    SynthGuiInterface* parent = component->findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || parent->getSynth() == nullptr || parent->getSynth()->getEngine() == nullptr)
      return nullptr;
    return parent->getSynth()->getEngine()->getVocoder();
  }
}

// The carrier table's frame at POSITION, drawn as a waveform
class VocoderTableViewer : public OpenGlLineRenderer {
  public:
    static constexpr int kResolution = 128;

    VocoderTableViewer() : OpenGlLineRenderer(kResolution), active_(true), position_(nullptr) {
      setFill(true);
      addRoundedCorners();
    }

    // Message thread; the render thread reads it with atomic_load
    void setTable(std::shared_ptr<const wv::Wavetable> table) { std::atomic_store(&table_, table); }
    void setPositionSlider(SynthSlider* position) { position_ = position; }
    void setActive(bool active) { active_ = active; }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      std::shared_ptr<const wv::Wavetable> table = std::atomic_load(&table_);
      if (table == nullptr || table->getNumFrames() <= 0)
        return;

      float position = position_ ? (float)position_->getValue() : 0.0f;
      int frame = vital::utils::iclamp((int)std::round(position * (table->getNumFrames() - 1)), 0,
                                       table->getNumFrames() - 1);
      const float* samples = table->data(frame, 0);

      float peak = 0.0f;
      for (int i = 0; i < wv::Wavetable::frameSize; ++i)
        peak = std::max(peak, std::abs(samples[i]));
      float scale = peak > 0.0f ? 0.8f / peak : 0.0f;

      float width = getWidth();
      float half_height = getHeight() / 2.0f;
      for (int i = 0; i < kResolution; ++i) {
        float t = i / (kResolution - 1.0f);
        float sample = samples[(int)(t * wv::Wavetable::frameSize)];
        setXAt(i, t * width);
        setYAt(i, (1.0f - sample * scale) * half_height);
      }

      setLineWidth(findValue(Skin::kWidgetLineWidth));
      setFillCenter(findValue(Skin::kWidgetFillCenter));
      Colour color = findColour(active_ ? Skin::kWidgetPrimary1 : Skin::kWidgetPrimaryDisabled, true);
      Colour fill = findColour(active_ ? Skin::kWidgetSecondary1 : Skin::kWidgetSecondaryDisabled, true);
      setColor(color);
      setFillColors(fill.withMultipliedAlpha(0.3f), fill);
      OpenGlLineRenderer::render(open_gl, animate);
      renderCorners(open_gl, animate);
    }

  private:
    bool active_;
    SynthSlider* position_;
    std::shared_ptr<const wv::Wavetable> table_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocoderTableViewer)
};

// Live band levels of the voice, one bar per band
class VocoderBandViewer : public BarRenderer {
  public:
    static constexpr float kRangeDb = 60.0f;

    VocoderBandViewer() : BarRenderer(wv::SynthVocoder::maxBands, true), active_(true) {
      setBarWidth(0.7f);
      setScale(1.0f);
      setAdditiveBlending(false);
    }

    void parentHierarchyChanged() override {
      vocoder_ = nullptr;
      if (vital::VocoderModule* module = findVocoder(this))
        vocoder_ = module->getVocoder();
    }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      if (vocoder_ == nullptr)
        parentHierarchyChanged();

      int num_bands = vocoder_ ? vital::utils::iclamp(vocoder_->getNumBands(), 1, wv::SynthVocoder::maxBands) : 1;
      setNumPoints(num_bands);
      for (int i = 0; i < num_bands; ++i) {
        float level = vocoder_ ? vocoder_->getBandLevel(i) : 0.0f;
        float db = 20.0f * std::log10(std::max(level, 1e-6f));
        float height = vital::utils::clamp((db + kRangeDb) / kRangeDb, 0.0f, 1.0f);
        setX(i, 2.0f * (i + 0.15f) / num_bands - 1.0f);
        setY(i, 2.0f * height - 1.0f);
        setBottom(i, -1.0f);
      }

      // Translucent, so the carrier's waveform stays visible underneath
      if (active_)
        setColor(findColour(Skin::kWidgetPrimary2, true).withMultipliedAlpha(0.55f));
      else
        setColor(findColour(Skin::kWidgetPrimaryDisabled, true).withMultipliedAlpha(0.55f));

      BarRenderer::render(open_gl, animate);
      renderCorners(open_gl, animate);
    }

    void setActive(bool active) { active_ = active; }

  private:
    bool active_;
    std::shared_ptr<wv::SynthVocoder> vocoder_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocoderBandViewer)
};

VocoderSection::VocoderSection(const String& name) : SynthSection(name) {
  static constexpr double kTextDragSensitivity = 0.5;

  carrier_ = std::make_unique<TextSelector>("vocoder_carrier");
  addSlider(carrier_.get());
  carrier_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  carrier_->setLookAndFeel(TextLookAndFeel::instance());
  carrier_->setLongStringLookup(strings::kVocoderCarrierNames);

  pitch_mode_ = std::make_unique<TextSelector>("vocoder_pitch_mode");
  addSlider(pitch_mode_.get());
  pitch_mode_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  pitch_mode_->setLookAndFeel(TextLookAndFeel::instance());
  pitch_mode_->setLongStringLookup(strings::kVocoderPitchModeNames);

  note_ = std::make_unique<SynthSlider>("vocoder_note");
  addSlider(note_.get());
  note_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  note_->setLookAndFeel(TextLookAndFeel::instance());
  note_->setStringLookup(noteNames());
  note_->setSensitivity(kTextDragSensitivity);

  octave_ = std::make_unique<SynthSlider>("vocoder_octave");
  addSlider(octave_.get());
  octave_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  octave_->setLookAndFeel(TextLookAndFeel::instance());
  octave_->setSensitivity(kTextDragSensitivity);

  tilt_ = std::make_unique<SynthSlider>("vocoder_tilt");
  addSlider(tilt_.get());
  tilt_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tilt_->setBipolar();
  tilt_->snapToValue(true);

  spread_ = std::make_unique<SynthSlider>("vocoder_spread");
  addSlider(spread_.get());
  spread_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  freeze_ = std::make_unique<TextSelector>("vocoder_freeze");
  addSlider(freeze_.get());
  freeze_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  freeze_->setLookAndFeel(TextLookAndFeel::instance());
  freeze_->setLongStringLookup(strings::kOffOnNames);

  table_selector_ = std::make_unique<PresetSelector>();
  addSubSection(table_selector_.get());
  table_selector_->addListener(this);
  table_selector_->setText("Init");

  table_viewer_ = std::make_unique<VocoderTableViewer>();
  addOpenGlComponent(table_viewer_.get());

  band_viewer_ = std::make_unique<VocoderBandViewer>();
  addOpenGlComponent(band_viewer_.get());

  position_ = std::make_unique<SynthSlider>("vocoder_position");
  addSlider(position_.get());
  position_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  table_viewer_->setPositionSlider(position_.get());

  detune_ = std::make_unique<SynthSlider>("vocoder_detune");
  addSlider(detune_.get());
  detune_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  glide_ = std::make_unique<SynthSlider>("vocoder_glide");
  addSlider(glide_.get());
  glide_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  unison_ = std::make_unique<SynthSlider>("vocoder_unison");
  addSlider(unison_.get());
  unison_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  unison_->setLookAndFeel(TextLookAndFeel::instance());
  unison_->setSensitivity(kTextDragSensitivity);

  bands_ = std::make_unique<SynthSlider>("vocoder_bands");
  addSlider(bands_.get());
  bands_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  bands_->setLookAndFeel(TextLookAndFeel::instance());
  bands_->setSensitivity(kTextDragSensitivity);

  dry_wet_ = std::make_unique<SynthSlider>("vocoder_dry_wet");
  addSlider(dry_wet_.get());
  dry_wet_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  level_ = std::make_unique<SynthSlider>("vocoder_level");
  addSlider(level_.get());
  level_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  level_->setBipolar();
  level_->snapToValue(true);

  width_ = std::make_unique<SynthSlider>("vocoder_width");
  addSlider(width_.get());
  width_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  formant_ = std::make_unique<SynthSlider>("vocoder_formant");
  addSlider(formant_.get());
  formant_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  formant_->setBipolar();
  formant_->snapToValue(true);

  attack_ = std::make_unique<SynthSlider>("vocoder_attack");
  addSlider(attack_.get());
  attack_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  release_ = std::make_unique<SynthSlider>("vocoder_release");
  addSlider(release_.get());
  release_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  air_ = std::make_unique<SynthSlider>("vocoder_air");
  addSlider(air_.get());
  air_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  noise_ = std::make_unique<SynthSlider>("vocoder_noise");
  addSlider(noise_.get());
  noise_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  gate_ = std::make_unique<SynthSlider>("vocoder_gate");
  addSlider(gate_.get());
  gate_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  on_ = std::make_unique<SynthButton>("vocoder_on");
  addButton(on_.get());
  setActivator(on_.get());
  setSkinOverride(Skin::kVocoder);
}

VocoderSection::~VocoderSection() = default;

void VocoderSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  for (Component* text : std::initializer_list<Component*>{ carrier_.get(), pitch_mode_.get(), note_.get(),
                                                            octave_.get(), freeze_.get(), unison_.get(),
                                                            bands_.get() })
    drawTextComponentBackground(g, text->getBounds(), true);

  setLabelFont(g);
  drawLabelForComponent(g, TRANS("CARRIER"), carrier_.get(), true);
  drawLabelForComponent(g, TRANS("PITCH"), pitch_mode_.get(), true);
  drawLabelForComponent(g, TRANS("NOTE"), note_.get(), true);
  drawLabelForComponent(g, TRANS("OCTAVE"), octave_.get(), true);
  drawLabelForComponent(g, TRANS("TILT"), tilt_.get());
  drawLabelForComponent(g, TRANS("SPREAD"), spread_.get());
  drawLabelForComponent(g, TRANS("FREEZE"), freeze_.get(), true);

  drawLabelForComponent(g, TRANS("POS"), position_.get());
  drawLabelForComponent(g, TRANS("DETUNE"), detune_.get());
  drawLabelForComponent(g, TRANS("GLIDE"), glide_.get());
  drawLabelForComponent(g, TRANS("UNISON"), unison_.get(), true);
  drawLabelForComponent(g, TRANS("BANDS"), bands_.get(), true);
  drawLabelForComponent(g, TRANS("MIX"), dry_wet_.get());
  drawLabelForComponent(g, TRANS("LEVEL"), level_.get());

  drawLabelForComponent(g, TRANS("WIDTH"), width_.get());
  drawLabelForComponent(g, TRANS("FORM"), formant_.get());
  drawLabelForComponent(g, TRANS("ATTACK"), attack_.get());
  drawLabelForComponent(g, TRANS("RELEASE"), release_.get());
  drawLabelForComponent(g, TRANS("AIR"), air_.get());
  drawLabelForComponent(g, TRANS("NOISE"), noise_.get());
  drawLabelForComponent(g, TRANS("GATE"), gate_.get());
}

void VocoderSection::resized() {
  static constexpr float kViewerPercent = 0.24f;

  int widget_margin = findValue(Skin::kWidgetMargin);
  int title_width = getTitleWidth();
  int section_height = getKnobSectionHeight();
  int row_step = section_height - widget_margin;

  // Left: the carrier table and the band meter; right: three rows of controls
  Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
  int viewer_x = bounds.getX() + widget_margin;
  int viewer_width = bounds.getWidth() * kViewerPercent;
  table_selector_->setBounds(viewer_x, widget_margin, viewer_width, title_width - 2 * widget_margin);
  table_selector_->setRoundAmount(findValue(Skin::kBodyRounding));
  int viewer_y = table_selector_->getBottom() + widget_margin;
  band_viewer_->setBounds(viewer_x, viewer_y, viewer_width, getHeight() - viewer_y - widget_margin);
  table_viewer_->setBounds(band_viewer_->getBounds());

  int knobs_x = viewer_x + viewer_width;
  int knobs_width = getWidth() - knobs_x;

  placeKnobsInArea(Rectangle<int>(knobs_x, 0, knobs_width, section_height),
                   { carrier_.get(), pitch_mode_.get(), note_.get(), octave_.get(), unison_.get(),
                     bands_.get(), freeze_.get() });
  for (Component* text : std::initializer_list<Component*>{ carrier_.get(), pitch_mode_.get(), note_.get(),
                                                            octave_.get(), unison_.get(), bands_.get(),
                                                            freeze_.get() })
    text->setBounds(text->getBounds().withTop(widget_margin));

  placeKnobsInArea(Rectangle<int>(knobs_x, row_step, knobs_width, section_height),
                   { position_.get(), detune_.get(), glide_.get(), width_.get(), formant_.get(),
                     dry_wet_.get(), level_.get() });
  placeKnobsInArea(Rectangle<int>(knobs_x, 2 * row_step, knobs_width, section_height),
                   { attack_.get(), release_.get(), air_.get(), noise_.get(), gate_.get(),
                     tilt_.get(), spread_.get() });

  SynthSection::resized();
}

void VocoderSection::reset() {
  SynthSection::reset();
  updateTableName();
}

void VocoderSection::setActive(bool active) {
  SynthSection::setActive(active);
  table_viewer_->setActive(active);
  band_viewer_->setActive(active);
}

void VocoderSection::prevClicked() {
  if (current_file_.existsAsFile()) {
    File file = LoadSave::getShiftedFile(LoadSave::kWavetableFolderName, kVocoderTableExtensions,
                                         LoadSave::kAdditionalWavetableFoldersName, current_file_, -1);
    if (file.exists())
      loadFile(file);
  }
  else {
    vital::VocoderModule* vocoder = findVocoder(this);
    int index = vocoder && vocoder->getTable() ? vocoder->getTable()->builtInIndex : 0;
    int total = wv::Wavetable::numBuiltIns();
    setTable(wv::Wavetable::builtIn((index - 1 + total) % total));
  }
  updatePopupBrowser(this);
}

void VocoderSection::nextClicked() {
  if (current_file_.existsAsFile()) {
    File file = LoadSave::getShiftedFile(LoadSave::kWavetableFolderName, kVocoderTableExtensions,
                                         LoadSave::kAdditionalWavetableFoldersName, current_file_, 1);
    if (file.exists())
      loadFile(file);
  }
  else {
    vital::VocoderModule* vocoder = findVocoder(this);
    int index = vocoder && vocoder->getTable() ? vocoder->getTable()->builtInIndex : 0;
    setTable(wv::Wavetable::builtIn((index + 1) % wv::Wavetable::numBuiltIns()));
  }
  updatePopupBrowser(this);
}

void VocoderSection::textMouseDown(const MouseEvent& e) {
  static constexpr int kBrowserWidth = 600;
  static constexpr int kBrowserHeight = 400;
  Rectangle<int> bounds(table_selector_->getX(), table_selector_->getY(),
                        kBrowserWidth * size_ratio_, kBrowserHeight * size_ratio_);
  showPopupBrowser(this, bounds, LoadSave::getWavetableDirectories(), kVocoderTableExtensions,
                   LoadSave::kWavetableFolderName, LoadSave::kAdditionalWavetableFoldersName);
}

void VocoderSection::loadFile(const File& file) {
  juce::String error;
  std::unique_ptr<wv::Wavetable> table = wv::Wavetable::loadFile(file, error);
  if (table)
    setTable(std::move(table));
}

void VocoderSection::setTable(std::unique_ptr<wv::Wavetable> table) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  vital::VocoderModule* vocoder = findVocoder(this);
  if (table == nullptr || parent == nullptr || vocoder == nullptr)
    return;

  std::shared_ptr<const wv::Wavetable> shared(table.release());
  {
    ScopedLock lock(parent->getSynth()->getCriticalSection());
    vocoder->setTable(shared);
  }
  updateTableName();
}

void VocoderSection::updateTableName() {
  vital::VocoderModule* vocoder = findVocoder(this);
  if (vocoder == nullptr || vocoder->getTable() == nullptr)
    return;

  std::shared_ptr<const wv::Wavetable> table = vocoder->getTable();
  current_file_ = table->file;
  table_selector_->setText(table->name);
  table_viewer_->setTable(table);
}
