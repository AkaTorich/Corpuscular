/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include "MusicData.h"

// Порт ядра BASSGeneratorVST: прогрессия аккордов -> ритмическая басовая линия
// (басовый паттерн + жанровый ритм-шаблон + выбор нот с предпочтением тонов
// аккорда + стилевые длительности) -> повторы -> склейка.
// Слой жанровой орнаментации (ApplyPsyTranceVars и т.п.) пока не применяется.
namespace mg
{
    // Имена басовых паттернов для выпадающего списка (порядок как в оригинале)
    const juce::StringArray& bassPatternNames();

    Clip generateBass(const juce::String& tonality, const juce::String& scaleName,
                      const juce::Array<int>& steps, const juce::String& bassPatternName,
                      int bassNotesCount, int measuresUnique, int repeats,
                      juce::Random& rng, juce::StringArray& chordInfoOut);
}
