/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include "MusicData.h"

namespace mg
{
    // Arpsy — PSY ARP по рецепту ARPSY (ArpsyGen.h); новым — в конец, чтобы не сдвигать сохранённые режимы
    enum class Mode { Pad = 0, Arp = 1, Bass = 2, Scale = 3, Arpsy = 4 };

    // Сгенерировать прогрессию имён аккордов (порт GenerateChordProgression)
    juce::StringArray generateProgression(const juce::String& tonality, const Scale& scale,
                                          int maxUnique, int length, juce::Random& rng);

    // Превратить прогрессию аккордов в ноты сетки согласно режиму
    Clip renderClip(Mode mode, const juce::StringArray& chordNames);
}
