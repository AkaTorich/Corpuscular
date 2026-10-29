/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include "MusicData.h"

// Точный порт алгоритма генерации ARPGeneratorVST: прогрессия аккордов ->
// уникальные такты арпеджио (с вариациями мотива и контуром) -> повторы -> склейка.
namespace mg
{
    // Возвращает клип (сетка 16-х, как в SaveMelodyToMidi) и текст с аккордами.
    Clip generateArp(const juce::String& tonality, const juce::String& category,
                     const juce::String& scaleName, const juce::Array<int>& steps,
                     int arpNotesCount, int measuresTotal, int repeats,
                     juce::Random& rng, juce::StringArray& chordInfoOut);

    // Жанры арпеджиатора (для выпадающего списка). Все — непрерывные 16-е.
    const juce::StringArray& arpGenreNames();

    // Стилевой арпеджиатор: непрерывный ролл 16-ми по нотам аккорда согласно жанру.
    // octaves — диапазон в октавах; measuresTotal/repeats как у generateArp.
    Clip generateArpStyled(const juce::String& tonality, const juce::String& scaleName,
                           const juce::Array<int>& steps, const juce::String& genre,
                           int octaves, int measuresTotal, int repeats,
                           juce::Random& rng, juce::StringArray& chordInfoOut);

    // Пситранс-арп: короткий мелодический РИФ (ячейка по ладу — шаги/соседние ступени с
    // возвратами и редкими скачками), повторяется непрерывными 16-ми весь паттерн.
    // cellLen — длина ячейки (нот); octaves — регистр; measuresTotal/repeats;
    // endingStyle — концовка 4-го такта фразы (mg::ArpEnding, список arpEndingNames()).
    Clip generateArpRiff(const juce::String& tonality, const juce::Array<int>& steps,
                         int cellLen, int octaves, int measuresTotal, int repeats,
                         juce::Random& rng, juce::StringArray& infoOut, int endingStyle = 0);
}
