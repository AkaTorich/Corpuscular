/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "routing_section.h"

#include "fonts.h"
#include "oscillator_section.h"
#include "sample_section.h"
#include "skin.h"
#include "synth_constants.h"
#include "synth_strings.h"
#include "synthesis_interface.h"

namespace {
  const char* kRouteSourceNames[RoutingSection::kNumSources] = { "OSC 1", "OSC 2", "OSC 3", "SAMPLE" };
  const char* kRouteColumnNames[RoutingSection::kNumColumns] = { "FILTER 1", "FILTER 2", "BOTH", "EFFECTS", "DIRECT" };
  const char* kRouteSerialNames[2] = { "FILTER 1 INTO FILTER 2", "FILTER 2 INTO FILTER 1" };
  // filter_2_filter_input: filter 2 takes filter 1; filter_1_filter_input: filter 1 takes filter 2
  const char* kRouteSerialParameters[2] = { "filter_2_filter_input", "filter_1_filter_input" };

  constexpr float kRouteDiagramPart = 0.42f;
}

RoutingSection::RoutingSection(SynthesisInterface* synthesis) : SynthSection("ROUTE"), synthesis_(synthesis) {
  setInterceptsMouseClicks(true, false);
}

void RoutingSection::resized() {
  SynthSection::resized();
}

void RoutingSection::visibilityChanged() {
  SynthSection::visibilityChanged();
  if (isShowing())
    repaintBackground();
}

int RoutingSection::getDestination(int row) const {
  if (synthesis_ == nullptr)
    return 0;
  if (row < vital::kNumOscillators)
    return synthesis_->getOscillatorSection(row)->getDestinationSelected();
  return synthesis_->getSampleSection()->getDestinationSelected();
}

void RoutingSection::setDestination(int row, int destination) {
  if (synthesis_ == nullptr)
    return;
  if (row < vital::kNumOscillators)
    synthesis_->getOscillatorSection(row)->setDestinationSelected(destination);
  else
    synthesis_->getSampleSection()->setDestinationSelected(destination);
}

ToggleButton* RoutingSection::serialButton(int filter) const {
  if (synthesis_ == nullptr)
    return nullptr;
  std::map<std::string, ToggleButton*> buttons = synthesis_->getAllButtons();
  auto found = buttons.find(kRouteSerialParameters[filter]);
  return found == buttons.end() ? nullptr : found->second;
}

Rectangle<float> RoutingSection::contentArea() const {
  float padding = 12.0f * size_ratio_;
  float left = findValue(Skin::kTitleWidth) + padding;
  return Rectangle<float>(left, padding, getWidth() - left - padding, getHeight() - 2.0f * padding);
}

Rectangle<float> RoutingSection::gridArea() const {
  Rectangle<float> content = contentArea();
  float top = getHeight() * kRouteDiagramPart;
  return content.withTop(top);
}

Rectangle<float> RoutingSection::cellBounds(int row, int column) const {
  Rectangle<float> grid = gridArea();
  float label_width = grid.getWidth() * 0.18f;
  float header_height = grid.getHeight() / 9.0f;
  float rows_height = grid.getHeight() * 6.0f / 9.0f - header_height;
  float row_height = rows_height / kNumSources;
  float column_width = (grid.getWidth() - label_width) / kNumColumns;
  float gap = 3.0f * size_ratio_;
  return Rectangle<float>(grid.getX() + label_width + column * column_width,
                          grid.getY() + header_height + row * row_height,
                          column_width, row_height).reduced(gap);
}

Rectangle<float> RoutingSection::serialBounds(int filter) const {
  Rectangle<float> grid = gridArea();
  float height = grid.getHeight() / 9.0f;
  float y = grid.getY() + grid.getHeight() * 7.0f / 9.0f;
  float width = (grid.getWidth() - 12.0f * size_ratio_) / 2.0f;
  float x = grid.getX() + filter * (width + 12.0f * size_ratio_);
  return Rectangle<float>(x, y, width, height * 1.3f);
}

void RoutingSection::mouseDown(const MouseEvent& e) {
  Point<float> point = e.position;
  for (int row = 0; row < kNumSources; ++row) {
    for (int column = 0; column < kNumColumns; ++column) {
      if (cellBounds(row, column).contains(point)) {
        setDestination(row, column);
        repaintBackground();
        return;
      }
    }
  }

  for (int filter = 0; filter < 2; ++filter) {
    if (serialBounds(filter).contains(point)) {
      ToggleButton* button = serialButton(filter);
      if (button)
        button->setToggleState(!button->getToggleState(), sendNotificationSync);
      repaintBackground();
      return;
    }
  }
}

void RoutingSection::paintBackground(Graphics& g) {
  paintContainer(g);
  paintHeadingText(g);

  float padding = 12.0f * size_ratio_;
  Rectangle<float> content = contentArea();
  Rectangle<float> diagram = content.withBottom(getHeight() * kRouteDiagramPart - padding);
  paintDiagram(g, diagram);
  paintGrid(g);
}

void RoutingSection::paintDiagram(Graphics& g, Rectangle<float> area) {
  Colour box = findColour(Skin::kTextComponentBackground, true);
  Colour text = findColour(Skin::kBodyText, true);
  Colour active = findColour(Skin::kRotaryArc, true);
  Colour quiet = findColour(Skin::kLightenScreen, true);
  float rounding = findValue(Skin::kLabelBackgroundRounding);
  float line = std::max(1.5f, 2.0f * size_ratio_);
  Font font = Fonts::instance()->proportional_regular().withPointHeight(area.getHeight() * 0.07f);
  g.setFont(font);

  float column_width = area.getWidth() / 4.0f;
  float box_width = column_width * 0.62f;
  float source_height = area.getHeight() / (kNumSources + 1.0f);

  auto boxAt = [&](float column, float center_y, float height) {
    float center_x = area.getX() + column_width * (column + 0.5f);
    return Rectangle<float>(center_x - box_width / 2.0f, center_y - height / 2.0f, box_width, height);
  };

  Rectangle<float> sources[kNumSources];
  for (int i = 0; i < kNumSources; ++i)
    sources[i] = boxAt(0, area.getY() + source_height * (i + 0.75f), source_height * 0.7f);

  float filter_height = area.getHeight() * 0.22f;
  Rectangle<float> filter1 = boxAt(1, area.getY() + area.getHeight() * 0.28f, filter_height);
  Rectangle<float> filter2 = boxAt(1, area.getY() + area.getHeight() * 0.72f, filter_height);
  Rectangle<float> effects = boxAt(2, area.getCentreY(), filter_height);
  Rectangle<float> output = boxAt(3, area.getCentreY(), filter_height);

  auto arrow = [&](Rectangle<float> from, Rectangle<float> to, Colour colour) {
    Path path;
    Point<float> start(from.getRight(), from.getCentreY());
    Point<float> end(to.getX(), to.getCentreY());
    float bend = (end.x - start.x) * 0.5f;
    path.startNewSubPath(start);
    path.cubicTo(start.x + bend, start.y, end.x - bend, end.y, end.x, end.y);
    g.setColour(colour);
    g.strokePath(path, PathStrokeType(line));
  };

  bool filter_used[2] = { false, false };
  for (int i = 0; i < kNumSources; ++i) {
    int destination = getDestination(i);
    if (destination == vital::constants::kFilter1 || destination == vital::constants::kDualFilters) {
      arrow(sources[i], filter1, active);
      filter_used[0] = true;
    }
    if (destination == vital::constants::kFilter2 || destination == vital::constants::kDualFilters) {
      arrow(sources[i], filter2, active);
      filter_used[1] = true;
    }
    if (destination == vital::constants::kEffects || destination > vital::constants::kDirectOut)
      arrow(sources[i], effects, active);
    if (destination == vital::constants::kDirectOut)
      arrow(sources[i], output, active.withMultipliedAlpha(0.7f));
  }

  ToggleButton* one_into_two = serialButton(0);
  ToggleButton* two_into_one = serialButton(1);
  g.setColour(active);
  if (one_into_two && one_into_two->getToggleState())
    g.drawArrow(Line<float>(filter1.getCentreX(), filter1.getBottom(), filter2.getCentreX(), filter2.getY()), line, line * 4, line * 4);
  if (two_into_one && two_into_one->getToggleState())
    g.drawArrow(Line<float>(filter2.getCentreX(), filter2.getY(), filter1.getCentreX(), filter1.getBottom()), line, line * 4, line * 4);

  arrow(filter1, effects, filter_used[0] ? active : quiet);
  arrow(filter2, effects, filter_used[1] ? active : quiet);
  arrow(effects, output, active);

  auto drawBox = [&](Rectangle<float> bounds, const String& name, bool lit) {
    g.setColour(box);
    g.fillRoundedRectangle(bounds, rounding);
    g.setColour(lit ? active : quiet);
    g.drawRoundedRectangle(bounds, rounding, line);
    g.setColour(text);
    g.drawText(name, bounds, Justification::centred);
  };

  for (int i = 0; i < kNumSources; ++i)
    drawBox(sources[i], kRouteSourceNames[i], true);
  drawBox(filter1, "FILTER 1", filter_used[0]);
  drawBox(filter2, "FILTER 2", filter_used[1]);
  drawBox(effects, "EFFECTS", true);
  drawBox(output, "OUT", true);
}

void RoutingSection::paintGrid(Graphics& g) {
  Colour cell = findColour(Skin::kTextComponentBackground, true);
  Colour text = findColour(Skin::kBodyText, true);
  Colour active = findColour(Skin::kRotaryArc, true);
  Colour active_text = findColour(Skin::kBodyText, true);
  float rounding = findValue(Skin::kLabelBackgroundRounding);
  Rectangle<float> grid = gridArea();
  float header_height = grid.getHeight() / 9.0f;
  Font font = Fonts::instance()->proportional_regular().withPointHeight(header_height * 0.42f);
  g.setFont(font);

  g.setColour(text);
  for (int column = 0; column < kNumColumns; ++column) {
    Rectangle<float> first = cellBounds(0, column);
    g.drawText(kRouteColumnNames[column], first.getX(), grid.getY(), first.getWidth(), header_height, Justification::centred);
  }

  for (int row = 0; row < kNumSources; ++row) {
    int destination = getDestination(row);
    Rectangle<float> first = cellBounds(row, 0);
    String label = kRouteSourceNames[row];
    if (destination > vital::constants::kDirectOut && destination < vital::constants::kNumSourceDestinations + vital::constants::kNumEffects)
      label += String("  > ") + strings::kDestinationNames[destination];

    g.setColour(text);
    g.drawText(label, grid.getX(), first.getY(), first.getX() - grid.getX(), first.getHeight(), Justification::centredLeft);

    for (int column = 0; column < kNumColumns; ++column) {
      Rectangle<float> bounds = cellBounds(row, column);
      bool on = destination == column;
      g.setColour(on ? active : cell);
      g.fillRoundedRectangle(bounds, rounding);
      if (on) {
        g.setColour(active_text);
        g.drawText("ON", bounds, Justification::centred);
      }
    }
  }

  for (int filter = 0; filter < 2; ++filter) {
    ToggleButton* button = serialButton(filter);
    bool on = button && button->getToggleState();
    Rectangle<float> bounds = serialBounds(filter);
    g.setColour(on ? active : cell);
    g.fillRoundedRectangle(bounds, rounding);
    g.setColour(on ? active_text : text);
    g.drawText(kRouteSerialNames[filter], bounds, Justification::centred);
  }
}
