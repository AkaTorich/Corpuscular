/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <juce_core/juce_core.h>

// Музыкальные данные и теория аккордов для генератора.
// Гаммы грузятся из встроенного scales.json; аккорды для гаммы выводятся
// из её интервалов (порт логики PADGenerator).
namespace mg
{
    struct Scale
    {
        juce::String name;
        juce::Array<int> steps; // шаги в полутонах
    };

    struct Category
    {
        juce::String name;
        juce::Array<Scale> scales;
    };

    // Одна нота в сетке: время и длина в шестнадцатых (16 = такт 4/4)
    struct NoteEvent
    {
        int midi;
        int startSixteenth;
        int lengthSixteenth;
    };

    using Clip = juce::Array<NoteEvent>;

    class MusicData
    {
    public:
        MusicData();

        const juce::Array<Category>& categories() const noexcept { return cats; }

        // 24 тональности: C..B мажор и Cm..Bm минор
        static const juce::StringArray& tonalities();

        // Хроматический ряд C..B (12 имён)
        static const juce::StringArray& chromatic();

        // Имена нот гаммы от тоники (для режима SCALE): walk по шагам
        static juce::StringArray scaleNoteNames(int tonicPitchClass, const juce::Array<int>& steps);

        // Диатонические аккорды гаммы в данной тональности (имена вида "Am7")
        static juce::StringArray chordsForScale(const juce::String& tonality, const Scale& scale);

        // MIDI-ноты аккорда по имени, уложенные в октаву 60..83
        static juce::Array<int> midiNotesForChord(const juce::String& chordName);

    private:
        juce::Array<Category> cats;
    };
}
