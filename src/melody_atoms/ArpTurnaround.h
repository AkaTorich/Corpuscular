/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <juce_core/juce_core.h>
#include <vector>

// Концовка 4-го такта фразы для пситранс-арпа (мелодик-пситранс: Indra, Astrix, Faders).
// В 4-м, 8-м, 12-м… такте меняется конец такта; каждая концовка отличается от прежних.
// Как именно меняется — выбирается стилем (список «4th bar ending» в окне):
//   QuestionAnswer — фразы чередуют «вопрос» (уход на напряжение) и «ответ» (подход к началу риффа);
//   Hierarchy      — чем крупнее граница (4 / 8 / 16 / 32 такта), тем сильнее перемена;
//   Gap            — пауза или «заикание» перед сильной долей следующей фразы;
//   FlatTwo        — концовка на ♭2 (или вводном тоне) с разрешением в начало риффа;
//   Sequence       — концовка из самого риффа: сдвиг на ступень, обращение, ход назад;
//   Growth         — от скромных концовок к смелым: больше нот и выше регистр к концу клипа.
namespace mg
{
    enum class ArpEnding { QuestionAnswer = 0, Hierarchy, Gap, FlatTwo, Sequence, Growth };

    const juce::StringArray& arpEndingNames();

    class ArpTurnaround
    {
    public:
        // riff — мотив, тайлящийся по 16 шестнадцатым; scalePcs — классы высоты лада;
        // lo..hi — регистр арпа; totalBars — длина клипа (для Growth)
        ArpTurnaround(ArpEnding style, const std::vector<int>& riff, const std::vector<int>& scalePcs,
                      int tonicPc, int lo, int hi, int totalBars);

        // Такт из 16 нот с концовкой (0 — пауза); bar — номер такта с нуля (3, 7, 11…)
        std::vector<int> makeBar(int bar, juce::Random& rng);

    private:
        std::vector<int> questionAnswer(int count, bool answer, juce::Random& rng) const;
        std::vector<int> hierarchy(int bar, juce::Random& rng) const;
        std::vector<int> gap(juce::Random& rng) const;
        std::vector<int> flatTwo(int count, juce::Random& rng) const;
        std::vector<int> sequence(int count, juce::Random& rng) const;
        std::vector<int> growth(int phrase, juce::Random& rng) const;

        int step(int note, int steps, int top = -1) const;       // сдвиг по ступеням лада; выше top (по умолч. hi+12) — октавой вниз
        void fitFigure(std::vector<int>& end) const;             // фигуру целиком — в регистр (lo-5 … hi+7)
        int ladderIndex(int note) const;
        int nearestWithInterval(int interval, int near) const;   // ступень лада около тоника+interval
        int nearestPc(int pc, int near, int top) const;          // нота класса pc рядом с near (≤ top)
        bool inScale(int pc) const;
        int riffAt(int pos) const { return riff[(size_t) (pos % (int) riff.size())]; }
        int randomCount(juce::Random& rng) const;
        void makeDifferent(std::vector<int>& end) const;         // каждая нота ≠ риффу на своём месте

        ArpEnding style;
        std::vector<int> riff, scalePcs, ladder;
        int tonicPc, lo, hi, totalBars;
        std::vector<std::vector<int>> usedTails;                 // последние 4 шестнадцатые прежних концовок
    };
}
