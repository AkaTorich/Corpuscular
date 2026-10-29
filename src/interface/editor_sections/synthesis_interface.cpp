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

#include "synthesis_interface.h"

#include "filter_section.h"
#include "oscillator_section.h"
#include "producers_module.h"
#include "synth_oscillator.h"
#include "sample_section.h"

SynthesisInterface::SynthesisInterface(Authentication* auth,
                                       const vital::output_map& mono_modulations,
                                       const vital::output_map& poly_modulations) : SynthSection("synthesis") {
  filter_section_2_ = std::make_unique<FilterSection>(2, mono_modulations, poly_modulations);
  addSubSection(filter_section_2_.get());
  filter_section_2_->addListener(this);

  filter_section_1_ = std::make_unique<FilterSection>(1, mono_modulations, poly_modulations);
  addSubSection(filter_section_1_.get());
  filter_section_1_->addListener(this);

  for (int i = 0; i < vital::kNumOscillators; ++i) {
    oscillators_[i] = std::make_unique<OscillatorSection>(auth, i, mono_modulations, poly_modulations);
    addSubSection(oscillators_[i].get());
    oscillators_[i]->addListener(this);
  }

  sample_section_ = std::make_unique<SampleSection>("SMP");
  addSubSection(sample_section_.get());
  sample_section_->addListener(this);

  setOpaque(false);
}

SynthesisInterface::~SynthesisInterface() { }

void SynthesisInterface::paintBackground(Graphics& g) {
  paintChildrenBackgrounds(g);
}

void SynthesisInterface::setDisplayMode(DisplayMode mode) {
  display_mode_ = mode;
  bool sources = mode != kFilters;
  bool filters = mode != kSources;
  for (int i = 0; i < vital::kNumOscillators; ++i)
    oscillators_[i]->setVisible(sources);
  sample_section_->setVisible(sources);
  filter_section_1_->setVisible(filters);
  filter_section_2_->setVisible(filters);
  resized();
}

void SynthesisInterface::resized() {
  int padding = getPadding();
  int active_width = getWidth() - padding;
  int width_left = (active_width - padding) / 2;
  int width_right = active_width - width_left;
  int right_x = width_left + padding;

  int oscillator_margin = oscillators_[0]->findValue(Skin::kWidgetMargin);
  int oscillator_height = 2 * (int)oscillators_[0]->getKnobSectionHeight() - oscillator_margin;

  if (display_mode_ == kSources) {
    // Oscillators and sample take the whole height
    int sample_height = sample_section_->getKnobSectionHeight();
    int large_padding = findValue(Skin::kLargePadding);
    int osc_area = getHeight() - sample_height - large_padding - (vital::kNumOscillators - 1) * padding;
    int osc_height = std::max(oscillator_height, osc_area / vital::kNumOscillators);
    for (int i = 0; i < vital::kNumOscillators; ++i)
      oscillators_[i]->setBounds(0, i * (osc_height + padding), getWidth(), osc_height);
    int sample_y = oscillators_[vital::kNumOscillators - 1]->getBottom() + large_padding;
    sample_section_->setBounds(0, sample_y, getWidth(), std::max(sample_height, getHeight() - sample_y));
    SynthSection::resized();
    return;
  }

  if (display_mode_ == kFilters) {
    // Two big filters, one above the other
    int filter_height = std::max(0, getHeight() - padding) / 2;
    filter_section_1_->setBounds(0, 0, getWidth(), filter_height);
    filter_section_2_->setBounds(0, filter_height + padding, getWidth(), std::max(0, getHeight() - filter_height - padding));
    SynthSection::resized();
    return;
  }

  for (int i = 0; i < vital::kNumOscillators; ++i)
    oscillators_[i]->setBounds(0, i * (oscillator_height + padding), getWidth(), oscillator_height);

  int sample_y = oscillators_[vital::kNumOscillators - 1]->getBottom() + padding;
  int sample_height = sample_section_->getKnobSectionHeight();
  int filter_y = sample_y + sample_height + findValue(Skin::kLargePadding);
  int filter_height = getHeight() - filter_y;

  sample_section_->setBounds(0, sample_y, getWidth(), sample_height);

  filter_section_1_->setBounds(0, filter_y, width_left, filter_height);
  filter_section_2_->setBounds(right_x, filter_y, width_right, filter_height);
  SynthSection::resized();
}

void SynthesisInterface::visibilityChanged() {
  if (isShowing()) {
    for (int i = 0; i < vital::kNumOscillators; ++i)
      oscillators_[i]->loadBrowserState();
  }
}

void SynthesisInterface::distortionTypeChanged(OscillatorSection* section, int distortion_type) {
  bool dependents[vital::kNumOscillators];
  for (int i = 0; i < vital::kNumOscillators; ++i)
    dependents[i] = false;

  int index = section->index();
  int last_index = index;
  while (!dependents[index]) {
    dependents[index] = true;
    last_index = index;
    int type = oscillators_[index]->getDistortion();
    if (vital::SynthOscillator::isFirstModulation(type))
      index = vital::ProducersModule::getFirstModulationIndex(index);
    else if (vital::SynthOscillator::isSecondModulation(type))
      index = vital::ProducersModule::getSecondModulationIndex(index);
    else
      return;
  }

  oscillators_[last_index]->resetOscillatorModulationDistortionType();
}

void SynthesisInterface::oscillatorDestinationChanged(OscillatorSection* section, int destination) {
  bool filter1_on = destination == vital::constants::kFilter1 || destination == vital::constants::kDualFilters;
  bool filter2_on = destination == vital::constants::kFilter2 || destination == vital::constants::kDualFilters;
  for (int i = 0; i < vital::kNumOscillators; ++i) {
    if (oscillators_[i].get() == section) {
      filter_section_1_->setOscillatorInput(i, filter1_on);
      filter_section_2_->setOscillatorInput(i, filter2_on);
    }
  }
}

void SynthesisInterface::filterSerialSelected(FilterSection* section) {
  if (section == filter_section_1_.get())
    filter_section_2_->clearFilterInput();
  else
    filter_section_1_->clearFilterInput();
}

void SynthesisInterface::oscInputToggled(FilterSection* section, int index, bool on) {
  int filter_index = section == filter_section_1_.get() ? 0 : 1;
  oscillators_[index]->toggleFilterInput(filter_index, on);
}

void SynthesisInterface::sampleInputToggled(FilterSection* section, bool on) {
  int filter_index = section == filter_section_1_.get() ? 0 : 1;
  sample_section_->toggleFilterInput(filter_index, on);
}

void SynthesisInterface::sampleDestinationChanged(SampleSection* section, int destination) {
  bool filter1_on = destination == vital::constants::kFilter1 || destination == vital::constants::kDualFilters;
  bool filter2_on = destination == vital::constants::kFilter2 || destination == vital::constants::kDualFilters;
  filter_section_1_->setSampleInput(filter1_on);
  filter_section_2_->setSampleInput(filter2_on);
}

