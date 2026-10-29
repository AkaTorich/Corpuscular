/* Copyright 2013-2019 Matt Tytel
 * Modified for Corpuscular, 2026.
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "full_interface.h"

#include "about_section.h"
#include "arp_section.h"
#include "bank_exporter.h"
#include "bend_section.h"
#include "corpuscular_library.h"
#include "delete_section.h"
#include "download_section.h"
#include "expired_section.h"
#include "extra_mod_section.h"
#include "skin.h"
#include "effects_interface.h"
#include "fonts.h"
#include "sound_engine.h"
#include "keyboard_interface.h"
#include "load_save.h"
#include "master_controls_interface.h"
#include "modulation_interface.h"
#include "modulation_manager.h"
#include "modulation_matrix.h"
#include "modulation_meter.h"
#include "wavetable_edit_section.h"
#include "overlay.h"
#include "portamento_section.h"
#include "generator_section.h"
#include "library_progress_section.h"
#include "preset_browser.h"
#include "routing_section.h"
#include "synthesis_interface.h"
#include "synth_gui_interface.h"
#include "text_look_and_feel.h"
#include "update_check_section.h"
#include "voice_section.h"

FullInterface::FullInterface(SynthGuiData* synth_data) : SynthSection("full_interface"), width_(0), resized_width_(0),
                                                         last_render_scale_(0.0f), display_scale_(1.0f),
                                                         pixel_multiple_(1), setting_all_values_(false),
                                                         unsupported_(false), animate_(true),
                                                         enable_redo_background_(true), needs_download_(false),
                                                         open_gl_(open_gl_context_) {
  full_screen_section_ = nullptr;
  Skin default_skin;
  setSkinValues(default_skin, true);
  default_skin.copyValuesToLookAndFeel(DefaultLookAndFeel::instance());

  bool synth = synth_data->wavetable_creators[0];
  if (synth) {
    synthesis_interface_ = std::make_unique<SynthesisInterface>(&auth_,
                                                                synth_data->mono_modulations,
                                                                synth_data->poly_modulations);

    for (int i = 0; i < vital::kNumOscillators; ++i) {
      wavetable_edits_[i] = std::make_unique<WavetableEditSection>(i, synth_data->wavetable_creators[i]);
      addSubSection(wavetable_edits_[i].get());
      wavetable_edits_[i]->setVisible(false);
      wavetable_edits_[i]->setWaveFrameSlider(synthesis_interface_->getWaveFrameSlider(i));
    }

    addSubSection(synthesis_interface_.get());
    synthesis_interface_->setDisplayMode(SynthesisInterface::kSources);
  }

  routing_section_ = std::make_unique<RoutingSection>(synthesis_interface_.get());
  addSubSection(routing_section_.get());
  routing_section_->setVisible(false);

  generator_section_ = std::make_unique<GeneratorSection>();
  addSubSection(generator_section_.get());
  generator_section_->setVisible(false);

  arp_section_ = std::make_unique<ArpSection>();
  addSubSection(arp_section_.get());
  arp_section_->setVisible(false);

  effects_interface_ = std::make_unique<EffectsInterface>(synth_data->mono_modulations);
  addSubSection(effects_interface_.get());
  effects_interface_->setVisible(false);
  effects_interface_->addListener(this);

  master_controls_interface_ = std::make_unique<MasterControlsInterface>(synth_data->mono_modulations,
                                                                         synth_data->poly_modulations,
                                                                         synth);
  addSubSection(master_controls_interface_.get());
  master_controls_interface_->setVisible(false);

  if (synthesis_interface_) {
    for (int i = 0; i < vital::kNumOscillators; ++i)
      master_controls_interface_->passOscillatorSection(i, synthesis_interface_->getOscillatorSection(i));
  }

  header_ = std::make_unique<HeaderSection>();
  addSubSection(header_.get());
  header_->addListener(this);

  modulation_interface_ = std::make_unique<ModulationInterface>(synth_data);
  addSubSection(modulation_interface_.get());

  extra_mod_section_ = std::make_unique<ExtraModSection>("extra_mod_section", synth_data);
  addSubSection(extra_mod_section_.get());

  keyboard_interface_ = std::make_unique<KeyboardInterface>(synth_data->synth->getKeyboardState());
  addSubSection(keyboard_interface_.get());

  library_progress_ = std::make_unique<LibraryProgressSection>();
  addSubSection(library_progress_.get());
  library_progress_->setVisible(false);

  bend_section_ = std::make_unique<BendSection>("BEND");
  addSubSection(bend_section_.get());

  portamento_section_ = std::make_unique<PortamentoSection>("PORTAMENTO");
  addSubSection(portamento_section_.get());

  voice_section_ = std::make_unique<VoiceSection>("VOICE");
  addSubSection(voice_section_.get());

  modulation_matrix_ = std::make_unique<ModulationMatrix>(synth_data->modulation_sources, synth_data->mono_modulations);
  addSubSection(modulation_matrix_.get());
  modulation_matrix_->setVisible(false);
  modulation_matrix_->addListener(this);
  createModulationSliders(synth_data->mono_modulations, synth_data->poly_modulations);

  preset_browser_ = std::make_unique<PresetBrowser>();
  addSubSection(preset_browser_.get());
  preset_browser_->setVisible(false);

  popup_browser_ = std::make_unique<PopupBrowser>();
  addSubSection(popup_browser_.get());
  popup_browser_->setVisible(false);

  popup_selector_ = std::make_unique<SinglePopupSelector>();
  addSubSection(popup_selector_.get());
  popup_selector_->setVisible(false);
  popup_selector_->setAlwaysOnTop(true);
  popup_selector_->setWantsKeyboardFocus(true);

  dual_popup_selector_ = std::make_unique<DualPopupSelector>();
  addSubSection(dual_popup_selector_.get());
  dual_popup_selector_->setVisible(false);
  dual_popup_selector_->setAlwaysOnTop(true);
  dual_popup_selector_->setWantsKeyboardFocus(true);

  popup_display_1_ = std::make_unique<PopupDisplay>();
  addSubSection(popup_display_1_.get());
  popup_display_1_->setVisible(false);
  popup_display_1_->setAlwaysOnTop(true);
  popup_display_1_->setWantsKeyboardFocus(false);

  popup_display_2_ = std::make_unique<PopupDisplay>();
  addSubSection(popup_display_2_.get());
  popup_display_2_->setVisible(false);
  popup_display_2_->setAlwaysOnTop(true);
  popup_display_2_->setWantsKeyboardFocus(false);

  bank_exporter_ = std::make_unique<BankExporter>();
  addSubSection(bank_exporter_.get());
  bank_exporter_->setVisible(false);
  header_->setBankExporter(bank_exporter_.get());

  save_section_ = std::make_unique<SaveSection>("save_section");
  addSubSection(save_section_.get(), false);
  addChildComponent(save_section_.get());
  preset_browser_->setSaveSection(save_section_.get());
  header_->setSaveSection(save_section_.get());
  header_->setBrowser(preset_browser_.get());

  // Corpuscular: the ARP tab hears about saved arps to show the new name
  save_section_->addSaveListener(arp_section_.get());

  // Corpuscular: whatever the generator makes shows up right away
  generator_section_->setWavetableLoader([this](int index, const File& file) {
    if (synthesis_interface_ == nullptr)
      return;
    header_->selectTab(kOscTab);
    synthesis_interface_->getOscillatorSection(index)->loadFile(file);

    std::map<std::string, ToggleButton*> buttons = synthesis_interface_->getAllButtons();
    auto on = buttons.find("osc_" + std::to_string(index + 1) + "_on");
    if (on != buttons.end() && !on->second->getToggleState())
      on->second->setToggleState(true, sendNotificationSync);
  });
  generator_section_->setBankViewer([this]() {
    preset_browser_->loadPresets();
    header_->setPresetBrowserVisibility(true);
  });

  // Corpuscular: the first launch fills the data folder with the generated library in the background
  Component::SafePointer<FullInterface> safe_this(this);
  corpuscular::ensureLibrary([safe_this]() {
    if (FullInterface* full_interface = safe_this.getComponent())
      full_interface->dataDirectoryChanged();
  });
  if (corpuscular::libraryBuilding()) {
    keyboard_interface_->setVisible(false);
    library_progress_->setFinishedCallback([this]() {
      keyboard_interface_->setVisible(true);
      redoBackground();
    });
    library_progress_->start();
  }

  delete_section_ = std::make_unique<DeleteSection>("delete_section");
  addSubSection(delete_section_.get(), false);
  addChildComponent(delete_section_.get());
  preset_browser_->setDeleteSection(delete_section_.get());

  download_section_ = std::make_unique<DownloadSection>("download_section", &auth_);
  addSubSection(download_section_.get(), false);
  addChildComponent(download_section_.get());
  download_section_->setAlwaysOnTop(true);
  download_section_->addListener(this);

  about_section_ = std::make_unique<AboutSection>("about");
  addSubSection(about_section_.get(), false);
  addChildComponent(about_section_.get());

  if (synthesis_interface_)
    synthesis_interface_->toFront(true);

  routing_section_->toFront(true);
  generator_section_->toFront(true);
  arp_section_->toFront(true);
  master_controls_interface_->toFront(true);
  effects_interface_->toFront(true);
  modulation_interface_->toFront(true);
  extra_mod_section_->toFront(true);
  keyboard_interface_->toFront(true);
  bend_section_->toFront(true);
  portamento_section_->toFront(true);
  voice_section_->toFront(true);
  modulation_manager_->toFront(false);
  preset_browser_->toFront(false);
  bank_exporter_->toFront(false);
  about_section_->toFront(true);
  save_section_->toFront(true);
  delete_section_->toFront(true);
  popup_browser_->toFront(true);
  popup_selector_->toFront(true);
  dual_popup_selector_->toFront(true);
  popup_display_1_->toFront(true);
  popup_display_2_->toFront(true);
  download_section_->toFront(true);

  update_check_section_ = std::make_unique<UpdateCheckSection>("update_check");
  addSubSection(update_check_section_.get(), false);
  addChildComponent(update_check_section_.get());
  update_check_section_->setAlwaysOnTop(true);
  update_check_section_->addListener(this);

  if (false) { // Corpuscular: no expiry
    expired_section_ = std::make_unique<ExpiredSection>("expired");
    addSubSection(expired_section_.get());
    expired_section_->setAlwaysOnTop(true);
  }

#if NDEBUG && !NO_AUTH
  bool authenticated = LoadSave::authenticated();
  bool work_offline = LoadSave::shouldWorkOffline();
  authentication_ = std::make_unique<AuthenticationSection>(&auth_);
  authentication_->addListener(this);
  addSubSection(authentication_.get(), false);
  addChildComponent(authentication_.get());
  authentication_->setVisible(!authenticated && !work_offline);
  authentication_->init();
  if (!work_offline)
    authentication_->create();
#endif

  setAllValues(synth_data->controls);
  setOpaque(true);
  setSkinValues(default_skin, true);

  UpdateMemory::getInstance()->incrementChecker();
  needs_download_ = false; // Corpuscular: nothing to download

  open_gl_context_.setContinuousRepainting(true);
  open_gl_context_.setOpenGLVersionRequired(OpenGLContext::openGL3_2);
  open_gl_context_.setSwapInterval(0);
  open_gl_context_.setRenderer(this);
  open_gl_context_.setComponentPaintingEnabled(false);
  open_gl_context_.attachTo(*this);
}

FullInterface::FullInterface() : SynthSection("EMPTY"), open_gl_(open_gl_context_) {
  Skin default_skin;
  setSkinValues(default_skin, true);

  open_gl_context_.setContinuousRepainting(true);
  open_gl_context_.setOpenGLVersionRequired(OpenGLContext::openGL3_2);
  open_gl_context_.setSwapInterval(0);
  open_gl_context_.setRenderer(this);
  open_gl_context_.setComponentPaintingEnabled(false);
  open_gl_context_.attachTo(*this);

  reset();
  setOpaque(true);
}

FullInterface::~FullInterface() {
  UpdateMemory::getInstance()->decrementChecker();

  open_gl_context_.detach();
  open_gl_context_.setRenderer(nullptr);
}

void FullInterface::paintBackground(Graphics& g) {
  g.fillAll(findColour(Skin::kBackground, true));
  paintChildrenShadows(g);

  if (effects_interface_ == nullptr)
    return;

  int padding = getPadding();
  int bar_width = 6 * padding;
  g.setColour(header_->findColour(Skin::kBody, true));
  int y = header_->getBottom();
  int height = keyboard_interface_->getY() - y;
  int x1 = extra_mod_section_->getRight() + padding;
  g.fillRect(x1, y, bar_width, height);

  bool wide_tab = current_tab_ == kAmpTab || current_tab_ == kModTab || current_tab_ == kArpTab;
  if (!wide_tab) {
    int x2 = main_bounds_.getRight() + padding;
    g.fillRect(x2, y, bar_width, height);
  }

  // Corpuscular: wooden side panels like the hardware
  Colour side = findColour(Skin::kSidePanel, true);
  if (side.getAlpha() && !left_side_.isEmpty()) {
    for (const Rectangle<int>& panel : { left_side_, right_side_ }) {
      ColourGradient grain(side.brighter(0.25f), panel.getX(), 0.0f,
                           side.darker(0.45f), panel.getRight(), 0.0f, false);
      grain.addColour(0.5, side);
      g.setGradientFill(grain);
      g.fillRect(panel);
      int step = 0;
      for (int line = panel.getX() + 2; line < panel.getRight() - 1; line += 3, ++step) {
        g.setColour(side.darker(0.6f).withAlpha(step % 3 ? 0.12f : 0.3f));
        g.drawVerticalLine(line, panel.getY(), panel.getBottom());
      }
    }
  }

  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
}

void FullInterface::copySkinValues(const Skin& skin) {
  ScopedLock open_gl_lock(open_gl_critical_section_);
  skin.copyValuesToLookAndFeel(DefaultLookAndFeel::instance());
  setSkinValues(skin, true);
}

void FullInterface::reloadSkin(const Skin& skin) {
  copySkinValues(skin);
  Rectangle<int> bounds = getBounds();
  setBounds(0, 0, bounds.getWidth() / 4, bounds.getHeight() / 4);
  setBounds(bounds);
}

void FullInterface::dataDirectoryChanged() {
  preset_browser_->loadPresets();
}

void FullInterface::noDownloadNeeded() {
  // Corpuscular: no update checks
}

void FullInterface::needsUpdate() {
  // Corpuscular: no update checks
}

void FullInterface::loggedIn() {
#if !defined(NO_TEXT_ENTRY)
  // Corpuscular: no downloads
#endif
}

void FullInterface::repaintChildBackground(SynthSection* child) {
  if (!background_image_.isValid() || setting_all_values_)
    return; 

  if (child->getParentComponent() == synthesis_interface_.get()) {
    repaintSynthesisSection();
    return;
  }

  if (effects_interface_ != nullptr && effects_interface_->isParentOf(child))
    child = effects_interface_.get();

  background_.lock();
  Graphics g(background_image_);
  paintChildBackground(g, child);
  background_.updateBackgroundImage(background_image_);
  background_.unlock();
}

void FullInterface::repaintSynthesisSection() {
  if (synthesis_interface_ == nullptr || !synthesis_interface_->isVisible() || !background_image_.isValid())
    return;

  background_.lock();
  Graphics g(background_image_);
  int padding = findValue(Skin::kPadding);
  g.setColour(findColour(Skin::kBackground, true));
  g.fillRect(synthesis_interface_->getBounds().expanded(padding));
  paintChildShadow(g, synthesis_interface_.get());
  paintChildBackground(g, synthesis_interface_.get());

  background_.updateBackgroundImage(background_image_);
  background_.unlock();
}

void FullInterface::repaintOpenGlBackground(OpenGlComponent* component) {
  if (!background_image_.isValid())
    return;

  background_.lock();
  Graphics g(background_image_);
  paintOpenGlBackground(g, component);
  background_.updateBackgroundImage(background_image_);
  background_.unlock();
}

void FullInterface::redoBackground() {
  int width = std::ceil(display_scale_ * getWidth());
  int height = std::ceil(display_scale_ * getHeight());
  if (width < vital::kMinWindowWidth || height < vital::kMinWindowHeight)
    return;

  ScopedLock open_gl_lock(open_gl_critical_section_);

  background_.lock();
  background_image_ = Image(Image::RGB, width, height, true);
  Graphics g(background_image_);
  paintBackground(g);
  background_.updateBackgroundImage(background_image_);
  background_.unlock();
}

void FullInterface::checkShouldReposition(bool resize) {
  float old_scale = display_scale_;
  int old_pixel_multiple = pixel_multiple_;
  display_scale_ = getDisplayScale();
  pixel_multiple_ = std::max<int>(1, display_scale_);

  if (resize && (old_scale != display_scale_ || old_pixel_multiple != pixel_multiple_))
    resized();
}

void FullInterface::resized() {
  checkShouldReposition(false);

  width_ = getWidth();
  if (!enable_redo_background_)
    return;

  resized_width_ = width_;

  ScopedLock lock(open_gl_critical_section_);
  static constexpr int kTopHeight = 48;
  static constexpr int kSideWidth = 16;

  if (effects_interface_ == nullptr)
    return;

  int left = 0;
  int top = 0;
  int width = std::ceil(getWidth() * display_scale_);
  int height = std::ceil(getHeight() * display_scale_);
  Rectangle<int> bounds(0, 0, width, height);

  float width_ratio = getWidth() / (1.0f * vital::kDefaultWindowWidth);
  float ratio = width_ratio * display_scale_;
  float height_ratio = getHeight() / (1.0f * vital::kDefaultWindowHeight);
  if (width_ratio > height_ratio + 1.0f / vital::kDefaultWindowHeight) {
    ratio = height_ratio;
    width = height_ratio * vital::kDefaultWindowWidth * display_scale_;
    left = (getWidth() - width) / 2;
  }
  if (height_ratio > width_ratio + 1.0f / vital::kDefaultWindowHeight) {
    ratio = width_ratio;
    height = ratio * vital::kDefaultWindowHeight * display_scale_;
    top = (getHeight() - height) / 2;
  }

  setSizeRatio(ratio);

  if (expired_section_)
    expired_section_->setBounds(bounds);

  if (authentication_)
    authentication_->setBounds(bounds);

  popup_browser_->setBounds(bounds);

  int padding = getPadding();
  int voice_padding = findValue(Skin::kLargePadding);
  int extra_mod_width = findValue(Skin::kModulationButtonWidth);

  // Corpuscular: wooden side panels take the outer edges, everything else moves inside
  int side_width = 0;
  if (findColour(Skin::kSidePanel, true).getAlpha())
    side_width = kSideWidth * ratio;
  left_side_ = Rectangle<int>(left, top, side_width, height);
  right_side_ = Rectangle<int>(left + width - side_width, top, side_width, height);
  left += side_width;
  width -= 2 * side_width;

  int main_x = left + extra_mod_width + 2 * voice_padding;
  int top_height = kTopHeight * ratio;

  int knob_section_height = getKnobSectionHeight();
  int keyboard_section_height = knob_section_height * 0.7f;
  int voice_height = height - top_height - keyboard_section_height;

  int section_one_width = 350 * ratio;
  int section_two_width = section_one_width;
  int audio_width = section_one_width + section_two_width + padding;
  int modulation_width = width - audio_width - extra_mod_width - 4 * voice_padding;

  header_->setTabOffset(extra_mod_width + 2 * voice_padding);
  header_->setBounds(left, top, width, top_height);
  Rectangle<int> main_bounds(main_x, top + top_height, audio_width, voice_height);
  main_bounds_ = main_bounds;

  if (synthesis_interface_)
    synthesis_interface_->setBounds(main_bounds);
  routing_section_->setBounds(main_bounds);
  generator_section_->setBounds(main_bounds);
  effects_interface_->setBounds(main_bounds.withRight(main_bounds.getRight() + voice_padding));
  modulation_matrix_->setBounds(main_bounds);
  int modulation_height = voice_height - knob_section_height - padding;
  column_bounds_ = Rectangle<int>(main_bounds.getRight() + voice_padding, main_bounds.getY(),
                                  modulation_width, modulation_height);

  int voice_y = top + height - knob_section_height - keyboard_section_height;
  voice_row_ = Rectangle<int>(column_bounds_.getX(), voice_y, modulation_width, knob_section_height);
  arp_section_->setBounds(main_bounds.withRight(column_bounds_.getRight()));
  layoutTab();

  bend_section_->setBounds(left + voice_padding, top + height - knob_section_height - padding,
                           extra_mod_width, knob_section_height);

  int extra_mod_height = height - top_height - knob_section_height - padding - 1;
  extra_mod_section_->setBounds(left + voice_padding, top + top_height, extra_mod_width, extra_mod_height);

  int keyboard_height = keyboard_section_height - voice_padding - padding;
  int keyboard_x = extra_mod_section_->getRight() + voice_padding;
  int keyboard_width = column_bounds_.getRight() - keyboard_x;
  keyboard_interface_->setBounds(keyboard_x, top + height - keyboard_height - padding, keyboard_width, keyboard_height);
  library_progress_->setBounds(keyboard_interface_->getBounds());

  about_section_->setBounds(bounds);
  update_check_section_->setBounds(bounds);
  save_section_->setBounds(bounds);
  delete_section_->setBounds(bounds);
  download_section_->setBounds(bounds);

  Rectangle<int> browse_bounds(main_bounds.getX(), main_bounds.getY(),
                               column_bounds_.getRight() - main_bounds.getX(), main_bounds.getHeight());
  preset_browser_->setBounds(browse_bounds);
  bank_exporter_->setBounds(browse_bounds);
  SynthSection::resized();

  modulation_manager_->setBounds(bounds);
  
  for (int i = 0; i < vital::kNumOscillators; ++i) {
    if (wavetable_edits_[i])
      wavetable_edits_[i]->setBounds(left, 0, width, height);
  }

  if (synthesis_interface_) {
    // Corpuscular: the oscillators live on their own tab, the advanced rows keep the classic stacking
    int oscillator_height = 2 * knob_section_height - (int)findValue(Skin::kWidgetMargin);
    for (int i = 0; i < vital::kNumOscillators; ++i) {
      Rectangle<int> advanced(0, i * (oscillator_height + padding), main_bounds.getWidth(), oscillator_height);
      master_controls_interface_->setOscillatorBounds(i, advanced);
    }
  }
  master_controls_interface_->setBounds(main_bounds);

  if (full_screen_section_) {
    Rectangle<float> relative = synthesis_interface_->getOscillatorSection(0)->getWavetableRelativeBounds();
    int total_width = getWidth() / relative.getWidth();
    full_screen_section_->setBounds(-total_width * relative.getX(), 0, total_width, getHeight());
  }

  if (getWidth() && getHeight())
    redoBackground();
}

void FullInterface::setOscilloscopeMemory(const vital::poly_float* memory) {
  if (header_)
    header_->setOscilloscopeMemory(memory);
  if (master_controls_interface_)
    master_controls_interface_->setOscilloscopeMemory(memory);
}

void FullInterface::setAudioMemory(const vital::StereoMemory* memory) {
  if (header_)
    header_->setAudioMemory(memory);
  if (master_controls_interface_)
    master_controls_interface_->setAudioMemory(memory);
}

void FullInterface::createModulationSliders(const vital::output_map& mono_modulations,
                                            const vital::output_map& poly_modulations) {
  std::map<std::string, SynthSlider*> all_sliders = getAllSliders();
  std::map<std::string, SynthSlider*> modulatable_sliders;

  for (auto& destination : mono_modulations) {
    if (all_sliders.count(destination.first))
      modulatable_sliders[destination.first] = all_sliders[destination.first];
  }

  modulation_manager_ = std::make_unique<ModulationManager>(getAllModulationButtons(),
                                                            modulatable_sliders, mono_modulations, poly_modulations);
  modulation_manager_->setOpaque(false);
  addSubSection(modulation_manager_.get());
}

void FullInterface::animate(bool animate) {
  if (animate_ != animate)
    open_gl_context_.setContinuousRepainting(animate);

  animate_ = animate;
  SynthSection::animate(animate);
}

void FullInterface::reset() {
  ScopedLock lock(open_gl_critical_section_);

  if (modulation_interface_)
    modulation_interface_->reset();

  setting_all_values_ = true;
  SynthSection::reset();
  modulationChanged();
  if (effects_interface_ && effects_interface_->isVisible())
    effects_interface_->redoBackgroundImage();

  setWavetableNames();
  setting_all_values_ = false;
  repaintSynthesisSection();
}

void FullInterface::setAllValues(vital::control_map& controls) {
  ScopedLock lock(open_gl_critical_section_);
  setting_all_values_ = true;
  SynthSection::setAllValues(controls);
  setting_all_values_ = false;
}

void FullInterface::setWavetableNames() {
  for (int i = 0; i < vital::kNumOscillators; ++i) {
    if (wavetable_edits_[i])
      synthesis_interface_->setWavetableName(i, wavetable_edits_[i]->getName());
  }
}

void FullInterface::startDownload() {
  // Corpuscular: no downloads
}

void FullInterface::newOpenGLContextCreated() {
  double version_supported = OpenGLShaderProgram::getLanguageVersion();
  unsupported_ = version_supported < kMinOpenGlVersion;
  if (unsupported_) {
    NativeMessageBox::showMessageBoxAsync(AlertWindow::WarningIcon, "Unsupported OpenGl Version",
                                          String("Corpuscular requires OpenGL version: ") + String(kMinOpenGlVersion) +
                                          String("\nSupported version: ") + String(version_supported));
    return;
  }

  shaders_ = std::make_unique<Shaders>(open_gl_context_);
  open_gl_.shaders = shaders_.get();
  open_gl_.display_scale = display_scale_;
  last_render_scale_ = display_scale_;

  background_.init(open_gl_);
  initOpenGlComponents(open_gl_);
}

void FullInterface::renderOpenGL() {
  if (unsupported_)
    return;

  float render_scale = open_gl_.context.getRenderingScale();
  if (render_scale != last_render_scale_) {
    last_render_scale_ = render_scale;
    MessageManager::callAsync([=] { checkShouldReposition(true); });
  }

  ScopedLock lock(open_gl_critical_section_);
  open_gl_.display_scale = display_scale_;
  background_.render(open_gl_);
  modulation_manager_->renderMeters(open_gl_, animate_);
  renderOpenGlComponents(open_gl_, animate_);
}

void FullInterface::openGLContextClosing() {
  if (unsupported_)
    return;

  background_.destroy(open_gl_);
  destroyOpenGlComponents(open_gl_);
  open_gl_.shaders = nullptr;
  shaders_ = nullptr;
}

void FullInterface::showAboutSection() {
  ScopedLock lock(open_gl_critical_section_);
  about_section_->setVisible(true);
}

void FullInterface::deleteRequested(File preset) {
  delete_section_->setFileToDelete(preset);
  delete_section_->setVisible(true);
}

void FullInterface::layoutTab() {
  if (modulation_interface_ == nullptr || column_bounds_.isEmpty())
    return;

  int padding = getPadding();
  bool wide = current_tab_ == kAmpTab || current_tab_ == kModTab;
  ModulationInterface::DisplayMode mode = current_tab_ == kAmpTab ? ModulationInterface::kEnvelopes
                                                                   : ModulationInterface::kAll;
  if (modulation_interface_->getDisplayMode() != mode)
    modulation_interface_->setDisplayMode(mode);

  Rectangle<int> modulation_bounds = column_bounds_;
  Rectangle<int> voice_row = voice_row_;
  if (wide) {
    int x = main_bounds_.getX();
    modulation_bounds = column_bounds_.withLeft(x);
    voice_row = voice_row_.withLeft(x);
  }
  modulation_interface_->setBounds(modulation_bounds);

  int portamento_width = 4 * (int)findValue(Skin::kModulationButtonWidth);
  portamento_section_->setBounds(voice_row.getRight() - portamento_width, voice_row.getY(),
                                 portamento_width, voice_row.getHeight());
  voice_section_->setBounds(voice_row.getX(), voice_row.getY(),
                            voice_row.getWidth() - portamento_width - padding, voice_row.getHeight());
}

void FullInterface::showCurrentTab(bool show) {
  int tab = show ? current_tab_ : -1;

  if (synthesis_interface_) {
    if (tab == kOscTab && synthesis_interface_->getDisplayMode() != SynthesisInterface::kSources)
      synthesis_interface_->setDisplayMode(SynthesisInterface::kSources);
    else if (tab == kFilterTab && synthesis_interface_->getDisplayMode() != SynthesisInterface::kFilters)
      synthesis_interface_->setDisplayMode(SynthesisInterface::kFilters);
    synthesis_interface_->setVisible(tab == kOscTab || tab == kFilterTab);
  }

  routing_section_->setVisible(tab == kRouteTab);
  effects_interface_->setVisible(tab == kFxTab);
  modulation_matrix_->setVisible(tab == kMatrixTab);
  master_controls_interface_->setVisible(tab == kSetupTab);
  generator_section_->setVisible(tab == kGenTab);
  arp_section_->setVisible(tab == kArpTab);

  // The ARP tab is the sequencer alone: no envelopes, LFOs, voices or glide next to it
  if (tab >= 0) {
    bool modulation_shown = tab != kArpTab;
    modulation_interface_->setVisible(modulation_shown);
    voice_section_->setVisible(modulation_shown);
    portamento_section_->setVisible(modulation_shown);
  }
  layoutTab();
}

void FullInterface::tabSelected(int index) {
  ScopedLock lock(open_gl_critical_section_);
  bool make_visible = !preset_browser_->isVisible() && !bank_exporter_->isVisible();
  if (index >= 0 && index < kNumTabs)
    current_tab_ = index;

  showCurrentTab(index >= 0 && make_visible);
  modulation_manager_->setModulationAmounts();
  modulation_manager_->resized();
  modulation_manager_->setVisibleMeterBounds();
  modulation_manager_->hideUnusedHoverModulations();
  redoBackground();
}

void FullInterface::clearTemporaryTab(int current_tab) {
  ScopedLock lock(open_gl_critical_section_);
  preset_browser_->setVisible(false);
  bank_exporter_->setVisible(false);
  modulation_interface_->setVisible(true);
  portamento_section_->setVisible(true);
  voice_section_->setVisible(true);
  tabSelected(current_tab);
}

void FullInterface::setPresetBrowserVisibility(bool visible, int current_tab) {
  ScopedLock lock(open_gl_critical_section_);
  preset_browser_->setVisible(visible);
  modulation_interface_->setVisible(!visible);
  portamento_section_->setVisible(!visible);
  voice_section_->setVisible(!visible);
  synthesis_interface_->setVisible(!visible);

  if (visible) {
    tabSelected(-1);
    bank_exporter_->setVisible(false);
    preset_browser_->repaintBackground();
    preset_browser_->grabKeyboardFocus();
    header_->setTemporaryTab("PRESET BROWSER");
  }
  else {
    tabSelected(current_tab);
    header_->setTemporaryTab("");
  }
}

void FullInterface::setBankExporterVisibility(bool visible, int current_tab) {
  ScopedLock lock(open_gl_critical_section_);
  bank_exporter_->setVisible(visible);
  modulation_interface_->setVisible(!visible);
  portamento_section_->setVisible(!visible);
  voice_section_->setVisible(!visible);
  synthesis_interface_->setVisible(!visible);

  if (visible) {
    tabSelected(-1);
    preset_browser_->setVisible(false);
    bank_exporter_->repaintBackground();
    header_->setTemporaryTab("EXPORT BANK");
  }
  else {
    tabSelected(current_tab);
    header_->setTemporaryTab("");
  }
}

void FullInterface::bankImported() {
  preset_browser_->loadPresets();
}

void FullInterface::effectsMoved() {
  modulation_manager_->setVisibleMeterBounds();
}

void FullInterface::modulationsScrolled() {
  modulation_manager_->setVisibleMeterBounds();
}

void FullInterface::setFocus() {
  if (authentication_ && authentication_->isShowing())
    authentication_->setFocus();
  else if (synthesis_interface_ && synthesis_interface_->isShowing())
    synthesis_interface_->setFocus();
}

void FullInterface::notifyChange() {
  if (header_)
    header_->notifyChange();
}

void FullInterface::notifyFresh() {
  if (header_)
    header_->notifyFresh();
}

void FullInterface::externalPresetLoaded(const File& preset) {
  if (preset_browser_)
    preset_browser_->externalPresetLoaded(preset);
}

void FullInterface::showFullScreenSection(SynthSection* full_screen) {
  ScopedLock lock(open_gl_critical_section_);
  full_screen_section_ = full_screen;

  if (full_screen_section_) {
    addSubSection(full_screen_section_);
    full_screen_section_->setBounds(getLocalBounds());
  }

  for (int i = 0; i < vital::kNumOscillators; ++i)
    wavetable_edits_[i]->setVisible(false);

  bool show_rest = full_screen == nullptr;
  header_->setVisible(show_rest);
  showCurrentTab(show_rest);
  modulation_interface_->setVisible(show_rest && current_tab_ != kArpTab);
  library_progress_->setVisible(show_rest && corpuscular::libraryBuilding());
  keyboard_interface_->setVisible(show_rest && !corpuscular::libraryBuilding());
  extra_mod_section_->setVisible(show_rest);
  modulation_manager_->setVisible(show_rest);
  voice_section_->setVisible(show_rest && current_tab_ != kArpTab);
  bend_section_->setVisible(show_rest);
  portamento_section_->setVisible(show_rest && current_tab_ != kArpTab);
  redoBackground();
}

void FullInterface::showWavetableEditSection(int index) {
  if (!wavetableEditorsInitialized())
    return;

  ScopedLock lock(open_gl_critical_section_);
  for (int i = 0; i < vital::kNumOscillators; ++i)
    wavetable_edits_[i]->setVisible(i == index);

  bool show_rest = index < 0;
  header_->setVisible(show_rest);
  showCurrentTab(show_rest);
  modulation_interface_->setVisible(show_rest && current_tab_ != kArpTab);
  library_progress_->setVisible(show_rest && corpuscular::libraryBuilding());
  keyboard_interface_->setVisible(show_rest && !corpuscular::libraryBuilding());
  extra_mod_section_->setVisible(show_rest);
  modulation_manager_->setVisible(show_rest);
  voice_section_->setVisible(show_rest && current_tab_ != kArpTab);
  bend_section_->setVisible(show_rest);
  portamento_section_->setVisible(show_rest && current_tab_ != kArpTab);
  redoBackground();
}

std::string FullInterface::getLastBrowsedWavetable(int index) {
  return wavetable_edits_[index]->getLastBrowsedWavetable();
}

std::string FullInterface::getWavetableName(int index) {
  return wavetable_edits_[index]->getName();
}

std::string FullInterface::getSignedInName() {
  if (authentication_ == nullptr || !auth_.loggedIn())
    return "";
  
  return authentication_->getSignedInName();
}

void FullInterface::signOut() {
  if (authentication_)
    authentication_->signOut();
}

void FullInterface::signIn() {
  if (authentication_) {
    authentication_->create();
    authentication_->setVisible(true);
  }
}

void FullInterface::hideWavetableEditSection() {
  showWavetableEditSection(-1);
}

void FullInterface::loadWavetableFile(int index, const File& wavetable) {
  if (wavetable_edits_[index])
    wavetable_edits_[index]->loadFile(wavetable);
}

void FullInterface::loadWavetable(int index, json& wavetable_data) {
  if (wavetable_edits_[index])
    wavetable_edits_[index]->loadWavetable(wavetable_data);
}

void FullInterface::loadDefaultWavetable(int index) {
  if (wavetable_edits_[index])
    wavetable_edits_[index]->loadDefaultWavetable();
}

void FullInterface::resynthesizeToWavetable(int index) {
  if (wavetable_edits_[index])
    wavetable_edits_[index]->resynthesizeToWavetable();
}

void FullInterface::saveWavetable(int index) {
  save_section_->setIsPreset(false);
  save_section_->setSaveBounds();
  save_section_->setFileExtension(vital::kWavetableExtension);
  save_section_->setFileType("Wavetable");
  File destination = LoadSave::getUserWavetableDirectory();
  if (!destination.exists())
    destination.createDirectory();
  save_section_->setDirectory(destination);
  save_section_->setFileData(getWavetableJson(index));
  save_section_->setVisible(true);
}

void FullInterface::saveArp(const json& data, const File& folder) {
  save_section_->setIsPreset(false);
  save_section_->setFileExtension(ArpSequencer::kExtension);
  save_section_->setFileType("Arp");
  save_section_->setDirectory(folder);
  save_section_->setFileData(data);
  save_section_->setVisible(true);
}

void FullInterface::saveLfo(const json& data) {
  save_section_->setIsPreset(false);
  save_section_->setFileExtension(vital::kLfoExtension);
  save_section_->setFileType("LFO");
  save_section_->setDirectory(LoadSave::getUserLfoDirectory());
  save_section_->setFileData(data);
  save_section_->setVisible(true);
}

json FullInterface::getWavetableJson(int index) {
  if (wavetable_edits_[index])
    return wavetable_edits_[index]->getWavetableJson();
  return json();
}

bool FullInterface::loadAudioAsWavetable(int index, const String& name, InputStream* audio_stream,
                                         WavetableCreator::AudioFileLoadStyle style) {
  if (wavetable_edits_[index])
    return wavetable_edits_[index]->loadAudioAsWavetable(name, audio_stream, style);

  delete audio_stream;
  return true;
}

void FullInterface::popupBrowser(SynthSection* owner, Rectangle<int> bounds, std::vector<File> directories,
                                 String extensions, std::string passthrough_name,
                                 std::string additional_folders_name) {
  popup_browser_->setIgnoreBounds(getLocalArea(owner, owner->getLocalBounds()));

  // Corpuscular: sections near the window edge (the sample source on the OSC tab) keep the browser inside
  Rectangle<int> browser_bounds = getLocalArea(owner, bounds);
  int margin = getPadding();
  if (browser_bounds.getBottom() > getHeight() - margin)
    browser_bounds.setY(std::max(margin, getHeight() - margin - browser_bounds.getHeight()));
  if (browser_bounds.getRight() > getWidth() - margin)
    browser_bounds.setX(std::max(margin, getWidth() - margin - browser_bounds.getWidth()));
  popup_browser_->setBrowserBounds(browser_bounds);
  popup_browser_->setVisible(true);
  popup_browser_->grabKeyboardFocus();
  popup_browser_->setOwner(owner);
  popup_browser_->loadPresets(directories, extensions, passthrough_name, additional_folders_name);
}

void FullInterface::popupBrowserUpdate(SynthSection* owner) {
  if (popup_browser_)
    popup_browser_->setOwner(owner);
}

void FullInterface::popupSelector(Component* source, Point<int> position, const PopupItems& options,
                                  std::function<void(int)> callback, std::function<void()> cancel) {
  popup_selector_->setCallback(callback);
  popup_selector_->setCancelCallback(cancel);
  popup_selector_->showSelections(options);
  Rectangle<int> bounds(0, 0, std::ceil(display_scale_ * getWidth()), std::ceil(display_scale_ * getHeight()));
  popup_selector_->setPosition(getLocalPoint(source, position), bounds);
  popup_selector_->setVisible(true);
}

void FullInterface::dualPopupSelector(Component* source, Point<int> position, int width,
                                      const PopupItems& options, std::function<void(int)> callback) {
  dual_popup_selector_->setCallback(callback);
  dual_popup_selector_->showSelections(options);
  Rectangle<int> bounds(0, 0, std::ceil(display_scale_ * getWidth()), std::ceil(display_scale_ * getHeight()));
  dual_popup_selector_->setPosition(getLocalPoint(source, position), width, bounds);
  dual_popup_selector_->setVisible(true);
}

void FullInterface::popupDisplay(Component* source, const std::string& text,
                                 BubbleComponent::BubblePlacement placement, bool primary) {
  PopupDisplay* display = primary ? popup_display_1_.get() : popup_display_2_.get();
  display->setContent(text, getLocalArea(source, source->getLocalBounds()), placement);
  display->setVisible(true);
}

void FullInterface::hideDisplay(bool primary) {
  PopupDisplay* display = primary ? popup_display_1_.get() : popup_display_2_.get();
  if (display)
    display->setVisible(false);
}

void FullInterface::modulationChanged() {
  if (modulation_matrix_)
    modulation_matrix_->updateModulations();
  if (modulation_interface_)
    modulation_interface_->checkNumShown();
  if (modulation_manager_)
    modulation_manager_->reset();
}

void FullInterface::modulationValueChanged(int index) {
  if (modulation_matrix_)
    modulation_matrix_->updateModulationValue(index);
  if (modulation_manager_)
    modulation_manager_->setModulationAmounts();
}

void FullInterface::toggleOscillatorZoom(int index) {
  if (full_screen_section_)
    showFullScreenSection(nullptr);
  else
    showFullScreenSection(synthesis_interface_->getOscillatorSection(index));
}

void FullInterface::toggleFilter1Zoom() {
  if (full_screen_section_)
    showFullScreenSection(nullptr);
  else
    showFullScreenSection(synthesis_interface_->getFilterSection1());
}

void FullInterface::toggleFilter2Zoom() {
  if (full_screen_section_)
    showFullScreenSection(nullptr);
  else
    showFullScreenSection(synthesis_interface_->getFilterSection2());
}
