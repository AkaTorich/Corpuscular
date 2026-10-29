/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "library_progress_section.h"

#include "corpuscular_library.h"
#include "fonts.h"
#include "skin.h"

LibraryProgressSection::LibraryProgressSection() : SynthSection("library_progress"), shown_progress_(-1.0f) {
  setInterceptsMouseClicks(false, false);
}

void LibraryProgressSection::start() {
  setVisible(true);
  startTimerHz(4);
}

void LibraryProgressSection::timerCallback() {
  if (!corpuscular::libraryBuilding()) {
    stopTimer();
    setVisible(false);
    if (finished_)
      finished_();
    return;
  }

  float progress = corpuscular::libraryBuildProgress();
  String status = corpuscular::libraryBuildStatus();
  if (std::abs(progress - shown_progress_) < 0.002f && status == shown_status_)
    return;

  shown_progress_ = progress;
  shown_status_ = status;
  repaintBackground();
}

void LibraryProgressSection::paintBackground(Graphics& g) {
  float rounding = findValue(Skin::kLabelBackgroundRounding);
  Rectangle<float> bounds = getLocalBounds().toFloat();
  g.setColour(findColour(Skin::kBody, true));
  g.fillRoundedRectangle(bounds, rounding);

  float padding = bounds.getHeight() * 0.18f;
  Rectangle<float> inner = bounds.reduced(padding);
  Rectangle<float> text_area = inner.removeFromTop(inner.getHeight() * 0.45f);
  Rectangle<float> bar = inner.withTrimmedTop(inner.getHeight() * 0.15f);

  float progress = jlimit(0.0f, 1.0f, std::max(0.0f, shown_progress_));
  g.setColour(findColour(Skin::kBodyText, true));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(text_area.getHeight() * 0.8f));
  String status = shown_status_.isEmpty() ? String("Starting") : shown_status_;
  g.drawText("Building the Corpuscular library:  " + status, text_area, Justification::centredLeft);
  g.drawText(String(roundToInt(progress * 100.0f)) + "%", text_area, Justification::centredRight);

  g.setColour(findColour(Skin::kTextComponentBackground, true));
  g.fillRoundedRectangle(bar, bar.getHeight() * 0.5f);
  if (progress > 0.0f) {
    g.setColour(findColour(Skin::kRotaryArc, true));
    g.fillRoundedRectangle(bar.withWidth(std::max(bar.getHeight(), bar.getWidth() * progress)), bar.getHeight() * 0.5f);
  }
}
