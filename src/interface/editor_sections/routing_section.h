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

class SynthesisInterface;

// ROUTE tab: where every sound source goes (filter 1, filter 2, both, effects, direct out)
// and whether the filters run in series. A remote for the controls that live in the oscillator,
// sample and filter sections, so there is still exactly one owner of every parameter.
class RoutingSection : public SynthSection {
  public:
    static constexpr int kNumSources = 4;
    static constexpr int kNumColumns = 5;

    RoutingSection(SynthesisInterface* synthesis);

    void paintBackground(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& e) override;
    void visibilityChanged() override;

  private:
    Rectangle<float> contentArea() const;
    Rectangle<float> gridArea() const;
    Rectangle<float> cellBounds(int row, int column) const;
    Rectangle<float> serialBounds(int filter) const;
    int getDestination(int row) const;
    void setDestination(int row, int destination);
    ToggleButton* serialButton(int filter) const;
    void paintDiagram(Graphics& g, Rectangle<float> area);
    void paintGrid(Graphics& g);

    SynthesisInterface* synthesis_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingSection)
};
