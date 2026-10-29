/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ArpTurnaround.h"
#include <algorithm>
#include <cmath>

namespace mg
{
    namespace
    {
        int pcOf(int n) { return ((n % 12) + 12) % 12; }
    }

    const juce::StringArray& arpEndingNames()
    {
        static const juce::StringArray names { "question / answer", "phrase hierarchy", "gap before downbeat",
                                               "b2 resolution", "riff sequence", "growing energy" };
        return names;
    }

    ArpTurnaround::ArpTurnaround(ArpEnding s, const std::vector<int>& r, const std::vector<int>& pcs,
                                 int tonic, int low, int high, int bars)
        : style(s), riff(r.empty() ? std::vector<int> { 60 } : r), scalePcs(pcs),
          tonicPc(tonic), lo(low), hi(high), totalBars(juce::jmax(1, bars))
    {
        if (scalePcs.empty()) scalePcs.push_back(tonicPc);
        // «Лестница» нот лада с запасом в октаву с обеих сторон — по ней ходим ступенями
        for (int n = lo - 12; n <= hi + 12; ++n)
            if (inScale(pcOf(n))) ladder.push_back(n);
    }

    // ---------- помощники ----------

    bool ArpTurnaround::inScale(int pc) const
    {
        return std::find(scalePcs.begin(), scalePcs.end(), pcOf(pc)) != scalePcs.end();
    }

    int ArpTurnaround::ladderIndex(int note) const
    {
        int idx = 0, best = 1 << 30;
        for (int i = 0; i < (int) ladder.size(); ++i)
        {
            const int d = std::abs(ladder[(size_t) i] - note);
            if (d < best) { best = d; idx = i; }
        }
        return idx;
    }

    int ArpTurnaround::step(int note, int steps, int top) const
    {
        if (ladder.empty()) return note;
        if (top < 0) top = hi + 12;   // фигура строится свободно, в регистр её ставит fitFigure целиком
        int n = ladder[(size_t) juce::jlimit(0, (int) ladder.size() - 1, ladderIndex(note) + steps)];
        while (n < lo) n += 12;      // в регистр — октавой
        while (n > top) n -= 12;
        return n;
    }

    int ArpTurnaround::nearestPc(int pc, int near, int top) const
    {
        int best = near, bestDist = 1 << 30;
        for (int n = lo - 12; n <= top; ++n)
            if (pcOf(n) == pcOf(pc) && std::abs(n - near) < bestDist) { bestDist = std::abs(n - near); best = n; }
        return best;
    }

    int ArpTurnaround::nearestWithInterval(int interval, int near) const
    {
        // Ступень лада, ближайшая к тоника+interval (в ладах без чистой квинты — соседняя)
        int pc = scalePcs[0], bestD = 99;
        for (int p : scalePcs)
        {
            const int rel = pcOf(p - tonicPc);
            const int d = std::min(std::abs(rel - interval), 12 - std::abs(rel - interval));
            if (d < bestD) { bestD = d; pc = p; }
        }
        int best = near, bestDist = 1 << 30;
        for (int n = lo; n <= hi; ++n)
            if (pcOf(n) == pc && std::abs(n - near) < bestDist) { bestDist = std::abs(n - near); best = n; }
        return best;
    }

    void ArpTurnaround::fitFigure(std::vector<int>& end) const
    {
        // Переносим всю фигуру октавой, чтобы не рвать её поштучным «загибанием» нот
        for (int guard = 0; guard < 4; ++guard)
        {
            int top = 0, bottom = 1 << 30;
            for (int v : end) if (v > 0) { top = juce::jmax(top, v); bottom = juce::jmin(bottom, v); }
            if (top == 0) return;
            if (top > hi + 7 && bottom - 12 >= lo - 5) for (auto& v : end) { if (v > 0) v -= 12; }
            else if (bottom < lo - 5 && top + 12 <= hi + 7) for (auto& v : end) { if (v > 0) v += 12; }
            else return;
        }
    }

    int ArpTurnaround::randomCount(juce::Random& rng) const
    {
        const double r = rng.nextDouble();
        return r < 0.3 ? 1 : (r < 0.7 ? 2 : 3);
    }

    void ArpTurnaround::makeDifferent(std::vector<int>& end) const
    {
        const int size = (int) end.size();
        for (int i = 0; i < size; ++i)
        {
            const int orig = riffAt(16 - size + i);
            if (end[(size_t) i] > 0 && end[(size_t) i] == orig) end[(size_t) i] = step(orig, i % 2 == 0 ? 1 : -1);
        }
    }

    // ---------- стили ----------

    // Вопрос — уход на напряжение; ответ — подход к первой ноте риффа (начало следующего такта)
    std::vector<int> ArpTurnaround::questionAnswer(int count, bool answer, juce::Random& rng) const
    {
        const int target = riffAt(0);
        std::vector<int> out((size_t) count);
        const int figure = answer ? rng.nextInt(3) : 3 + rng.nextInt(3);
        switch (figure)
        {
            case 0:   // ответ: сверху по ступеням в первую ноту риффа
                for (int i = 0; i < count; ++i) out[(size_t) i] = step(target, count - i);
                break;
            case 1:   // ответ: снизу по ступеням
                for (int i = 0; i < count; ++i) out[(size_t) i] = step(target, -(count - i));
                break;
            case 2:   // ответ: обыгрывание — соседи сверху и снизу вокруг цели
            {
                static const int around[3] = { 2, 1, -1 };
                for (int i = 0; i < count; ++i) out[(size_t) i] = step(target, around[3 - count + i]);
                break;
            }
            case 3:   // вопрос: поступенный подъём к квинте лада
            {
                const int fifth = nearestWithInterval(7, riffAt(15));
                for (int i = 0; i < count; ++i) out[(size_t) i] = step(fifth, -(count - 1 - i));
                break;
            }
            case 4:   // вопрос: октавный выкрик последней ноты риффа (вверх, можно на квинту выше регистра)
            {
                const int last = riffAt(15);
                const int shout = last + 12 <= hi + 7 ? last + 12 : last - 12;
                for (int i = 0; i < count; ++i) out[(size_t) i] = step(shout, -2 * (count - 1 - i), hi + 7);
                out[(size_t) count - 1] = shout;
                break;
            }
            default:  // вопрос: вниз по звукам тонического трезвучия 5 — 3 — 1
            {
                const int third = inScale(tonicPc + 3) ? 3 : 4;   // терция лада: малая, если она в ладу
                const int chord[3] = { 7, third, 0 };
                int near = riffAt(16 - count);
                for (int i = 0; i < count; ++i)
                {
                    out[(size_t) i] = nearestWithInterval(chord[3 - count + i], near);
                    near = out[(size_t) i];
                }
                break;
            }
        }
        return out;
    }

    // Иерархия фраз: такт 4 — одна нота, 8 — две, 16 — три октавой выше, 32 — пробежка на всю долю
    std::vector<int> ArpTurnaround::hierarchy(int bar, juce::Random& rng) const
    {
        const int n = bar + 1;
        const bool answer = (bar / 4) % 2 == 1;
        const int target = riffAt(0);

        if (n % 32 == 0)   // граница большой части: каскад сверху (с октавы выше) через ступень в начало риффа
        {
            const int top = step(target, 7, hi + 12);
            return { step(top, -1, hi + 12), step(top, -3, hi + 12), step(top, -5, hi + 12), step(target, 1, hi + 12) };
        }
        if (n % 16 == 0)   // три ноты и переход в другую октаву — всей фигурой сразу
        {
            auto e = questionAnswer(3, answer, rng);
            const bool up = std::all_of(e.begin(), e.end(), [this](int v) { return v + 12 <= hi + 7; });
            const bool down = std::all_of(e.begin(), e.end(), [this](int v) { return v - 12 >= lo - 5; });
            if (up || down) for (auto& v : e) v += up ? 12 : -12;
            return e;
        }
        if (n % 8 == 0) return questionAnswer(2, answer, rng);
        static const int nudge[4] = { 1, -1, 2, -2 };        // такт 4: одна нота — лёгкий подход к началу риффа
        return { step(target, nudge[rng.nextInt(4)]) };
    }

    // Пауза перед сильной долей, «заикание» или удар после тишины
    std::vector<int> ArpTurnaround::gap(juce::Random& rng) const
    {
        const int held = riffAt(12);
        const int hit = riffAt(0) + 12 <= hi + 7 ? riffAt(0) + 12 : riffAt(0);
        switch (rng.nextInt(5))
        {
            case 0:  return { 0 };                   // последняя шестнадцатая молчит
            case 1:  return { 0, 0 };                // две молчат
            case 2:  return { held, held, held };    // заикание: нота доли повторяется
            case 3:  return { 0, 0, hit };           // тишина и удар октавой выше
            default: return { 0, held, held };       // вдох и повтор
        }
    }

    // Фригийское напряжение: концовка встаёт на ♭2 (сверху) или на вводный тон (снизу)
    // и разрешается в тонику. Обе ноты берутся хроматически, даже если их нет в ладу —
    // в этом и смысл стиля: C–B–B♭ → A сверху, E–F–G# → A снизу (для ля минора).
    std::vector<int> ArpTurnaround::flatTwo(int count, juce::Random& rng) const
    {
        const int target = riffAt(0);
        const bool fromAbove = rng.nextDouble() < 0.65;
        const int tension = fromAbove ? nearestPc(tonicPc + 1, target + 1, hi + 7)
                                      : nearestPc(tonicPc + 11, target - 1, hi);

        std::vector<int> out((size_t) count);
        // сверху: ♭3 → ♭2 (или 4 → ♭3 → ♭2) — нисходящий фригийский ход; снизу: 6 → 7 — восходящий
        for (int i = 0; i < count; ++i)
            out[(size_t) i] = step(tension, (fromAbove ? 1 : -1) * (count - 1 - i), hi + 7);
        out[(size_t) count - 1] = tension;
        return out;
    }

    // Концовка из самого риффа: его последние ноты со сдвигом на ступень, обращением или ходом назад
    std::vector<int> ArpTurnaround::sequence(int count, juce::Random& rng) const
    {
        count = juce::jmax(2, count);
        std::vector<int> src((size_t) count), out((size_t) count);
        for (int i = 0; i < count; ++i) src[(size_t) i] = riffAt(16 - count + i);

        switch (rng.nextInt(5))
        {
            case 0: for (int i = 0; i < count; ++i) out[(size_t) i] = step(src[(size_t) i], 1); break;    // на ступень вверх
            case 1: for (int i = 0; i < count; ++i) out[(size_t) i] = step(src[(size_t) i], 2); break;    // на терцию вверх
            case 2: for (int i = 0; i < count; ++i) out[(size_t) i] = step(src[(size_t) i], -1); break;   // на ступень вниз
            case 3:                                                                                         // ход назад
                out.assign(src.rbegin(), src.rend());
                if (out == src) for (auto& v : out) v = step(v, 1);
                break;
            default:                                                                                        // обращение
            {
                const int pivot = ladderIndex(src[0]);
                for (int i = 0; i < count; ++i)
                {
                    const int idx = juce::jlimit(0, (int) ladder.size() - 1, 2 * pivot - ladderIndex(src[(size_t) i]));
                    out[(size_t) i] = step(ladder[(size_t) idx], 0);
                }
                break;
            }
        }
        return out;
    }

    // Рост к концу клипа: число нот 1 → 3, в последней трети — октавой выше, в последней фразе — выкрик
    std::vector<int> ArpTurnaround::growth(int phrase, juce::Random& rng) const
    {
        const int phrases = juce::jmax(1, totalBars / 4);
        const double progress = phrases > 1 ? juce::jlimit(0.0, 1.0, phrase / (double) (phrases - 1)) : 1.0;
        const int count = 1 + (int) std::lround(2.0 * progress);
        // первая треть — только мягкие поступенные подходы («ответы»), дальше — вопрос и ответ по очереди
        auto e = questionAnswer(count, progress < 0.33 ? true : phrase % 2 == 1, rng);

        if (progress >= 0.66 && std::all_of(e.begin(), e.end(), [this](int v) { return v + 12 <= hi + 7; }))
            for (auto& v : e) v += 12;
        if (progress >= 0.99)
        {
            const int last = riffAt(15);
            e.back() = last + 12 <= hi + 7 ? last + 12 : e.back();
        }
        return e;
    }

    // ---------- такт с концовкой ----------

    std::vector<int> ArpTurnaround::makeBar(int bar, juce::Random& rng)
    {
        const int phrase = bar / 4;
        const bool answer = phrase % 2 == 1;
        std::vector<int> base(16);
        for (int p = 0; p < 16; ++p) base[(size_t) p] = riffAt(p);

        // Концовка не повторяет ни одну прежнюю; если за 24 попытки новой не нашлось —
        // хотя бы отличается от предыдущей
        for (int tries = 0; tries < 32; ++tries)
        {
            std::vector<int> end;
            switch (style)
            {
                case ArpEnding::Hierarchy: end = hierarchy(bar, rng); break;
                case ArpEnding::Gap:       end = gap(rng); break;
                case ArpEnding::FlatTwo:   end = flatTwo(randomCount(rng), rng); break;
                case ArpEnding::Sequence:  end = sequence(randomCount(rng), rng); break;
                case ArpEnding::Growth:    end = growth(phrase, rng); break;
                default:                   end = questionAnswer(randomCount(rng), answer, rng); break;
            }
            if (end.empty() || end.size() > 16) continue;
            fitFigure(end);
            if (style != ArpEnding::Gap) makeDifferent(end);   // в Gap перемена — сама пауза или повтор

            std::vector<int> candidate = base;
            for (size_t i = 0; i < end.size(); ++i) candidate[16 - end.size() + i] = end[i];
            const std::vector<int> tail(candidate.end() - 4, candidate.end());
            const bool seen = std::find(usedTails.begin(), usedTails.end(), tail) != usedTails.end();
            const bool sameAsPrevious = !usedTails.empty() && usedTails.back() == tail;
            if ((tries < 24 && !seen) || (tries >= 24 && !sameAsPrevious) || tries == 31)
            {
                usedTails.push_back(tail);
                return candidate;
            }
        }
        return base;
    }
}
