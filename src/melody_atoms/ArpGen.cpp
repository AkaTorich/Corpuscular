/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ArpGen.h"
#include "ArpTurnaround.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

using namespace mg;

namespace
{
    using Vec = std::vector<int>;

    //==========================================================================
    // Таблицы (порт noteOffsets и chordTypes из ARPGeneratorForm)
    //==========================================================================
    int nameToOffset(const juce::String& n)
    {
        static const std::map<juce::String, int> m = {
            {"C",0},{"C#",1},{"Db",1},{"D",2},{"D#",3},{"Eb",3},{"E",4},{"F",5},
            {"F#",6},{"Gb",6},{"G",7},{"G#",8},{"Ab",8},{"A",9},{"A#",10},{"Bb",10},{"B",11} };
        auto it = m.find(n); return it != m.end() ? it->second : 0;
    }

    juce::String offsetToName(int off) // FirstOrDefault по порядку словаря -> диез
    {
        static const char* s[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        return s[((off % 12) + 12) % 12];
    }

    struct ChordType { const char* name; Vec intervals; };
    const std::vector<ChordType>& chordTypes()
    {
        static const std::vector<ChordType> t = {
            {"Major",{0,4,7}},{"Minor",{0,3,7}},{"Diminished",{0,3,6}},{"Augmented",{0,4,8}},
            {"Sus2",{0,2,7}},{"Sus4",{0,5,7}},{"Major7",{0,4,7,11}},{"Minor7",{0,3,7,10}},
            {"Dominant7",{0,4,7,10}},{"Diminished7",{0,3,6,9}},{"Half-Diminished7",{0,3,6,10}},
            {"Minor-Major7",{0,3,7,11}},{"Augmented7",{0,4,8,10}},{"Add9",{0,4,7,14}},
            {"Add11",{0,4,7,17}},{"Add13",{0,4,7,21}},{"6",{0,4,7,9}},{"m6",{0,3,7,9}},
            {"Major9",{0,4,7,11,14}},{"Minor9",{0,3,7,10,14}},{"Dominant9",{0,4,7,10,14}},
            {"Major11",{0,4,7,11,14,17}},{"Minor11",{0,3,7,10,14,17}},{"Dominant11",{0,4,7,10,14,17}},
            {"Major13",{0,4,7,11,14,17,21}},{"Minor13",{0,3,7,10,14,17,21}},{"Dominant13",{0,4,7,10,14,17,21}},
            {"7b5",{0,4,6,10}},{"7#5",{0,4,8,10}},{"7b9",{0,4,7,10,13}},{"7#9",{0,4,7,10,15}},
            {"7#11",{0,4,7,10,14,18}},{"7b13",{0,4,7,10,14,17,20}},{"5",{0,7}},{"Quartal",{0,5,10}},
            {"Polychord C/G",{0,4,7,12,19}},{"Phrygian",{0,3,7,10,13}},{"Lydian",{0,4,7,11,18}},
            {"Hendrix",{0,4,7,10,15}},{"So What",{0,5,10,17,22}} };
        return t;
    }

    const Vec* chordIntervals(const juce::String& name)
    {
        for (auto& c : chordTypes()) if (name == c.name) return &c.intervals;
        return nullptr;
    }

    juce::String chordSuffix(const juce::String& t)
    {
        if (t=="Major") return ""; if (t=="Minor") return "m"; if (t=="Diminished") return "dim";
        if (t=="Augmented") return "aug"; if (t=="Sus2") return "sus2"; if (t=="Sus4") return "sus4";
        if (t=="Major7") return "maj7"; if (t=="Minor7") return "m7"; if (t=="Dominant7") return "7";
        if (t=="Diminished7") return "dim7"; if (t=="Half-Diminished7") return "m7b5";
        if (t=="Minor-Major7") return "mMaj7"; if (t=="Augmented7") return "aug7"; if (t=="Add9") return "add9";
        if (t=="Add11") return "add11"; if (t=="Add13") return "add13"; if (t=="6") return "6";
        if (t=="m6") return "m6"; if (t=="Major9") return "maj9"; if (t=="Minor9") return "m9";
        if (t=="Dominant9") return "9"; if (t=="Major11") return "maj11"; if (t=="Minor11") return "m11";
        if (t=="Dominant11") return "11"; if (t=="Major13") return "maj13"; if (t=="Minor13") return "m13";
        if (t=="Dominant13") return "13"; if (t=="Hendrix") return "7#9"; if (t=="So What") return "";
        if (t=="5") return "5"; if (t=="Quartal") return "quartal";
        return t;
    }

    //==========================================================================
    // Утилиты для нот
    //==========================================================================
    bool contains(const Vec& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

    int findExactInScale(int target, const Vec& scale) { return contains(scale, target) ? target : -1; }

    int findClosestInScale(int target, const Vec& scale)
    {
        if (scale.empty()) return -1;
        if (contains(scale, target)) return target;
        int closest = scale[0], minDist = std::abs(target - closest);
        for (int n : scale)
        {
            int d = std::abs(target - n);
            if (d < minDist) { minDist = d; closest = n; }
            else if (d == minDist && std::abs(n - 60) < std::abs(closest - 60)) closest = n;
        }
        int targetOct = target / 12, closeOct = closest / 12;
        if (std::abs(targetOct - closeOct) > 1)
        {
            Vec same;
            for (int n : scale) if (n % 12 == closest % 12 && std::abs(n / 12 - targetOct) <= 1) same.push_back(n);
            if (!same.empty())
            {
                int best = same[0];
                for (int n : same) if (std::abs(n/12 - targetOct) < std::abs(best/12 - targetOct)) best = n;
                closest = best;
            }
        }
        return closest;
    }

    int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

    Vec getScaleNotes(const juce::String& tonality, const juce::Array<int>& steps)
    {
        // Абсолютные интервалы = префиксные суммы шагов, включая октаву
        Vec absI; absI.push_back(0);
        for (int s : steps) absI.push_back(absI.back() + s);

        bool isMinor = tonality.endsWith("m");
        juce::String base = isMinor ? tonality.dropLastCharacters(1) : tonality;
        int tonicOff = nameToOffset(base);

        Vec notes;
        for (int oct = 3; oct <= 6; ++oct)
        {
            int baseMidi = oct * 12 + tonicOff;
            for (int interval : absI) notes.push_back(baseMidi + interval);
        }
        std::sort(notes.begin(), notes.end());
        return notes;
    }

    //==========================================================================
    // Совместимые типы аккордов
    //==========================================================================
    bool allNotesInScale(const Vec& intervals, int rootMidi, const Vec& scale)
    {
        for (int interval : intervals)
        {
            int nm = (rootMidi + interval) % 12;
            bool found = false;
            for (int n : scale) if (n % 12 == nm) { found = true; break; }
            if (!found) return false;
        }
        return true;
    }

    void addIfCompatible(std::vector<juce::String>& out, const std::vector<juce::String>& add,
                         const Vec& scale, bool isMinor)
    {
        int rootMidi = isMinor ? 9 : 0;
        for (auto& type : add)
        {
            auto* iv = chordIntervals(type);
            if (iv == nullptr) continue;
            if (allNotesInScale(*iv, rootMidi, scale) &&
                std::find(out.begin(), out.end(), type) == out.end())
                out.push_back(type);
        }
    }

    std::vector<juce::String> getCompatibleChordTypes(const juce::String& scaleName, bool isMinor, const Vec& scale)
    {
        std::vector<juce::String> result = { "Major","Minor","Major7","Minor7","Dominant7" };
        std::vector<juce::String> filtered;
        int rootMidi = isMinor ? 9 : 0;
        for (auto& c : chordTypes())
            if (allNotesInScale(c.intervals, rootMidi, scale))
                filtered.push_back(c.name);
        if (!filtered.empty()) result = filtered;

        if (scaleName.contains("Major") || scaleName.contains("Ionian"))
            addIfCompatible(result, { "Major9","Major13","6","Add9","Quartal" }, scale, isMinor);
        else if (scaleName.contains("Minor") || scaleName.contains("Aeolian"))
            addIfCompatible(result, { "Minor9","Minor11","m6","Half-Diminished7","Diminished" }, scale, isMinor);
        else if (scaleName.contains("Dorian"))
            addIfCompatible(result, { "Minor9","Minor13","m6","Dominant9","So What" }, scale, isMinor);
        return result;
    }

    Vec getGoodSteps(bool isMinor)
    {
        if (isMinor) return { 3,4,5,7,8,9 };
        return { 5,7,9,2,4,10 };
    }

    using Chord = std::pair<juce::String, juce::String>; // root, type

    std::vector<Chord> generateChordProgression(const juce::String& tonica, const juce::String& scaleName,
                                                int count, const Vec& scale, juce::Random& rng)
    {
        bool isMinor = tonica.endsWith("m");
        juce::String root = isMinor ? tonica.dropLastCharacters(1) : tonica;
        auto compatible = getCompatibleChordTypes(scaleName, isMinor, scale);

        std::vector<Chord> prog;
        prog.push_back({ root, isMinor ? "Minor" : "Major" });
        if (count > 1)
        {
            for (int i = 1; i < count - 1; ++i)
            {
                auto prevRoot = prog[i - 1].first;
                auto goodSteps = getGoodSteps(isMinor);
                int step = goodSteps[rng.nextInt((int)goodSteps.size())];
                int newOff = (nameToOffset(prevRoot) + step) % 12;
                juce::String newRoot = offsetToName(newOff);
                juce::String type = compatible[rng.nextInt((int)compatible.size())];
                prog.push_back({ newRoot, type });
            }
            prog.push_back({ root, isMinor ? "Minor" : "Major" });
        }
        return prog;
    }

    //==========================================================================
    // Базовый паттерн арпеджио
    //==========================================================================
    Vec createBaseArpeggioPattern(const Vec& scale, int arpNotesCount, juce::Random& rng)
    {
        Vec base;
        Vec main;
        for (int n : scale) if (n >= 60 && n <= 83) main.push_back(n);

        if ((int)main.size() >= 5)
        {
            base.push_back(main[0]);
            base.push_back(main[2]);
            base.push_back(main[4]);
            base.push_back(main[0] + 12);
        }
        else if (!main.empty())
        {
            base.push_back(main[0]);
            for (int i = 1; i < std::min(4, (int)main.size()); ++i)
                base.push_back(main[i % (int)main.size()]);
        }

        while ((int)base.size() < arpNotesCount)
        {
            if (!main.empty())
            {
                int cand = main[rng.nextInt((int)main.size())];
                if (base.empty() || base.back() != cand) base.push_back(cand);
                else
                {
                    Vec others;
                    for (int n : main) if (n != cand) others.push_back(n);
                    if (!others.empty()) base.push_back(others[rng.nextInt((int)others.size())]);
                    else
                    {
                        int up = cand + 12, dn = cand - 12;
                        if (contains(scale, up) && up <= 83) base.push_back(up);
                        else if (contains(scale, dn) && dn >= 60) base.push_back(dn);
                        else base.push_back(cand);
                    }
                }
            }
            else base.push_back(60);
        }

        // Разнообразие
        std::set<int> uniq(base.begin(), base.end());
        if (base.size() > 1 && uniq.size() <= 1)
        {
            Vec diff;
            for (int n : scale) if (n != base[0] && n >= 60 && n <= 83) diff.push_back(n);
            if (!diff.empty())
                for (int i = 1; i < (int)base.size(); i += 2)
                    base[i] = diff[rng.nextInt((int)diff.size())];
        }
        return base;
    }

    Vec getChordNotesInScale(const juce::String& rootName, const juce::String& chordType,
                             int octave, const Vec& scale, juce::Random& rng)
    {
        int rootOff = nameToOffset(rootName);
        int baseNote = octave * 12 + rootOff;
        auto* iv = chordIntervals(chordType);
        if (iv == nullptr) iv = chordIntervals("Major");

        Vec result;
        int rv = findExactInScale(baseNote, scale);
        if (rv != -1) result.push_back(rv);
        else { int c = findClosestInScale(baseNote, scale); if (c != -1) result.push_back(c); }

        for (size_t k = 1; k < iv->size(); ++k)
        {
            int noteToFind = baseNote + (*iv)[k];
            int inScale = findExactInScale(noteToFind, scale);
            if (inScale != -1) { if (!contains(result, inScale)) result.push_back(inScale); }
            else { int c = findClosestInScale(noteToFind, scale); if (c != -1 && !contains(result, c)) result.push_back(c); }
        }

        while ((int)result.size() < 3)
        {
            Vec avail;
            for (int n : scale) if (!contains(result, n) && n >= baseNote && n < baseNote + 12) avail.push_back(n);
            if (avail.empty()) break;
            result.push_back(avail[rng.nextInt((int)avail.size())]);
        }
        return result;
    }

    //==========================================================================
    // Контур, вариации мотива, ритм, артикуляция
    //==========================================================================
    void applyContour(Vec& notes, int contourType, int pos)
    {
        if ((int)notes.size() <= 1) return;
        Vec sorted = notes; std::sort(sorted.begin(), sorted.end());
        int n = (int)notes.size(), sn = (int)sorted.size();
        switch (contourType)
        {
            case 0:
                for (int i = 0; i < n; ++i)
                    notes[i] = sorted[std::min(i + pos / 2, sn - 1)];
                break;
            case 1:
                for (int i = 0; i < n; ++i)
                    notes[i] = sorted[std::min(n - 1 - i + pos / 2, sn - 1)];
                break;
            case 2:
                for (int i = 0; i < n; ++i)
                {
                    int dir = (i % 2 == 0) ? 1 : -1;
                    int mid = sn / 2;
                    int off = std::min(i / 2 + pos / 3, mid);
                    notes[i] = sorted[clampi(mid + dir * off, 0, sn - 1)];
                }
                break;
        }
    }

    Vec makeSimilarButNotIdentical(const Vec& motif, const Vec& scale, juce::Random& rng)
    {
        Vec result = motif;
        int vt = rng.nextInt(7);
        switch (vt)
        {
            case 0:
                for (auto& cur : result)
                    if (rng.nextDouble() < 0.6 && cur > 0)
                    {
                        static const int valid[] = { 2,3,4,5,7,9 };
                        int interval = valid[rng.nextInt(6)];
                        int dir = (rng.nextDouble() < 0.5) ? 1 : -1;
                        int target = cur + interval * dir;
                        if (contains(scale, target) && target >= 60 && target <= 83) cur = target;
                        else { int c = findClosestInScale(target, scale); if (c != -1) cur = c; }
                    }
                break;
            case 1:
                if ((int)result.size() > 3)
                {
                    int positions = std::min(2, (int)result.size() / 2);
                    for (int o = 0; o < positions; ++o)
                    {
                        int idx = rng.nextInt((int)result.size() - 2);
                        int cur = result[idx];
                        if (cur > 0)
                        {
                            Vec higher, lower;
                            for (int n : scale) { if (n > cur && n <= cur + 7) higher.push_back(n); if (n < cur && n >= cur - 7) lower.push_back(n); }
                            if (!higher.empty() && !lower.empty())
                            {
                                int hi = *std::min_element(higher.begin(), higher.end());
                                int lo = *std::max_element(lower.begin(), lower.end());
                                double ot = rng.nextDouble();
                                if (ot < 0.33) { result[idx] = hi; result[idx+1] = cur; if (idx+2 < (int)result.size()) result[idx+2] = lo; }
                                else if (ot < 0.66) { result[idx] = lo; result[idx+1] = cur; }
                                else { result[idx] = cur; if (idx+1<(int)result.size()) result[idx+1]=hi; if (idx+2<(int)result.size()) result[idx+2]=cur; if (idx+3<(int)result.size()) result[idx+3]=lo; }
                            }
                        }
                    }
                }
                break;
            case 2:
                if (result.size() > 1)
                {
                    int rot = 1 + rng.nextInt((int)result.size() - 1);
                    Vec tmp = result;
                    for (int i = 0; i < (int)result.size(); ++i) result[i] = tmp[(i + rot) % tmp.size()];
                }
                break;
            case 3:
                std::reverse(result.begin(), result.end());
                for (auto& v : result)
                    if (rng.nextDouble() < 0.3 && v > 0)
                    {
                        Vec nb; for (int n : scale) if (std::abs(n - v) <= 4 && n != v) nb.push_back(n);
                        if (!nb.empty()) v = nb[rng.nextInt((int)nb.size())];
                    }
                break;
            case 4:
                for (auto& v : result)
                    if (v > 0)
                    {
                        double ch = rng.nextDouble();
                        if (ch < 0.4 && v >= 60 && v <= 72) { int up = v + 12; if (contains(scale, up)) v = up; }
                        else if (ch < 0.7 && v >= 72) { int dn = v - 12; if (contains(scale, dn)) v = dn; }
                    }
                break;
            case 5:
                if (result.size() > 1)
                {
                    Vec dirs;
                    for (int i = 1; i < (int)result.size(); ++i)
                        dirs.push_back((result[i] > 0 && result[i-1] > 0) ? (result[i] > result[i-1] ? 1 : (result[i] < result[i-1] ? -1 : 0)) : 0);
                    for (int i = 1; i < (int)result.size(); ++i)
                        if (result[i] > 0)
                        {
                            int dir = (i - 1 < (int)dirs.size()) ? dirs[i-1] : 0;
                            if (dir != 0)
                            {
                                int step = 2 + rng.nextInt(6);
                                int target = result[i-1] + step * dir;
                                int c = findClosestInScale(target, scale);
                                if (c != -1) result[i] = c;
                            }
                        }
                }
                break;
            case 6:
                for (int i = 0; i < (int)result.size(); ++i)
                {
                    double ch = rng.nextDouble();
                    if (ch < 0.15 && i > 0 && i < (int)result.size() - 1) result[i] = 0;
                    else if (ch < 0.40 && result[i] > 0)
                    {
                        Vec acc; for (int n : scale) if (n != result[i] && std::abs(n - result[i]) >= 2 && std::abs(n - result[i]) <= 7 && n >= 60 && n <= 83) acc.push_back(n);
                        if (!acc.empty()) result[i] = acc[rng.nextInt((int)acc.size())];
                    }
                }
                break;
        }

        for (auto& v : result)
            if (v > 0 && !contains(scale, v)) { int c = findClosestInScale(v, scale); v = (c != -1) ? c : scale[rng.nextInt((int)scale.size())]; }

        for (int i = 1; i < (int)result.size(); ++i)
            if (result[i] > 0 && result[i] == result[i-1] && rng.nextDouble() < 0.7)
            {
                Vec alt; for (int n : scale) if (n != result[i] && std::abs(n - result[i]) <= 7 && n >= 60 && n <= 83) alt.push_back(n);
                if (!alt.empty()) result[i] = alt[rng.nextInt((int)alt.size())];
            }
        return result;
    }

    void applyRhythmicVariation(Vec& motif, int pt, bool useAccents, int position, juce::Random& rng)
    {
        if ((int)motif.size() <= 1) return;
        switch (pt)
        {
            case 0:
                if (useAccents && position % 2 == 0)
                    for (int i = 0; i < (int)motif.size(); ++i)
                        if (i % 2 == 0 && motif[i] > 0 && rng.nextDouble() < 0.7) motif[i] += 12;
                break;
            case 1:
                if ((int)motif.size() >= 3 && useAccents)
                    for (int i = 1; i < (int)motif.size(); i += 2)
                        if (motif[i] > 0 && rng.nextDouble() < 0.6) motif[i] += 7;
                break;
            case 2:
                if ((int)motif.size() >= 3)
                    for (int i = 0; i < (int)motif.size(); ++i)
                        if (i % 3 == 0 && motif[i] > 0 && useAccents && rng.nextDouble() < 0.5) motif[i] += 4;
                break;
            case 3:
                if ((int)motif.size() >= 4)
                {
                    int idx = 1 + rng.nextInt((int)motif.size() - 1);
                    if (rng.nextDouble() < 0.3) motif[idx] = 0;
                }
                break;
            default: break;
        }
    }

    void applyArticulationVariation(Vec& motif, juce::Random& rng)
    {
        int at = rng.nextInt(3);
        switch (at)
        {
            case 0:
                if ((int)motif.size() >= 4)
                {
                    int pauseCount = 1 + rng.nextInt((int)motif.size() / 3);
                    for (int i = 0; i < pauseCount; ++i)
                        motif[1 + rng.nextInt((int)motif.size() - 2)] = 0;
                }
                break;
            case 1:
                if ((int)motif.size() >= 3)
                {
                    int ri = rng.nextInt((int)motif.size());
                    if (ri < (int)motif.size() - 1 && motif[ri] > 0) motif[ri + 1] = motif[ri];
                }
                break;
            case 2:
                if ((int)motif.size() >= 3)
                {
                    int ai = rng.nextInt((int)motif.size());
                    if (motif[ai] > 0)
                    {
                        if (rng.nextDouble() < 0.5) motif[ai] += 12;
                        else motif[ai] = std::max(motif[ai] - 12, 48);
                    }
                }
                break;
        }
    }

    Vec adaptArpeggioToChord(const Vec& basePattern, const juce::String& rootNote, const juce::String& chordType,
                             int octave, const Vec& scale, int contourType, int pos, juce::Random& rng)
    {
        Vec chordNotes = getChordNotesInScale(rootNote, chordType, octave, scale, rng);
        Vec passing;
        for (int n : scale) if (n >= 60 && n <= 83 && !contains(chordNotes, n)) passing.push_back(n);

        Vec extended = chordNotes;
        if (!passing.empty())
        {
            int add = std::min(3, (int)passing.size());
            for (int i = 0; i < add; ++i)
            {
                int idx = rng.nextInt((int)passing.size());
                extended.push_back(passing[idx]);
                passing.erase(passing.begin() + idx);
            }
        }

        if (chordNotes.empty())
        {
            int rootMidi = octave * 12 + nameToOffset(rootNote);
            for (int n : scale) if (n >= rootMidi && n < rootMidi + 12) chordNotes.push_back(n);
            if (chordNotes.empty()) for (int n : scale) if (n >= 60 && n <= 83) chordNotes.push_back(n);
            extended = chordNotes;
        }
        if (chordNotes.empty()) return basePattern;

        Vec result = basePattern;
        if (pos == 0 && !chordNotes.empty()) result[0] = chordNotes[0];

        for (int i = (pos == 0) ? 1 : 0; i < (int)result.size(); ++i)
        {
            bool useExt = rng.nextDouble() < 0.4;
            const Vec& src = (useExt && extended.size() > chordNotes.size()) ? extended : chordNotes;
            if (i < (int)src.size()) result[i] = src[i];
            else if (!src.empty())
            {
                int index = i % (int)src.size();
                result[i] = src[index];
                if (i > 0 && result[i] == result[i-1] && src.size() > 1)
                    for (int j = 1; j < (int)src.size(); ++j)
                    {
                        int alt = (index + j) % (int)src.size();
                        if (src[alt] != result[i-1]) { result[i] = src[alt]; break; }
                    }
            }
        }

        applyContour(result, contourType, pos);

        for (int i = 1; i < (int)result.size(); ++i)
            if (i % 3 == 0 && rng.nextDouble() < 0.6)
            {
                Vec jumps;
                for (int n : scale) if (std::abs(n - result[i-1]) >= 4 && std::abs(n - result[i-1]) <= 9 && n >= 60 && n <= 83) jumps.push_back(n);
                if (!jumps.empty()) result[i] = jumps[rng.nextInt((int)jumps.size())];
            }

        for (auto& v : result)
            if (!contains(scale, v)) { int c = findClosestInScale(v, scale); v = (c != -1) ? c : scale[0]; }

        // Монотонность
        std::set<int> uniq(result.begin(), result.end());
        if (result.size() > 1 && uniq.size() <= 1)
        {
            for (int i = 1; i < (int)result.size(); i += 2)
            {
                Vec alt; for (int n : scale) if (n != result[0] && n >= 60 && n <= 83 && contains(chordNotes, n)) alt.push_back(n);
                if (!alt.empty()) result[i] = alt[rng.nextInt((int)alt.size())];
                else { Vec any; for (int n : scale) if (n != result[0] && n >= 60 && n <= 83) any.push_back(n); if (!any.empty()) result[i] = any[rng.nextInt((int)any.size())]; }
            }
        }
        else if (uniq.size() < 3 && result.size() >= 4)
        {
            std::map<int,int> counts; for (int n : result) counts[n]++;
            int mostCommon = result[0], best = 0;
            for (auto& kv : counts) if (kv.second > best) { best = kv.second; mostCommon = kv.first; }
            for (int i = 1; i < (int)result.size(); ++i)
                if (result[i] == mostCommon && result[i-1] == mostCommon)
                {
                    Vec alt; for (int n : scale) if (n != mostCommon && n >= 60 && n <= 83 && (contains(chordNotes, n) || rng.nextDouble() < 0.5)) alt.push_back(n);
                    if (!alt.empty()) result[i] = alt[rng.nextInt((int)alt.size())];
                }
        }
        return result;
    }

    Vec generateMeasure(const Vec& motif, const Vec& scale, int totalNotes, juce::Random& rng)
    {
        Vec measure;
        int rhythmPattern = rng.nextInt(7);
        bool useAccents = rng.nextDouble() < 0.8;

        Vec corrected;
        for (int note : motif)
        {
            if (note == 0 || contains(scale, note)) corrected.push_back(note);
            else { int c = findClosestInScale(note, scale); corrected.push_back(c != -1 ? c : scale[rng.nextInt((int)scale.size())]); }
        }

        // Монотонность мотива
        int firstNonZero = 0; for (int n : corrected) if (n > 0) { firstNonZero = n; break; }
        if (firstNonZero > 0)
        {
            bool mono = true;
            for (int n : corrected) if (n > 0 && n != firstNonZero) { mono = false; break; }
            if (mono)
            {
                Vec avail; for (int n : scale) if (n >= 60 && n <= 83 && n != firstNonZero) avail.push_back(n);
                if ((int)avail.size() >= 2)
                    for (int i = 0; i < (int)corrected.size(); ++i)
                        if (i % 2 == 1 && corrected[i] > 0) corrected[i] = avail[rng.nextInt((int)avail.size())];
            }
        }

        if (corrected.empty()) return Vec(totalNotes, 0);
        int motifCount = totalNotes / (int)corrected.size();
        int remainder = totalNotes % (int)corrected.size();
        double uniqMult = 0.8 + rng.nextDouble() * 0.2;

        for (int i = 0; i < motifCount; ++i)
        {
            Vec cur;
            if (i == 0)
                cur = (rng.nextDouble() < 0.4) ? makeSimilarButNotIdentical(corrected, scale, rng) : corrected;
            else
            {
                if (rng.nextDouble() < uniqMult)
                {
                    cur = makeSimilarButNotIdentical(corrected, scale, rng);
                    if (rng.nextDouble() < 0.4) std::reverse(cur.begin(), cur.end());
                    if (rng.nextDouble() < 0.4 && cur.size() > 2)
                    {
                        int rot = 1 + rng.nextInt((int)cur.size() - 1);
                        Vec rotated;
                        for (int j = 0; j < (int)cur.size(); ++j) rotated.push_back(cur[(j + rot) % cur.size()]);
                        cur = rotated;
                    }
                }
                else cur = corrected;
            }
            applyRhythmicVariation(cur, rhythmPattern, useAccents, i, rng);
            if (rng.nextDouble() < 0.3) applyArticulationVariation(cur, rng);
            measure.insert(measure.end(), cur.begin(), cur.end());
        }

        if (remainder > 0)
        {
            Vec rem(corrected.begin(), corrected.begin() + remainder);
            if (rng.nextDouble() < 0.5 && !scale.empty())
                for (auto& v : rem)
                    if (v > 0 && rng.nextDouble() < 0.6)
                    {
                        Vec alt; for (int n : scale) if (n != v && n >= 60 && n <= 83) alt.push_back(n);
                        if (!alt.empty()) v = alt[rng.nextInt((int)alt.size())];
                    }
            measure.insert(measure.end(), rem.begin(), rem.end());
        }

        for (auto& v : measure)
            if (v > 0 && !contains(scale, v)) { int c = findClosestInScale(v, scale); v = (c != -1) ? c : scale[rng.nextInt((int)scale.size())]; }

        // Монотонность всего такта
        Vec nz; for (int n : measure) if (n > 0) nz.push_back(n);
        if (nz.size() > 1)
        {
            std::map<int,int> counts; for (int n : nz) counts[n]++;
            int common = nz[0], best = 0;
            for (auto& kv : counts) if (kv.second > best) { best = kv.second; common = kv.first; }
            if ((double)best / nz.size() > 0.5)
            {
                Vec alt; for (int n : scale) if (n != common && n >= 60 && n <= 83) alt.push_back(n);
                if (!alt.empty())
                    for (auto& v : measure)
                        if (v == common && rng.nextDouble() < 0.7) v = alt[rng.nextInt((int)alt.size())];
            }
        }
        return measure;
    }

    std::vector<Vec> generateUniqueMeasures(const std::vector<Chord>& prog, int arpNotesCount,
                                            int uniqueCount, const Vec& scale, juce::Random& rng)
    {
        std::vector<Vec> measures;
        Vec basePattern = createBaseArpeggioPattern(scale, arpNotesCount, rng);
        for (auto& v : basePattern)
            if (!contains(scale, v)) { int c = findClosestInScale(v, scale); v = (c != -1) ? c : scale[rng.nextInt((int)scale.size())]; }

        int contourType = rng.nextInt(3);
        std::vector<Vec> uniqueMotifs;

        for (int i = 0; i < uniqueCount; ++i)
        {
            auto chord = prog[i % (int)prog.size()];
            Vec baseMotif = basePattern;
            if (i > 0 && !uniqueMotifs.empty())
            {
                if (i % 2 == 1 || rng.nextDouble() < 0.7)
                {
                    if (rng.nextDouble() < 0.6)
                        baseMotif = makeSimilarButNotIdentical(uniqueMotifs[rng.nextInt((int)uniqueMotifs.size())], scale, rng);
                    else
                        for (auto& v : baseMotif)
                            if (rng.nextDouble() < 0.5 && v > 0)
                            {
                                Vec alt; for (int n : scale) if (n != v && n >= 60 && n <= 83) alt.push_back(n);
                                if (!alt.empty()) v = alt[rng.nextInt((int)alt.size())];
                            }
                }
            }
            Vec motif = adaptArpeggioToChord(baseMotif, chord.first, chord.second, 4, scale, contourType, i, rng);
            uniqueMotifs.push_back(motif);
            Vec measure = generateMeasure(motif, scale, 16, rng);
            for (auto& v : measure)
                if (v > 0 && !contains(scale, v)) { int c = findClosestInScale(v, scale); v = (c != -1) ? c : scale[rng.nextInt((int)scale.size())]; }
            measures.push_back(measure);
        }
        return measures;
    }
}

//==============================================================================
Clip mg::generateArp(const juce::String& tonality, const juce::String& /*category*/,
                     const juce::String& scaleName, const juce::Array<int>& steps,
                     int arpNotesCount, int measuresTotal, int repeats,
                     juce::Random& rng, juce::StringArray& chordInfoOut)
{
    Clip clip;
    int actualUnique = measuresTotal - repeats;
    if (actualUnique < 1) actualUnique = 1;

    Vec scale = getScaleNotes(tonality, steps);
    if (scale.empty()) return clip;

    auto prog = generateChordProgression(tonality, scaleName, actualUnique, scale, rng);
    auto unique = generateUniqueMeasures(prog, arpNotesCount, actualUnique, scale, rng);

    // Повторы
    std::vector<Vec> repeated;
    if (!unique.empty())
        for (int i = 0; i < repeats; ++i)
            repeated.push_back(unique[rng.nextInt((int)unique.size())]);

    // Склейка: уникальные + повторы во случайные позиции
    std::vector<Vec> melody = unique;
    for (auto& r : repeated)
        melody.insert(melody.begin() + rng.nextInt((int)melody.size() + 1), r);

    // Текст аккордов
    for (int i = 0; i < (int)prog.size(); ++i)
        chordInfoOut.add(juce::String::fromUTF8("Chord ") + juce::String(i + 1) + ": " + prog[i].first + chordSuffix(prog[i].second));

    // Разворачиваем в сетку 16-х (как SaveMelodyToMidi): measure[i] -> позиция (i*16)/count
    int globalPos = 0;
    for (auto& measure : melody)
    {
        int measureNotes[16] = { 0 };
        int count = (int)measure.size();
        for (int i = 0; i < count; ++i)
        {
            int position = (count > 0) ? (i * 16) / count : 0;
            if (position < 16) measureNotes[position] = measure[i];
        }
        for (int p = 0; p < 16; ++p)
        {
            if (measureNotes[p] > 0)
                clip.add({ measureNotes[p], globalPos + p, 1 });
        }
        globalPos += 16;
    }

    return clip;
}

//==============================================================================
// Стилевой арпеджиатор (жанры) — непрерывные 16-е
//==============================================================================
const juce::StringArray& mg::arpGenreNames()
{
    static const juce::StringArray g {
        "PsyTrance", "Goa", "Full-On", "Dark Psy", "Hi-Tech",
        "Progressive", "Uplifting", "Up", "Down", "Up-Down", "Random" };
    return g;
}

namespace
{
    // Набор нот аккорда, расширенный на несколько октав (восходящий пул)
    Vec buildArpExt(const Vec& chordNotes, int octaves)
    {
        Vec base = chordNotes;
        std::sort(base.begin(), base.end());
        base.erase(std::unique(base.begin(), base.end()), base.end());
        Vec ext;
        int oc = juce::jlimit(1, 3, octaves);
        for (int o = 0; o < oc; ++o)
            for (int n : base) ext.push_back(n + 12 * o);
        std::sort(ext.begin(), ext.end());
        ext.erase(std::unique(ext.begin(), ext.end()), ext.end());
        return ext;
    }

    // 16 шестнадцатых одного такта согласно жанру (всегда без пауз)
    Vec fillArpMeasure(const juce::String& genre, const Vec& ext, juce::Random& rng)
    {
        Vec r(16, 0);
        if (ext.empty()) { std::fill(r.begin(), r.end(), 60); return r; }
        const int m = (int)ext.size();
        auto at = [&](int i) { i = ((i % m) + m) % m; return ext[i]; };

        if (genre.equalsIgnoreCase("PsyTrance"))
        {
            // Классический ролл: чётные позиции — тоника, нечётные — восходящие тоны
            int j = 1;
            for (int i = 0; i < 16; ++i) r[i] = (i % 2 == 0) ? ext[0] : at(j++);
        }
        else if (genre.equalsIgnoreCase("Up") || genre.equalsIgnoreCase("Uplifting"))
        {
            for (int i = 0; i < 16; ++i) r[i] = at(i);
        }
        else if (genre.equalsIgnoreCase("Down"))
        {
            for (int i = 0; i < 16; ++i) r[i] = at(-i);
        }
        else if (genre.equalsIgnoreCase("Up-Down") || genre.equalsIgnoreCase("Goa")
                 || genre.equalsIgnoreCase("Progressive"))
        {
            std::vector<int> seq;                  // треугольник 0..m-1..1
            for (int i = 0; i < m; ++i) seq.push_back(i);
            for (int i = m - 2; i >= 1; --i) seq.push_back(i);
            int sl = juce::jmax(1, (int)seq.size());
            bool slow = genre.equalsIgnoreCase("Progressive");
            for (int i = 0; i < 16; ++i) { int s = slow ? (i / 2) : i; r[i] = ext[seq[s % sl]]; }
        }
        else if (genre.equalsIgnoreCase("Full-On"))
        {
            for (int i = 0; i < 16; ++i) { r[i] = at(i); if (i % 4 == 0) r[i] += 12; }
        }
        else if (genre.equalsIgnoreCase("Dark Psy"))
        {
            // Ниже регистром, плотное чередование тоника/сосед
            for (int i = 0; i < 16; ++i) { int n = (i % 2 == 0) ? ext[0] : at(1); r[i] = juce::jmax(24, n - 12); }
        }
        else if (genre.equalsIgnoreCase("Hi-Tech"))
        {
            int idx = 0;
            for (int i = 0; i < 16; ++i) { r[i] = at(idx); idx += (rng.nextBool() ? 1 : -1) * (1 + rng.nextInt(2)); }
        }
        else if (genre.equalsIgnoreCase("Random"))
        {
            for (int i = 0; i < 16; ++i) r[i] = ext[rng.nextInt(m)];
        }
        else // по умолчанию — Up
        {
            for (int i = 0; i < 16; ++i) r[i] = at(i);
        }
        return r;
    }
}

//==============================================================================
// Пситранс-арп: короткий мелодический риф (ячейка), повторяется непрерывно
//==============================================================================
namespace
{
    static const char* kChrom[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

    // Веса ступеней из анализа 515 пситранс-арпов (тоника и ♭2 доминируют)
    static const int kDegW[12] = { 39, 31, 4, 3, 3, 4, 2, 4, 4, 1, 3, 2 };

    // Все ноты заданного класса высоты в регистре [lo,hi]
    Vec octavesOf(int pc, int lo, int hi)
    {
        Vec v; int base = ((pc % 12) + 12) % 12;
        for (int n = lo; n <= hi; ++n) if (((n % 12) + 12) % 12 == base) v.push_back(n);
        if (v.empty()) v.push_back(juce::jlimit(0, 127, base + 48));
        return v;
    }

    // Ячейка пситранс-арпа: непрерывные ноты, в основном тоника/♭2,
    // «мелодия» из октавных скачков той же ноты (как в реальных арпах).
    Vec makeArpCell(const Vec& scalePcs, int tonicPc, int cellLen, int lo, int hi, juce::Random& rng)
    {
        // Взвешенный пул классов высоты по ступеням лада
        std::vector<int> pool;
        for (int pc : scalePcs)
        {
            int deg = (((pc - tonicPc) % 12) + 12) % 12;
            int w = juce::jmax(1, kDegW[deg]);
            for (int k = 0; k < w; ++k) pool.push_back(pc);
        }
        if (pool.empty()) pool.push_back(tonicPc);

        Vec cell; int prev = -1, prevPc = -1;
        for (int k = 0; k < cellLen; ++k)
        {
            double r = rng.nextDouble();
            int note;
            if (prev >= 0 && r < 0.15)                 // 1) повтор ноты (грув)
                note = prev;
            else if (prev >= 0 && r < 0.45)            // 2) октавный скачок ТОЙ ЖЕ ноты
            {
                Vec v = octavesOf(prevPc, lo, hi), o;
                for (int n : v) if (n != prev) o.push_back(n);
                if (o.empty()) o = v;
                note = o[rng.nextInt((int)o.size())];
            }
            else                                       // 3) новая нота (взвеш. ступень) в случ. октаве
            {
                int pc = pool[rng.nextInt((int)pool.size())];
                Vec v = octavesOf(pc, lo, hi);
                note = v[rng.nextInt((int)v.size())];
            }
            cell.push_back(note);
            prev = note; prevPc = ((note % 12) + 12) % 12;
        }
        return cell;
    }

    // Вариация: пара нот переносится в другую октаву (характерно для psy)
    Vec varyArpCell(const Vec& cell, int lo, int hi, juce::Random& rng)
    {
        Vec c = cell;
        if (c.empty()) return c;
        int changes = 1 + rng.nextInt(2);
        for (int t = 0; t < changes; ++t)
        {
            int i = rng.nextInt((int)c.size());
            Vec v = octavesOf(c[i] % 12, lo, hi);
            c[i] = v[rng.nextInt((int)v.size())];
        }
        return c;
    }

    // Вариационная нота для «филла»: ступень лада (взвеш.) в регистре, отличная от avoid
    int pickVariationNote(const Vec& scalePcs, int tonicPc, int avoid, int lo, int hi, juce::Random& rng)
    {
        std::vector<int> pool;
        for (int pc : scalePcs)
        {
            int deg = (((pc - tonicPc) % 12) + 12) % 12;
            int w = juce::jmax(1, kDegW[deg]);
            for (int k = 0; k < w; ++k) pool.push_back(pc);
        }
        if (pool.empty()) return avoid;
        for (int tries = 0; tries < 8; ++tries)
        {
            int pc = pool[rng.nextInt((int)pool.size())];
            Vec v = octavesOf(pc, lo, hi);
            int n = v[rng.nextInt((int)v.size())];
            if (n != avoid) return n;
        }
        return avoid;
    }
}

Clip mg::generateArpRiff(const juce::String& tonality, const juce::Array<int>& steps,
                         int notes, int octaves, int measuresTotal, int repeats,
                         juce::Random& rng, juce::StringArray& infoOut, int endingStyle)
{
    juce::ignoreUnused(repeats);
    Clip clip;

    // notes = «количество нот». 4 — нейтраль (чистый риф, как сейчас).
    // Больше 4 → в КЛИПЕ появляется вариация: глубина = notes-4 (1..4 ноты).
    int N = juce::jlimit(1, 16, notes);
    const int baseCellLen = 4;                       // базовый мотив — 4 ноты (монотонный риф)
    int variation = juce::jlimit(0, 4, N - 4);       // сколько нот варьируется с конца такта
    int totalBars = juce::jmax(1, measuresTotal);

    // Классы высоты лада + тоника
    int tonicPc = ((nameToOffset(tonality.endsWith("m") ? tonality.dropLastCharacters(1) : tonality)) % 12 + 12) % 12;
    Vec scalePcs;
    {
        int idx = tonicPc; scalePcs.push_back(idx % 12);
        for (int s : steps) { idx = (idx + s) % 12; scalePcs.push_back(idx); }
        std::sort(scalePcs.begin(), scalePcs.end());
        scalePcs.erase(std::unique(scalePcs.begin(), scalePcs.end()), scalePcs.end());
    }
    if (scalePcs.empty()) scalePcs.push_back(tonicPc);

    // Регистр: ~2 октавы (как в реальных арпах, span ~24 полутона), октавные скачки внутри
    int oc = juce::jlimit(1, 3, octaves);
    int lo = 48, hi = 48 + 12 * oc;

    // Базовый мотив (4 ноты) — повторяется во ВСЕХ тактах одинаково (монотонность)
    Vec base = makeArpCell(scalePcs, tonicPc, baseCellLen, lo, hi, rng);

    // Текст: ноты базового мотива
    for (int v : base) infoOut.add(juce::String(kChrom[((v % 12) + 12) % 12]) + juce::String(v / 12 - 1));

    // Развёртка. Мотив тайлится по 16 шестнадцатым.
    // 4-й такт каждой фразы (4, 8, 12…) — музыкальная концовка: 1–3 последние ноты,
    // каждый раз другая, по выбранному стилю (ArpTurnaround, список «4th bar ending»).
    // 2-й такт фразы (2, 6, 10…) — прежняя вариация последней четверти при notes > 4.
    mg::ArpTurnaround turnaround((mg::ArpEnding) juce::jlimit(0, mg::arpEndingNames().size() - 1, endingStyle),
                                 base, scalePcs, tonicPc, lo, hi, totalBars);
    int globalPos = 0;
    for (int bar = 0; bar < totalBars; ++bar)
    {
        if (bar % 4 == 3)
        {
            const auto ending = turnaround.makeBar(bar, rng);
            for (int p = 0; p < 16; ++p) if (ending[(size_t) p] > 0) clip.add({ ending[(size_t) p], globalPos + p, 1 });
            globalPos += 16;
            continue;
        }
        bool evenBar = (bar % 4 == 1); // 2-й такт фразы: 2-й, 6-й, 10-й…
        for (int p = 0; p < 16; ++p)
        {
            int note = base[p % baseCellLen];
            // Вариация: только чётный такт, только последняя четверть, только при variation>0
            if (evenBar && variation > 0 && p >= 16 - variation)
                note = pickVariationNote(scalePcs, tonicPc, note, lo, hi, rng);
            if (note > 0) clip.add({ note, globalPos + p, 1 });
        }
        globalPos += 16;
    }
    return clip;
}

Clip mg::generateArpStyled(const juce::String& tonality, const juce::String& scaleName,
                           const juce::Array<int>& steps, const juce::String& genre,
                           int octaves, int measuresTotal, int repeats,
                           juce::Random& rng, juce::StringArray& chordInfoOut)
{
    Clip clip;
    int actualUnique = measuresTotal - repeats;
    if (actualUnique < 1) actualUnique = 1;

    Vec scale = getScaleNotes(tonality, steps);
    if (scale.empty()) return clip;

    auto prog = generateChordProgression(tonality, scaleName, actualUnique, scale, rng);

    std::vector<Vec> unique;
    for (int i = 0; i < actualUnique; ++i)
    {
        auto chord = prog[i % (int)prog.size()];
        Vec chordNotes = getChordNotesInScale(chord.first, chord.second, 4, scale, rng);
        if (chordNotes.empty())
            for (int n : scale) if (n >= 60 && n <= 83) chordNotes.push_back(n);
        Vec ext = buildArpExt(chordNotes, octaves);
        unique.push_back(fillArpMeasure(genre, ext, rng));
    }

    std::vector<Vec> repeatedM;
    if (!unique.empty())
        for (int i = 0; i < repeats; ++i)
            repeatedM.push_back(unique[rng.nextInt((int)unique.size())]);
    std::vector<Vec> melody = unique;
    for (auto& r : repeatedM)
        melody.insert(melody.begin() + rng.nextInt((int)melody.size() + 1), r);

    for (int i = 0; i < (int)prog.size(); ++i)
        chordInfoOut.add(juce::String::fromUTF8("Chord ") + juce::String(i + 1) + ": "
                         + prog[i].first + chordSuffix(prog[i].second));

    int globalPos = 0;
    for (auto& measure : melody)
    {
        for (int p = 0; p < 16 && p < (int)measure.size(); ++p)
            if (measure[p] > 0) clip.add({ measure[p], globalPos + p, 1 });
        globalPos += 16;
    }
    return clip;
}
