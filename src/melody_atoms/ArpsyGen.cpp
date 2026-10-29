/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ArpsyGen.h"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <vector>

namespace mg
{
    namespace
    {
        using Bar = std::vector<int>;   // 16 шестнадцатых: MIDI-нота, 0 — пауза

        const char* kDeg[12] = { "1", "b2", "2", "b3", "3", "4", "b5", "5", "b6", "6", "b7", "7" };

        // Числа — из анализа ARPSY (report.txt), в процентах
        const double kColorW[12]   = { 0, 8.1, 1.7, 4.2, 3.1, 4.9, 0.2, 6.3, 1.8, 0.1, 4.3, 0.4 };          // ступени среди всех нот
        const double kPosW[16]     = { 23, 31, 34, 35, 31, 30, 39, 35, 28, 36, 30, 43, 38, 35, 53, 41 };     // доля не-тоник по шестнадцатым
        const double kDiffPosW[16] = { 5.7, 5.0, 4.7, 4.6, 3.9, 4.2, 5.3, 4.2, 4.4, 5.2, 5.3, 7.8, 7.2, 8.7, 12.3, 11.5 }; // где меняют ответы
        const double kLandW[12]    = { 32.4, 13.3, 3.0, 6.5, 6.7, 10.2, 0.7, 13.7, 4.8, 0.2, 8.3, 0.3 };     // чем кончается ответ
        const double kChangeCount[10] = { 14, 15, 13, 10, 10, 10, 5, 5, 5, 5 };                             // 1…10 нот в ответе

        struct Structure { const char* name; double weight; };
        const Structure kStructures[] = { { "ABAC", 47.0 }, { "ABCD", 36.3 }, { "ABAB", 6.1 }, { "AABC", 3.9 },
                                          { "ABCB", 2.9 }, { "AAAB", 2.0 } };

        int pcOf(int n) { return ((n % 12) + 12) % 12; }

        int pickWeighted(const std::vector<double>& w, juce::Random& rng)
        {
            double sum = 0;
            for (double v : w) sum += v;
            if (sum <= 0) return -1;
            double r = rng.nextDouble() * sum;
            for (size_t i = 0; i < w.size(); ++i) { r -= w[i]; if (r < 0) return (int) i; }
            return (int) w.size() - 1;
        }

        int tonicFromTonality(const juce::String& tonality)
        {
            static const std::map<juce::String, int> m = {
                { "C", 0 }, { "C#", 1 }, { "Db", 1 }, { "D", 2 }, { "D#", 3 }, { "Eb", 3 }, { "E", 4 }, { "F", 5 },
                { "F#", 6 }, { "Gb", 6 }, { "G", 7 }, { "G#", 8 }, { "Ab", 8 }, { "A", 9 }, { "A#", 10 }, { "Bb", 10 }, { "B", 11 } };
            auto name = tonality.endsWith("m") ? tonality.dropLastCharacters(1) : tonality;
            auto it = m.find(name.trim());
            return it != m.end() ? it->second : 9;
        }

        struct Ctx
        {
            int base = 57, octaves = 2;
            std::vector<int> scaleDeg, palette, ladder;

            int top() const { return base + 12 * octaves + 7; }
            int degreeOf(int note) const { return pcOf(note - base); }
            bool inScale(int d) const { return std::find(scaleDeg.begin(), scaleDeg.end(), pcOf(d)) != scaleDeg.end(); }

            int fit(int note) const   // в диапазон петли октавой
            {
                while (note > top()) note -= 12;
                while (note < base) note += 12;
                return note;
            }

            int ladderStep(int note, int steps) const
            {
                if (ladder.empty()) return note;
                int idx = 0, best = 1 << 30;
                for (int i = 0; i < (int) ladder.size(); ++i)
                    if (std::abs(ladder[(size_t) i] - note) < best) { best = std::abs(ladder[(size_t) i] - note); idx = i; }
                return fit(ladder[(size_t) juce::jlimit(0, (int) ladder.size() - 1, idx + steps)]);
            }

            int nearestWithDegree(int degree, int near) const
            {
                int best = near, bestDist = 1 << 30;
                for (int n = base; n <= top(); ++n)
                    if (degreeOf(n) == pcOf(degree) && std::abs(n - near) < bestDist) { bestDist = std::abs(n - near); best = n; }
                return best;
            }
        };

        // Такт A: тоника, прыгающая по октавам, с вкраплениями цветных ступеней
        Bar makeBarA(const Ctx& c, int colorNotes, juce::Random& rng)
        {
            std::vector<double> posW(kPosW, kPosW + 16);       // первая доля сама редко цветная (23%)
            std::vector<bool> color(16, false);
            for (int k = 0; k < colorNotes && !c.palette.empty(); ++k)
            {
                const int p = pickWeighted(posW, rng);
                if (p < 0) break;
                color[(size_t) p] = true;
                posW[(size_t) p] = 0;
                if (p > 0) posW[(size_t) p - 1] *= 0.5;     // цветные ноты стоят по одной
                if (p < 15) posW[(size_t) p + 1] *= 0.5;
            }

            std::vector<double> colorW;
            for (int d : c.palette) colorW.push_back(kColorW[d] + 0.05);

            const int levels = c.octaves + 1;
            Bar bar(16);
            int oct = rng.nextInt(levels);
            for (int p = 0; p < 16; ++p)
            {
                if (p > 0 && rng.nextDouble() >= 0.2)         // чаще всего — прыжок в другую октаву
                {
                    std::vector<double> w((size_t) levels, 1.0);
                    w[(size_t) oct] = 0;
                    if (p >= 12) for (int l = 1; l < levels; ++l) w[(size_t) l] += 0.5;   // к концу такта — выше
                    const int o = pickWeighted(w, rng);
                    if (o >= 0) oct = o;
                }
                int note = c.base + 12 * oct;
                if (color[(size_t) p]) note = c.fit(note + c.palette[(size_t) pickWeighted(colorW, rng)]);
                bar[(size_t) p] = note;
            }

            // 11% петель ARPSY — такт из двух одинаковых половин
            if (rng.nextDouble() < 0.2)
                for (int p = 8; p < 16; ++p) bar[(size_t) p] = bar[(size_t) p - 8];

            // 22% петель ARPSY — с паузами: 2–4 в такте (не на первой доле, не рядом друг с другом)
            if (rng.nextDouble() < 0.27)
            {
                const int rests = 2 + rng.nextInt(3);
                for (int k = 0, guard = 0; k < rests && guard < 32; ++guard)
                {
                    const int p = 1 + rng.nextInt(15);
                    if (bar[(size_t) p] == 0 || bar[(size_t) p - 1] == 0 || (p < 15 && bar[(size_t) p + 1] == 0)) continue;
                    bar[(size_t) p] = 0;
                    ++k;
                }
            }
            return bar;
        }

        // Ответ: копия A, в которой меняются 1–6 нот, чаще в конце такта
        Bar makeAnswer(const Ctx& c, const Bar& a, const std::vector<Bar>& existing, juce::Random& rng)
        {
            Bar best = a;
            for (int tries = 0; tries < 24; ++tries)
            {
                const int m = 1 + pickWeighted(std::vector<double>(kChangeCount, kChangeCount + 10), rng);
                std::vector<int> pos;
                if (rng.nextDouble() < 0.7)
                    for (int p = 16 - m; p < 16; ++p) pos.push_back(p);
                else
                {
                    std::vector<double> w(kDiffPosW, kDiffPosW + 16);
                    for (int k = 0; k < m; ++k) { const int p = pickWeighted(w, rng); if (p < 0) break; pos.push_back(p); w[(size_t) p] = 0; }
                    std::sort(pos.begin(), pos.end());
                }

                Bar b = a;
                for (size_t i = 0; i < pos.size(); ++i)
                {
                    const int p = pos[i], orig = a[(size_t) p];
                    if (orig == 0) { b[(size_t) p] = c.base + 12 * rng.nextInt(c.octaves + 1); continue; }   // пауза → нота
                    const bool last = i + 1 == pos.size();
                    const double r = rng.nextDouble();
                    int note;
                    if (r < 0.35 || (r >= 0.9 && last))                          // перенос на октаву
                    {
                        note = orig + (rng.nextBool() ? 12 : -12);
                        if (note > c.top() || note < c.base) note = orig - (note - orig);
                        if (note > c.top() || note < c.base) note = c.ladderStep(orig, 1);
                    }
                    else if (r < 0.65)                                            // шаг по ладу
                        note = c.ladderStep(orig, (rng.nextBool() ? 1 : -1) * (1 + rng.nextInt(2)));
                    else if (r < 0.9 && !c.palette.empty())                       // другая цветная ступень
                        note = c.nearestWithDegree(c.palette[(size_t) rng.nextInt((int) c.palette.size())], orig);
                    else
                        note = 0;                                                 // пауза (редко, не в конце)
                    if (note == orig) note = c.ladderStep(orig, 1);
                    if (note == 0 && (p == 0 || b[(size_t) p - 1] == 0)) note = c.ladderStep(orig, 1);       // без двух пауз подряд
                    b[(size_t) p] = note;
                }

                // Конец ответа: чаще тоника, квинта, ♭2 — как в ARPSY
                const int lastPos = pos.empty() ? 15 : pos.back();
                if (b[(size_t) lastPos] > 0)
                {
                    std::vector<double> w(kLandW, kLandW + 12);
                    for (int d = 1; d < 12; ++d) if (!c.inScale(d)) w[(size_t) d] = 0;
                    const int d = pickWeighted(w, rng);
                    int note = c.nearestWithDegree(d < 0 ? 0 : d, b[(size_t) lastPos]);
                    if (note == a[(size_t) lastPos]) note = c.fit(note + (note + 12 <= c.top() ? 12 : -12));
                    if (note != a[(size_t) lastPos]) b[(size_t) lastPos] = note;
                }

                best = b;
                if (b != a && std::find(existing.begin(), existing.end(), b) == existing.end()) return b;
            }
            return best;
        }
    }

    const juce::StringArray& arpsyStructureNames()
    {
        static const juce::StringArray names { "auto", "ABAC", "ABCD", "ABAB", "AABC", "ABCB", "AAAB" };
        return names;
    }

    Clip generateArpsy(const juce::String& tonality, const juce::Array<int>& steps,
                       int colorNotes, int bars, int octaves, int structure,
                       juce::Random& rng, juce::StringArray& infoOut)
    {
        Ctx c;
        const int tonicPc = tonicFromTonality(tonality);
        c.octaves = juce::jlimit(1, 3, octaves);
        c.base = 48 + tonicPc;                        // нижняя тоника — G#2…G3, как в ARPSY
        if (c.base > 55) c.base -= 12;

        c.scaleDeg.push_back(0);
        int acc = 0;
        for (int s : steps) { acc += s; if (pcOf(acc) != 0) c.scaleDeg.push_back(pcOf(acc)); }
        std::sort(c.scaleDeg.begin(), c.scaleDeg.end());
        c.scaleDeg.erase(std::unique(c.scaleDeg.begin(), c.scaleDeg.end()), c.scaleDeg.end());
        for (int n = c.base - 12; n <= c.top() + 12; ++n) if (c.inScale(n - c.base)) c.ladder.push_back(n);

        // Палитра петли: 3–5 цветных ступеней лада, выбранных по частоте в ARPSY
        std::vector<int> cand;
        std::vector<double> candW;
        for (int d : c.scaleDeg) if (d != 0) { cand.push_back(d); candW.push_back(kColorW[d] + 0.05); }
        const int paletteSize = juce::jmin((int) cand.size(), 3 + rng.nextInt(3));
        for (int k = 0; k < paletteSize; ++k)
        {
            const int i = pickWeighted(candW, rng);
            if (i < 0) break;
            c.palette.push_back(cand[(size_t) i]);
            candW[(size_t) i] = 0;
        }

        const Bar a = makeBarA(c, juce::jlimit(0, 12, colorNotes), rng);

        // Схема тактов
        juce::String pattern;
        const int si = juce::jlimit(0, arpsyStructureNames().size() - 1, structure);
        if (si == 0)
        {
            std::vector<double> w;
            for (const auto& s : kStructures) w.push_back(s.weight);
            pattern = kStructures[pickWeighted(w, rng)].name;
        }
        else pattern = arpsyStructureNames()[si];

        // Блоки по 4 такта: A — сам рифф, остальные буквы — свежие ответы (в каждом блоке новые)
        std::vector<Bar> out;
        const int total = juce::jmax(1, bars);
        for (int block = 0; block * 4 < total; ++block)
        {
            std::map<juce::juce_wchar, Bar> letters;
            letters['A'] = a;
            for (int i = 0; i < 4 && block * 4 + i < total; ++i)
            {
                const auto ch = pattern[i];
                if (letters.find(ch) == letters.end()) letters[ch] = makeAnswer(c, a, out, rng);
                out.push_back(letters[ch]);
            }
        }

        Clip clip;
        for (size_t b = 0; b < out.size(); ++b)
            for (int p = 0; p < 16; ++p)
                if (out[b][(size_t) p] > 0) clip.add({ out[b][(size_t) p], (int) b * 16 + p, 1 });

        juce::StringArray colors;
        for (int d : c.palette) colors.add(kDeg[d]);
        infoOut.add(pattern + "  |  colors: " + colors.joinIntoString(" ") + "  |  " + juce::String(c.octaves) + " oct");
        return clip;
    }
}
