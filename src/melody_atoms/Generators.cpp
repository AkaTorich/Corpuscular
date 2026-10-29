/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Generators.h"

using namespace mg;

juce::StringArray mg::generateProgression(const juce::String& tonality, const Scale& scale,
                                          int maxUnique, int length, juce::Random& rng)
{
    juce::StringArray progression;
    auto allChords = MusicData::chordsForScale(tonality, scale);
    if (allChords.isEmpty())
        return progression;

    // Не больше, чем доступно аккордов
    maxUnique = juce::jmin(maxUnique, allChords.size());
    if (maxUnique < 1) maxUnique = 1;
    if (length < maxUnique) length = maxUnique;

    // Выбираем maxUnique уникальных аккордов
    juce::StringArray unique;
    int guard = 0;
    while (unique.size() < maxUnique && guard++ < 10000)
    {
        auto c = allChords[rng.nextInt(allChords.size())];
        if (!unique.contains(c))
            unique.add(c);
    }

    // Перемешиваем (Fisher-Yates) и кладём в начало все уникальные
    for (int i = unique.size(); --i > 0;)
    {
        int j = rng.nextInt(i + 1);
        auto tmp = unique[i];
        unique.set(i, unique[j]);
        unique.set(j, tmp);
    }
    progression.addArray(unique);

    // Дополняем до нужной длины, избегая трёх одинаковых подряд
    while (progression.size() < length)
    {
        auto next = unique[rng.nextInt(unique.size())];
        int n = progression.size();
        if (n >= 2 && progression[n - 1] == next && progression[n - 2] == next)
            continue;
        progression.add(next);
    }

    return progression;
}

Clip mg::renderClip(Mode mode, const juce::StringArray& chordNames)
{
    Clip clip;
    const int measure = 16; // шестнадцатых в такте

    for (int i = 0; i < chordNames.size(); ++i)
    {
        auto notes = MusicData::midiNotesForChord(chordNames[i]);
        if (notes.isEmpty())
            continue;

        const int base = i * measure;

        if (mode == Mode::Pad)
        {
            // Аккорд целиком на весь такт
            for (int n : notes)
                clip.add({ n, base, measure });
        }
        else if (mode == Mode::Arp)
        {
            // Арпеджио вверх шестнадцатыми по нотам аккорда
            juce::Array<int> sorted(notes);
            std::sort(sorted.begin(), sorted.end());
            for (int s = 0; s < measure; ++s)
                clip.add({ sorted[s % sorted.size()], base + s, 1 });
        }
        else // Bass
        {
            // Корень аккорда на октаву ниже, пульс половинками
            int root = notes[0];
            for (int n : notes) root = juce::jmin(root, n);
            root = juce::jmax(24, root - 12);
            clip.add({ root, base, measure / 2 });
            clip.add({ root, base + measure / 2, measure / 2 });
        }
    }

    return clip;
}
