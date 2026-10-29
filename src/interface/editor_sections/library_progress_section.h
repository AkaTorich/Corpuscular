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

// The progress bar of the first-launch library build; sits where the keyboard is until the build is done
class LibraryProgressSection : public SynthSection, public Timer {
  public:
    LibraryProgressSection();

    // Runs once the build is over (the section has hidden itself by then)
    void setFinishedCallback(std::function<void()> callback) { finished_ = std::move(callback); }
    void start();

    void paintBackground(Graphics& g) override;
    void timerCallback() override;

  private:
    std::function<void()> finished_;
    float shown_progress_;
    String shown_status_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryProgressSection)
};
