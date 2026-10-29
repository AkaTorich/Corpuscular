/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "BassGen.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>

using namespace mg;

namespace
{
    using Vec = std::vector<int>;

    int nameToOffset(const juce::String& n)
    {
        static const std::map<juce::String, int> m = {
            {"C",0},{"C#",1},{"Db",1},{"D",2},{"D#",3},{"Eb",3},{"E",4},{"F",5},
            {"F#",6},{"Gb",6},{"G",7},{"G#",8},{"Ab",8},{"A",9},{"A#",10},{"Bb",10},{"B",11} };
        auto it = m.find(n); return it != m.end() ? it->second : 0;
    }
    juce::String offsetToName(int off)
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
        if (t=="Augmented") return "aug"; if (t=="Major7") return "maj7"; if (t=="Minor7") return "m7";
        if (t=="Dominant7") return "7"; if (t=="Diminished7") return "dim7"; if (t=="Half-Diminished7") return "m7b5";
        if (t=="Major9") return "maj9"; if (t=="Minor9") return "m9"; if (t=="Dominant9") return "9";
        if (t=="5") return "5"; return t;
    }

    struct BassPattern { const char* name; Vec intervals; };
    const std::vector<BassPattern>& bassPatterns()
    {
        static const std::vector<BassPattern> p = {
            {"Root Note",{0}},{"Root-Fifth",{0,7}},{"Root-Octave",{0,12}},
            {"Arpeggiated Triad",{0,4,7}},{"Arpeggiated Minor",{0,3,7}},
            {"Walking Bass",{0,4,7,9,7,4,2,0}},{"Boogie",{0,0,0,0,7,7,7,7}},{"Blues",{0,3,5,6,7,6,5,3}},
            {"Rock",{0,0,7,7,0,0,5,7}},{"Hard Rock",{0,0,7,0,12,12,7,0}},
            {"Metal",{0,0,0,0,7,7,7,7,5,5,3,3,0,0,0,0}},{"Thrash Metal",{0,0,0,5,7,7,7,5,0,0,0,12}},
            {"Progressive Metal",{0,3,7,12,7,5,3,0,5,7,12,15}},
            {"Funk",{0,7,10,7,12,10,7,5,4,5,7,5,0}},{"R&B",{0,5,7,9,10,9,7,5,4,0,4,5}},
            {"Soul",{0,3,7,10,12,10,7,3,0,3,5,7}},{"Disco",{0,7,0,7,12,7,12,0,7,0,5,7,5,0}},
            {"Techno",{0,0,0,0,12,12,12,12,7,7,7,7,5,5,5,5}},{"House",{0,0,7,7,12,12,7,7,0,0,5,5,7,7,5,5}},
            {"Trance",{0,4,7,12,7,4,0,0,5,9,12,9,5,0,0,0}},{"PsyTrance",{0,0,7,0,0,12,7,0,0,7,5,0,5,7,12,0}},
            {"Acid",{0,3,7,10,7,3,7,10,7,3,0,3,7,3,7,10}},{"Drum & Bass",{0,0,7,0,5,0,12,0,7,0,5,7,5,0,12,5}},
            {"Jazz",{0,4,7,11,9,7,5,4,2,0,11,9}},{"Bebop",{0,4,7,9,11,10,9,7,6,4,2,0}},
            {"Jazz Waltz",{0,7,4,7,12,9,7,4,0}},{"Blues Shuffle",{0,0,3,5,6,5,3,0}},
            {"Salsa",{0,5,7,10,7,5,3,0,0,3,5,7,3,5,7,10}},{"Bossa Nova",{0,7,10,14,12,10,7,5,4,0,2,4,5}},
            {"Samba",{0,7,10,7,3,0,3,7,10,7,3,0,7,10,12,14}},{"Reggae",{0,0,0,5,7,0,0,5,0,0,0,5,7,0,0,5}},
            {"Celtic",{0,2,4,7,9,7,4,2,0,2,4,7}},{"Middle Eastern",{0,1,4,5,7,5,4,1,0,1,4,5}},
            {"Indian",{0,5,7,10,12,10,7,5,0,5,7,10}},{"African",{0,2,5,7,9,7,5,2,0,2,5,7}},
            {"Hip-Hop",{0,7,0,12,0,7,5,3,0,7,12,7,0,5,3,0}},{"Trap",{0,0,0,0,7,7,7,7,0,5,7,5,0,0,5,7}},
            {"Lo-Fi",{0,3,5,7,10,7,5,3,0,3,5,7,5,3,0,0}},{"Future Bass",{0,4,7,11,12,11,7,4,0,4,7,11,7,4,0,0}},
            {"Breakbeat",{0,7,0,12,0,7,5,0,7,0,12,7,5,0,7,12}},{"UK Garage",{0,0,7,0,12,0,7,0,5,0,7,0,0,5,7,12}},
            {"Dubstep",{0,0,0,0,7,7,7,7,12,12,12,12,0,0,0,0}},{"Glitch Hop",{0,5,7,0,12,7,5,0,7,5,0,7,5,0,7,12}},
            {"Progressive Trance",{0,0,7,7,12,12,7,0,0,7,12,12,7,7,5,0}},{"Goa Trance",{0,5,7,0,7,5,0,12,7,5,3,0,3,5,7,12}},
            {"Uplifting Trance",{0,4,7,12,7,4,0,4,7,12,16,12,7,4,0,0}},{"Hard Trance",{0,0,7,0,0,0,7,0,12,12,7,7,0,0,7,0}},
            {"Eurodance",{0,7,0,7,12,7,12,0,0,7,12,7,5,7,5,0}},{"Synthwave",{0,4,7,12,16,12,7,4,0,4,7,12,7,4,0,0}},
            {"Retrowave",{0,7,12,7,0,7,12,16,12,7,0,7,5,0,5,7}},{"Outrun",{0,4,7,12,7,4,0,4,7,12,7,4,0,0,0,0}},
            {"Downtempo",{0,7,12,10,7,5,3,0,3,5,7,10,12,10,7,0}},{"Chillout",{0,5,7,12,14,12,7,5,0,5,7,5,0,0,7,12}},
            {"Trip-Hop",{0,3,5,7,10,7,5,3,0,3,7,10,7,5,3,0}} };
        return p;
    }

    const Vec* findBassPattern(const juce::String& name)
    {
        for (auto& b : bassPatterns()) if (name == b.name) return &b.intervals;
        return nullptr;
    }

    // Ритм-шаблоны (порт CreateRhythmicTemplates)
    const std::vector<Vec>& rhythmicTemplates()
    {
        static const std::vector<Vec> t = {
            {1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0},{1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0},{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
            {1,0,0,1,0,1,0,0,1,0,0,1,0,1,0,0},{1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,1},{1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0},
            {1,0,1,1,0,0,1,0,1,0,0,1,1,0,1,0},{1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0},{1,1,0,1,0,0,1,1,0,1,0,0,1,1,0,1},
            {1,0,0,0,1,0,0,0,1,0,1,0,1,0,0,0},{1,0,1,0,1,0,0,1,1,0,1,0,0,0,1,0},{1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1},
            {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},{0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0},{1,0,0,0,0,1,0,0,1,0,0,0,0,1,0,0},
            {1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},{1,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0},{1,0,1,1,0,0,1,0,0,1,0,1,1,0,0,1},
            {1,0,1,0,0,1,0,1,1,0,0,1,0,1,0,0},{1,1,0,1,1,0,1,0,1,1,0,1,0,1,0,0},{1,0,0,1,0,0,1,0,1,0,0,1,0,0,0,1},
            {1,0,0,0,0,1,0,0,1,0,0,0,0,1,0,0},{1,0,1,0,0,0,1,0,0,1,0,0,1,0,0,1},{1,0,0,1,0,0,1,0,0,1,0,0,1,0,1,0},
            {1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1},{1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1},{1,0,1,0,1,0,1,1,0,1,0,1,0,1,1,0},
            {1,0,1,0,0,1,0,0,1,0,1,0,0,1,0,0},{1,0,0,1,0,1,0,0,1,0,0,1,0,1,0,0},{1,1,0,1,0,0,1,1,0,1,0,0,1,1,0,1},
            {1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,1},{1,0,0,1,1,0,1,0,0,1,1,0,1,0,0,1},{1,1,0,1,0,1,1,0,1,0,1,1,0,1,0,0} };
        return t;
    }

    // Порт SelectRhythmTemplate
    const Vec& selectRhythmTemplate(const juce::String& name, juce::Random& rng)
    {
        struct GR { const char* genre; std::vector<int> idx; };
        static const std::vector<GR> map = {
            {"Jazz",{3,4,5}},{"Bebop",{4,5}},{"Funk",{6,7,8}},{"Soul",{6,32,33}},{"R&B",{32,33,34}},
            {"Reggae",{9,10,11}},{"Dub",{10,11}},{"House",{15,16}},{"Techno",{15,16}},{"Rock",{12,13,14}},
            {"Hard Rock",{13,14}},{"Metal",{35,36,37}},{"Progressive Metal",{36,37}},{"Latin",{18,19,20}},
            {"Salsa",{18}},{"Bossa Nova",{19}},{"Samba",{20}},{"Hip-Hop",{21,22,23}},{"Trap",{22}},
            {"Fusion",{38,39,40}},{"Electronic",{15,16,17}},{"Breakbeat",{17}} };
        auto& tpls = rhythmicTemplates();
        for (auto& gr : map)
            if (name.containsIgnoreCase(gr.genre))
            {
                // Берём только валидные индексы (в наших шаблонах их 33)
                std::vector<int> valid;
                for (int i : gr.idx) if (i < (int)tpls.size()) valid.push_back(i);
                if (!valid.empty()) return tpls[valid[rng.nextInt((int)valid.size())]];
            }
        return tpls[rng.nextInt((int)tpls.size())];
    }

    Vec styleDurations(const juce::String& name)
    {
        struct SD { const char* style; Vec d; };
        static const std::vector<SD> t = {
            {"Jazz",{2,1,2,1,3,1,2,1,2,1,2,1,3,1,1,1}},{"Walking Bass",{4,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0}},
            {"Bebop",{3,1,2,1,2,1,2,1,3,1,2,1,2,1,1,1}},{"Funk",{1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1}},
            {"Slap",{1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0}},{"Soul",{2,1,1,1,2,1,1,1,2,1,1,1,2,1,1,1}},
            {"R&B",{2,1,1,1,2,1,1,1,2,1,1,1,2,1,1,1}},{"Rock",{3,0,0,0,3,0,0,0,3,0,0,0,3,0,0,0}},
            {"Hard Rock",{3,0,2,0,3,0,2,0,3,0,2,0,3,0,2,0}},{"Metal",{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}},
            {"Progressive Metal",{2,2,1,1,2,2,1,1,3,0,2,0,2,1,1,1}},{"Reggae",{1,0,0,0,4,0,0,0,1,0,0,0,4,0,0,0}},
            {"Dub",{6,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0}},{"Blues Shuffle",{3,0,2,0,3,0,2,0,3,0,2,0,3,0,2,0}},
            {"Salsa",{2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1}},{"Bossa Nova",{3,0,2,0,1,1,2,0,3,0,1,1,2,0,1,1}},
            {"Samba",{1,1,2,0,1,1,2,0,1,1,2,0,1,1,2,0}},{"House",{4,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0}},
            {"Techno",{2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0}},{"Drum & Bass",{3,0,0,0,1,0,1,0,3,0,0,0,1,0,1,0}},
            {"Trance",{3,0,1,0,3,0,1,0,3,0,1,0,3,0,1,0}},{"PsyTrance",{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}},
            {"Breakbeat",{2,0,1,1,2,0,1,1,2,0,1,1,2,0,1,1}},{"Progressive Trance",{4,0,2,0,2,0,3,0,4,0,2,0,3,0,1,0}},
            {"Progressive House",{3,0,2,0,3,0,2,0,4,0,0,0,3,1,2,0}},{"Uplifting Trance",{2,0,3,0,2,0,3,0,4,0,0,0,2,0,3,0}},
            {"Goa Trance",{1,1,1,0,1,1,1,0,1,1,1,0,2,0,2,0}},{"Tech Trance",{2,0,2,0,1,0,2,1,2,0,2,0,1,0,3,0}},
            {"Hip-Hop",{4,0,0,0,2,0,2,0,4,0,0,0,2,0,2,0}},{"Trap",{6,0,0,0,0,0,0,0,3,0,0,0,3,0,0,0}},
            {"Lo-Fi",{3,0,0,0,2,0,0,0,3,0,0,0,2,1,0,0}},{"Folk",{4,0,2,0,4,0,2,0,4,0,2,0,4,0,2,0}},
            {"Celtic",{2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1}} };
        for (auto& sd : t) if (name.containsIgnoreCase(sd.style)) return sd.d;
        return { 2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1 }; // Default
    }

    bool contains(const Vec& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

    bool isNoteInScale(int midi, const Vec& scale)
    {
        int nm = ((midi % 12) + 12) % 12;
        for (int n : scale) if (((n % 12) + 12) % 12 == nm) return true;
        return false;
    }

    int findClosestInScale(int target, const Vec& scale)
    {
        if (scale.empty()) return target;
        if (contains(scale, target)) return target;
        int closest = scale[0], minDist = std::abs(target - closest);
        for (int n : scale) { int d = std::abs(target - n); if (d < minDist) { minDist = d; closest = n; } }
        return closest;
    }

    // Декодер артикуляции (порт ExtractBaseNoteValue)
    int extractBase(int v)
    {
        if (v >= 4000) return v - 4000;
        if (v >= 3000) return v - 3000;
        if (v >= 2000) return v - 2000;
        if (v >= 1000) return v - 1000;
        return v;
    }

    int firstInScale(const Vec& scale, int target, int tol) // FirstOrDefault(|n-target|<=tol)
    {
        for (int n : scale) if (std::abs(n - target) <= tol) return n;
        return 0;
    }

    //==========================================================================
    // Жанровые вариации (порт Apply*Variations / Apply*Vars)
    //==========================================================================
    Vec applyFunkSoulRB(Vec result, const Vec& scale, juce::Random& rng)
    {
        for (int i = 0; i < (int)result.size(); i += 4)
            if (result[i] > 0 && rng.nextDouble() < 0.6) result[i] = 1000 + extractBase(result[i]);
        for (int i = 1; i < (int)result.size(); i += 2)
            if (result[i] == -1 && i > 0 && result[i-1] > 0 && rng.nextDouble() < 0.3) result[i] = extractBase(result[i-1]) - 2;
        for (int i = 2; i < (int)result.size(); i += 4)
        {
            if (result[i] > 0) { if (rng.nextDouble() < 0.7) result[i] = 2000 + extractBase(result[i]); }
            else if (result[i] == -1 && rng.nextDouble() < 0.4 && i > 0)
                for (int j = i - 1; j >= 0; --j) if (result[j] > 0) { int b = extractBase(result[j]); result[i] = rng.nextDouble() < 0.6 ? 1000 + b : b; break; }
        }
        for (int i = 0; i < (int)result.size() - 1; ++i)
            if (result[i] > 0 && result[i+1] > 0)
            {
                int c = extractBase(result[i]), n = extractBase(result[i+1]);
                if (std::abs(n - c) > 3 && rng.nextDouble() < 0.5) result[i] = 4000 + c;
            }
        if (rng.nextDouble() < 0.3)
        {
            int root = -1; for (int v : result) if (v > 0) { root = extractBase(v); break; }
            if (root > 0)
            {
                Vec nearby; for (int n : scale) if (std::abs(n - root) <= 5) nearby.push_back(n);
                std::sort(nearby.begin(), nearby.end(), [&](int a, int b){ return std::abs(a-root) < std::abs(b-root); });
                if ((int)nearby.size() >= 3)
                    for (int s = 4; s < (int)result.size() - 4; s += 8)
                        if (rng.nextDouble() < 0.7)
                        {
                            int pat[4] = { root, nearby[1], nearby[2], root };
                            for (int i = 0; i < 4 && s + i < (int)result.size(); ++i) result[s+i] = pat[i];
                        }
            }
        }
        return result;
    }

    Vec applyHipHopTrap(Vec result, const Vec&, juce::Random& rng)
    {
        int root = -1; for (int v : result) if (v > 0) { root = extractBase(v); break; }
        if (root > 0)
        {
            for (int i = 0; i < (int)result.size(); i += 4)
                if (rng.nextDouble() < 0.6)
                {
                    int sustain = 2 + rng.nextInt(2);
                    int sub = root - 12; if (sub < 28) sub = 28;
                    result[i] = 2000 + sub;
                    for (int j = 1; j <= sustain && i + j < (int)result.size(); ++j) result[i+j] = -1;
                }
            if (rng.nextDouble() < 0.4)
            {
                int pos = 4 + rng.nextInt(8);
                if (pos < (int)result.size())
                {
                    if (rng.nextDouble() < 0.5) result[pos] = 4000 + (root - 5);
                    else for (int i = 0; i < 3 && pos + i < (int)result.size(); ++i) result[pos+i] = root - 12;
                }
            }
        }
        for (int i = 0; i < (int)result.size(); ++i)
            if (i % 8 >= 5 && i % 8 <= 6 && result[i] > 0 && rng.nextDouble() < 0.6) result[i] = -1;
        if (rng.nextDouble() < 0.5)
        {
            int drop = (int)result.size() - 4;
            if (drop > 0 && root > 0)
            {
                int dn = root - 12;
                result[drop] = 2000 + dn;
                if (drop + 1 < (int)result.size()) result[drop+1] = 4000 + (dn - 2);
            }
        }
        return result;
    }

    Vec applyRockMetal(Vec result, const Vec& scale, juce::Random& rng)
    {
        Vec uniq; for (int v : result) if (v > 0) { int b = extractBase(v); if (std::find(uniq.begin(),uniq.end(),b)==uniq.end()) uniq.push_back(b); }
        if (uniq.empty()) return result;
        int baseNote = *std::min_element(uniq.begin(), uniq.end());
        for (int i = 0; i < (int)result.size(); i += 2)
            if (result[i] > 0 && rng.nextDouble() < 0.6) result[i] = 3000 + extractBase(result[i]);
        if (rng.nextDouble() < 0.7)
        {
            int ps = rng.nextInt(8), pl = 4 + rng.nextInt(4);
            pl = std::min(pl, (int)result.size() - ps);
            for (int i = 0; i < pl; i += 2) { int p = ps + i; if (p < (int)result.size()) { result[p] = baseNote; if (i % 4 == 0) result[p] = 2000 + baseNote; } }
        }
        if (rng.nextDouble() < 0.4)
        {
            int gs = rng.nextInt(juce::jmax(1,(int)result.size() - 6));
            for (int i = 0; i < 6 && gs + i < (int)result.size(); ++i) result[gs+i] = -1;
            result[gs] = 2000 + baseNote;
            if (gs+2 < (int)result.size()) result[gs+2] = baseNote;
            if (gs+3 < (int)result.size()) result[gs+3] = baseNote;
            if (gs+5 < (int)result.size()) result[gs+4] = 2000 + baseNote;
        }
        for (int i = 0; i < (int)result.size() - 3; i += 4)
            if (result[i] > 0 && rng.nextDouble() < 0.5)
            {
                int cur = extractBase(result[i]);
                int fifth = firstInScale(scale, cur + 7, 1);
                if (fifth > 0)
                {
                    if (i+1 < (int)result.size()) result[i+1] = fifth;
                    if (i+2 < (int)result.size() && rng.nextDouble() < 0.3) result[i+2] = cur - 1;
                }
            }
        for (int i = 0; i < (int)result.size(); i += 4)
            if (result[i] > 0 && result[i] < 1000) result[i] = 2000 + extractBase(result[i]);
        return result;
    }

    Vec applyReggaeDub(Vec result, const Vec& scale, juce::Random& rng)
    {
        for (int i = 0; i < (int)result.size(); ++i)
        {
            if (i % 4 == 0 && result[i] > 0 && rng.nextDouble() < 0.7) result[i] = -1;
            if ((i % 4 == 1 || i % 4 == 3) && result[i] > 0) result[i] = 2000 + extractBase(result[i]);
        }
        for (int i = 0; i < (int)result.size(); i += 2)
            if (i % 4 != 0 && result[i] == -1 && rng.nextDouble() < 0.5)
            {
                int ref = -1; for (int j = std::max(0,i-4); j < std::min((int)result.size(), i+4); ++j) if (result[j] > 0) { ref = extractBase(result[j]); break; }
                if (ref > 0)
                {
                    static const int iv[] = { 3,5,7 };
                    int sk = ref + iv[rng.nextInt(3)];
                    result[i] = isNoteInScale(sk, scale) ? sk : findClosestInScale(sk, scale);
                }
            }
        if (rng.nextDouble() < 0.4)
        {
            int ds = rng.nextInt(juce::jmax(1,(int)result.size() - 8));
            int dr = -1; for (int i = ds; i < ds + 8 && i < (int)result.size(); ++i) if (result[i] > 0) { dr = extractBase(result[i]); break; }
            if (dr > 0)
                for (int i = 0; i < 6; i += 2) { int ep = ds + i; if (ep < (int)result.size()) { result[ep] = dr; if (i > 0) result[ep] = 3000 + dr; } }
        }
        for (int i = 0; i < (int)result.size() - 1; ++i)
            if (result[i] > 0 && result[i+1] > 0)
            {
                int c = extractBase(result[i]), n = extractBase(result[i+1]);
                if (std::abs(n - c) > 2 && rng.nextDouble() < 0.6) result[i] = 4000 + c;
            }
        if (rng.nextDouble() < 0.3)
            for (int i = 0; i < (int)result.size(); i += 4)
                if (result[i] > 0) { int d = extractBase(result[i]) - 12; if (d >= 28) result[i] = d; }
        return result;
    }

    Vec applyJazzBlues(Vec result, const Vec& scale, juce::Random& rng)
    {
        for (int i = 0; i < (int)result.size() - 1; ++i)
            if (result[i] > 0 && result[i+1] > 0)
            {
                int c = extractBase(result[i]), n = extractBase(result[i+1]);
                if (std::abs(n - c) > 1 && i + 2 < (int)result.size() && result[i+2] == -1 && rng.nextDouble() < 0.4)
                    result[i+2] = (n > c) ? n - 1 : n + 1;
            }
        static const int blues[] = { 3,6,10 };
        for (int i = 0; i < (int)result.size(); i += 4)
            if (i+1 < (int)result.size() && result[i] > 0 && result[i+1] == -1 && rng.nextDouble() < 0.3)
                result[i+1] = 4000 + (extractBase(result[i]) + blues[rng.nextInt(3)]);
        for (int i = 0; i < (int)result.size(); ++i)
            if ((i % 16 == 4 || i % 16 == 12) && result[i] > 0 && rng.nextDouble() < 0.7) result[i] = 2000 + extractBase(result[i]);
        for (int i = 3; i < (int)result.size(); i += 4)
            if (result[i] > 0 && i+1 < (int)result.size() && result[i+1] > 0)
            {
                int c = extractBase(result[i]), n = extractBase(result[i+1]);
                if (std::abs(n - c) > 3 && rng.nextDouble() < 0.5)
                {
                    int dir = (n > c) ? 1 : -1;
                    Vec inter; for (int s : scale) if ((dir > 0 && s > c && s < n) || (dir < 0 && s < c && s > n)) inter.push_back(s);
                    std::sort(inter.begin(), inter.end(), [&](int a,int b){ return std::abs(a-c) < std::abs(b-c); });
                    if (!inter.empty() && i+2 < (int)result.size() && result[i+2] == -1) result[i+2] = inter.front();
                }
            }
        for (int i = 0; i < (int)result.size() - 2; i += 4)
            if (result[i] > 0 && result[i+1] == -1 && result[i+2] == -1 && rng.nextDouble() < 0.4)
            {
                int b = extractBase(result[i]);
                Vec nearby; for (int n : scale) if (std::abs(n - b) <= 7 && n != b) nearby.push_back(n);
                std::sort(nearby.begin(), nearby.end(), [&](int a,int c){ return std::abs(a-b) < std::abs(c-b); });
                if ((int)nearby.size() >= 2) { result[i+1] = nearby[0]; result[i+2] = nearby[1]; }
            }
        if (rng.nextDouble() < 0.5)
            for (int i = 0; i < (int)result.size(); i += 8)
                if (i+7 < (int)result.size())
                {
                    Vec seg; for (int j = i; j < i+8; ++j) if (result[j] > 0) seg.push_back(extractBase(result[j]));
                    if ((int)seg.size() >= 2)
                    {
                        int root = seg.front();
                        int fifth = firstInScale(scale, root + 7, 1);
                        if (fifth > 0) { result[i] = root; result[i+2] = fifth; result[i+4] = root; result[i+6] = fifth - 2; }
                    }
                }
        return result;
    }

    Vec applyLatino(Vec result, const Vec& scale, juce::Random& rng)
    {
        int root = -1; for (int v : result) if (v > 0) { root = extractBase(v); break; }
        if (root <= 0) return result;
        if (rng.nextDouble() < 0.6)
        {
            static const int tumbao[] = { 1,0,0,1,0,1,0,0,1,0,0,1,0,0,1,0 };
            int fifth = firstInScale(scale, root + 7, 1);
            if (fifth > 0)
                for (int i = 0; i < (int)result.size() && i < 16; ++i)
                {
                    if (tumbao[i] == 1) { result[i] = (i % 4 == 0) ? root : fifth; if (rng.nextDouble() < 0.4) result[i] = 1000 + result[i]; }
                    else if (result[i] > 0 && rng.nextDouble() < 0.7) result[i] = -1;
                }
        }
        else
        {
            static const int bossa[] = { 1,0,1,0,0,1,0,1,0,0,1,0,1,0,0,1 };
            int fifth = firstInScale(scale, root+7, 1), maj3 = firstInScale(scale, root+4, 1), min7 = firstInScale(scale, root+10, 1);
            Vec notes; if (root>0) notes.push_back(root); if (fifth>0) notes.push_back(fifth); if (maj3>0) notes.push_back(maj3); if (min7>0) notes.push_back(min7);
            if ((int)notes.size() < 3)
            {
                notes.clear(); Vec tmp; for (int n : scale) if (std::abs(n-root) <= 12) tmp.push_back(n);
                std::sort(tmp.begin(), tmp.end(), [&](int a,int b){ return std::abs(a-root) < std::abs(b-root); });
                for (int k = 0; k < 4 && k < (int)tmp.size(); ++k) notes.push_back(tmp[k]);
            }
            if ((int)notes.size() >= 2)
                for (int i = 0; i < (int)result.size() && i < 16; ++i)
                {
                    if (bossa[i] == 1) { result[i] = notes[(i/2) % (int)notes.size()]; if (i % 4 == 0 && rng.nextDouble() < 0.3) result[i] = 2000 + result[i]; }
                    else if (result[i] > 0 && rng.nextDouble() < 0.6) result[i] = -1;
                }
        }
        for (int i = 0; i < (int)result.size() - 1; ++i)
            if (result[i] > 0 && result[i+1] > 0)
            {
                int c = extractBase(result[i]), n = extractBase(result[i+1]);
                if (std::abs(n - c) > 2 && rng.nextDouble() < 0.5) result[i] = 4000 + c;
            }
        if (rng.nextDouble() < 0.4)
        {
            int ms = rng.nextInt(juce::jmax(1,(int)result.size() - 8));
            Vec mn; for (int n : scale) if (std::abs(n-root) <= 12) mn.push_back(n);
            std::sort(mn.begin(), mn.end(), [&](int a,int b){ return std::abs(a-root) < std::abs(b-root); });
            if ((int)mn.size() >= 3)
            {
                int pat[4] = { mn[0], mn[2], mn[0], mn[1] };
                for (int i = 0; i < 8 && ms + i < (int)result.size(); ++i)
                {
                    if (i % 2 == 0) { result[ms+i] = pat[(i/2) % 4]; if (i == 0 || i == 4) result[ms+i] = 2000 + result[ms+i]; }
                    else if (result[ms+i] > 0 && rng.nextDouble() < 0.7) result[ms+i] = -1;
                }
            }
        }
        return result;
    }

    Vec applyElectronic(Vec result, const Vec& scale, juce::Random& rng)
    {
        int root = -1; for (int v : result) if (v > 0) { root = extractBase(v); break; }
        if (root <= 0) return result;
        if (rng.nextDouble() < 0.5)
        {
            for (int i = 0; i < (int)result.size(); i += 4)
            {
                result[i] = root;
                if (rng.nextDouble() < 0.3) result[i] = 2000 + root;
                if (i+2 < (int)result.size() && result[i+2] == -1 && rng.nextDouble() < 0.4) result[i+2] = root + 12;
            }
            int fp = 8 + rng.nextInt(6);
            if (fp < (int)result.size() - 2) { result[fp] = root; result[fp+1] = root + 12; result[fp+2] = root + 7; }
        }
        else
        {
            for (int i = 0; i < (int)result.size(); i += 2)
            {
                if (rng.nextDouble() < 0.7)
                {
                    int tn = (i % 8 == 0 || i % 8 == 4) ? root : (root + 7);
                    if (!isNoteInScale(tn, scale)) tn = findClosestInScale(tn, scale);
                    result[i] = tn;
                    if (i % 8 == 0) result[i] = 2000 + tn;
                }
                else if (result[i] > 0) result[i] = -1;
            }
            if (rng.nextDouble() < 0.5)
            {
                int ss = 4 + rng.nextInt(8);
                if (ss < (int)result.size() - 4)
                {
                    bool asc = rng.nextDouble() < 0.5;
                    for (int i = 0; i < 4; ++i) { int p = ss + i; if (p < (int)result.size()) { int sn = asc ? (root + i*2) : (root + (3-i)*2); if (!isNoteInScale(sn, scale)) sn = findClosestInScale(sn, scale); result[p] = sn; if (i < 3) result[p] = 4000 + sn; } }
                }
            }
        }
        for (int i = 0; i < (int)result.size(); i += 8)
            for (int j = 1; j <= 3; ++j) { int p = i + j; if (p < (int)result.size() && result[p] > 0 && rng.nextDouble() < 0.7) result[p] = -1; }
        if (rng.nextDouble() < 0.4)
        {
            int as2 = rng.nextInt(juce::jmax(1,(int)result.size() - 8));
            Vec arp; for (int n : scale) if (std::abs(n-root) <= 12 && n >= root) arp.push_back(n);
            std::sort(arp.begin(), arp.end());
            if ((int)arp.size() > 4) arp.resize(4);
            if ((int)arp.size() >= 3)
                for (int i = 0; i < 8 && as2 + i < (int)result.size(); ++i)
                {
                    if (i % 2 == 0) { result[as2+i] = arp[(i/2) % (int)arp.size()]; if (i == 0) result[as2+i] = 2000 + result[as2+i]; else if (rng.nextDouble() < 0.3) result[as2+i] = 4000 + result[as2+i]; }
                    else if (result[as2+i] > 0) result[as2+i] = -1;
                }
        }
        if (rng.nextDouble() < 0.5)
        {
            int pst = rng.nextInt(8);
            for (int i = pst; i < (int)result.size(); i += 8)
                if (i+3 < (int)result.size())
                {
                    result[i] = root; result[i+2] = root;
                    if (rng.nextDouble() < 0.4) { int f = firstInScale(scale, root+7, 1); if (f > 0) result[i+2] = f; }
                }
        }
        return result;
    }

    void applyPsyVarsNoPauses(Vec& pattern, const Vec& scale, juce::Random& rng)
    {
        double intensity = 0.3 + rng.nextDouble() * 0.4;
        for (int i = 0; i < (int)pattern.size(); ++i)
        {
            int base = extractBase(pattern[i]);
            if (rng.nextDouble() < intensity)
            {
                double vt = rng.nextDouble();
                if (vt < 0.3 && i % 4 == 3) pattern[i] = 4000 + base;
                else if (vt < 0.6)
                {
                    Vec nb; for (int n : scale) if (std::abs(n - base) <= 5 && n != base) nb.push_back(n);
                    if (!nb.empty())
                    {
                        int nn = nb[rng.nextInt((int)nb.size())];
                        if (pattern[i] >= 4000) pattern[i] = 4000 + nn;
                        else if (pattern[i] >= 3000) pattern[i] = 3000 + nn;
                        else if (pattern[i] >= 2000) pattern[i] = 2000 + nn;
                        else if (pattern[i] >= 1000) pattern[i] = 1000 + nn;
                        else pattern[i] = nn;
                    }
                }
                else if (vt < 0.9) pattern[i] = 1000 + base;
            }
        }
    }

    Vec applyPsyTranceVars(const Vec& bassLine, const Vec& scaleNotes, juce::Random& rng)
    {
        Vec result(16, 0);
        int baseNote = -1; for (int i = 0; i < (int)bassLine.size() && i < 16; ++i) if (bassLine[i] > 0) { baseNote = extractBase(bassLine[i]); break; }
        if (baseNote < 0 && !scaleNotes.empty()) baseNote = *std::min_element(scaleNotes.begin(), scaleNotes.end());
        else if (baseNote < 0) baseNote = 28;

        Vec bass; for (int n : scaleNotes) if (n >= 28 && n <= 55) bass.push_back(n);
        std::sort(bass.begin(), bass.end());
        if (bass.empty())
        {
            Vec baseOct; for (int n : scaleNotes) if (n < 12) baseOct.push_back(n);
            if (!baseOct.empty()) for (int n : baseOct) bass.push_back(28 + n);
            else bass = { baseNote, baseNote+1, baseNote+3, baseNote+5, baseNote+7, baseNote+8, baseNote+10 };
        }

        int root = std::find(bass.begin(),bass.end(),baseNote) != bass.end() ? baseNote
                   : *std::min_element(bass.begin(), bass.end(), [&](int a,int b){ return std::abs(a-baseNote) < std::abs(b-baseNote); });
        int third = 0; { Vec c; for (int n : bass) if (n > root && n <= root+4) c.push_back(n); std::sort(c.begin(),c.end(),[&](int a,int b){return std::abs((a-root)-3)<std::abs((b-root)-3);}); if (!c.empty()) third = c.front(); }
        if (third == 0) third = root + 3;
        int fifth = 0; { Vec c; for (int n : bass) if (n > root && n <= root+8) c.push_back(n); std::sort(c.begin(),c.end(),[&](int a,int b){return std::abs((a-root)-7)<std::abs((b-root)-7);}); if (!c.empty()) fifth = c.front(); }
        if (fifth == 0) fifth = root + 7;

        std::vector<Vec> pats = {
            { root,root,third,root,root,root,fifth,root,root,root,third,root,root,root,fifth,root },
            { root,third,third,root,fifth,fifth,root,third,root,root,fifth,third,third,root,fifth,fifth },
            { root,third,fifth,third,root,third,fifth,third,root,third,fifth,third,root,third,fifth,third },
            { root,fifth,fifth,root,root,fifth,fifth,fifth,root,fifth,fifth,root,root,fifth,root,fifth },
            { root,root+1,root,root-1,root,root+1,root,fifth,root,root+1,root,root-1,root,third,root,fifth },
            { root,root,root,third,third,root,fifth,fifth,root,root,root,third,third,fifth,fifth,third },
            { root,fifth,fifth,third,root,fifth,fifth,fifth,root,fifth,fifth,third,third,root,fifth,fifth },
            { root,root+2,root,root+2,fifth,fifth-2,fifth,third,root,root+2,root,root+2,third,third+2,fifth,root },
            { root,third,root,fifth,root,third,root,fifth,root,third,root,fifth,root,third,root,fifth } };
        Vec sel = pats[rng.nextInt((int)pats.size())];

        for (int i = 0; i < 16; ++i)
        {
            int pn = sel[i];
            if (std::find(bass.begin(),bass.end(),pn) == bass.end())
                pn = *std::min_element(bass.begin(), bass.end(), [&](int a,int b){ return std::abs(a-pn) < std::abs(b-pn); });
            double ac = rng.nextDouble();
            if (i % 4 == 0) result[i] = (ac < 0.8) ? 2000 + pn : pn;
            else if (i % 2 == 0) result[i] = (i < 15 && ac < 0.4) ? 4000 + pn : (ac < 0.7 ? 1000 + pn : pn);
            else result[i] = (ac < 0.6) ? 1000 + pn : pn;
        }
        applyPsyVarsNoPauses(result, bass, rng);
        return result;
    }

    Vec applyProgressiveTranceVars(Vec result, juce::Random& rng)
    {
        for (int i = 0; i < 4; ++i) if (result[i] > 0) { int b = extractBase(result[i]); result[i] = rng.nextDouble() < 0.2 ? 2000 + b : b; }
        for (int i = 4; i < 8; ++i) if (result[i] > 0) { int b = extractBase(result[i]); if (rng.nextDouble() < 0.3) result[i] = 4000 + b; else if (rng.nextDouble() < 0.4) result[i] = 2000 + b; }
        for (int i = 8; i < 12; ++i) if (result[i] > 0) { int b = extractBase(result[i]); if (rng.nextDouble() < 0.5) { if (i < 11 && result[i+1] > 0) result[i] = 4000 + b; else result[i] = 2000 + b; } }
        for (int i = 12; i < 16; ++i) if (result[i] > 0) { int b = extractBase(result[i]); double v = rng.nextDouble(); if (v < 0.3) result[i] = 4000 + b; else if (v < 0.6) result[i] = 2000 + b; else if (v < 0.7 && i < 15 && result[i+1] == -1) result[i+1] = b; }
        if (rng.nextDouble() < 0.7) for (int i = 14; i >= 12; --i) if (result[i] > 0) { result[i] = 4000 + extractBase(result[i]); break; }
        return result;
    }

    Vec applyUpliftingTranceVars(Vec result, juce::Random& rng)
    {
        for (int i = 0; i < (int)result.size(); i += 4) if (result[i] > 0) result[i] = 2000 + extractBase(result[i]);
        for (int i = 8; i < 16; ++i) if (result[i] > 0 && result[i] < 1000) { int b = extractBase(result[i]); if (i >= 12 && i < 15 && result[i+1] > 0) { if (extractBase(result[i+1]) > b) result[i] = 4000 + b; } }
        for (int i = 2; i < (int)result.size(); i += 4) if (result[i] > 0 && rng.nextDouble() < 0.6) result[i] = 1000 + extractBase(result[i]);
        int fin = -1; for (int i = 14; i >= 12; --i) if (result[i] > 0) { fin = i; break; }
        if (fin > 0)
        {
            for (int i = fin - 2; i < fin; ++i) if (i >= 0 && result[i] > 0) result[i] = 2000 + extractBase(result[i]);
            result[fin] = 2000 + extractBase(result[fin]);
        }
        return result;
    }

    Vec applyGoaTranceVars(Vec result, juce::Random& rng)
    {
        for (int i = 0; i < (int)result.size(); ++i)
            if (result[i] > 0)
            {
                int b = extractBase(result[i]);
                if (i < (int)result.size()-1 && result[i+1] > 0 && rng.nextDouble() < 0.3) result[i] = 4000 + b;
                if (i % 4 == 3 && rng.nextDouble() < 0.5) result[i] = 4000 + b;
            }
        if (rng.nextDouble() < 0.7)
        {
            int ss = rng.nextInt(4), ds = 8 + rng.nextInt(4);
            for (int i = 0; i < 4; ++i)
                if (ss+i < (int)result.size() && ds+i < (int)result.size() && result[ss+i] > 0)
                {
                    int b = extractBase(result[ss+i]);
                    result[ds+i] = (rng.nextDouble() < 0.5) ? b + (rng.nextDouble() < 0.5 ? 1 : -1) : b;
                }
        }
        for (int i = 6; i < 10; ++i)
            if (result[i] > 0 && rng.nextDouble() < 0.4)
            {
                int b = extractBase(result[i]);
                if (i+1 < (int)result.size() && result[i+1] == -1) result[i+1] = b + (rng.nextDouble() < 0.5 ? 2 : -2);
            }
        for (int i = 0; i < (int)result.size(); ++i)
            if (result[i] > 0 && rng.nextDouble() < 0.25) result[i] = 2000 + extractBase(result[i]);
        return result;
    }

    Vec applyTechTranceVars(Vec result, juce::Random& rng)
    {
        for (int i = 0; i < (int)result.size(); i += 2) if (result[i] > 0 && rng.nextDouble() < 0.6) result[i] = 2000 + extractBase(result[i]);
        for (int i = 1; i < (int)result.size(); i += 4)
            if (i+2 < (int)result.size() && result[i] == -1 && result[i+1] == -1)
            {
                int ref = -1; for (int j = i-1; j >= 0; --j) if (result[j] > 0) { ref = extractBase(result[j]); break; }
                if (ref > 0 && rng.nextDouble() < 0.4) { result[i] = ref; result[i+1] = -1; }
            }
        if (rng.nextDouble() < 0.6)
        {
            int ps = rng.nextInt(4), pl = 2;
            for (int rep = 1; rep < 3; ++rep) { int ds = ps + rep*pl; for (int i = 0; i < pl; ++i) if (ps+i < (int)result.size() && ds+i < (int)result.size() && result[ps+i] > 0) result[ds+i] = result[ps+i]; }
        }
        for (int i = 7; i < 15; i += 8)
            if (result[i] > 0)
            {
                int b = extractBase(result[i]);
                result[i] = 3000 + b;
                if (i+1 < (int)result.size() && result[i+1] == -1 && rng.nextDouble() < 0.5) result[i+1] = b + (rng.nextDouble() < 0.5 ? 5 : -5);
            }
        return result;
    }

    Vec applyProgressiveHouseVars(Vec result, juce::Random& rng)
    {
        for (int i = 0; i < (int)result.size() - 1; ++i)
            if (result[i] > 0 && result[i+1] > 0) { int c = extractBase(result[i]), n = extractBase(result[i+1]); if (std::abs(n-c) > 3 && rng.nextDouble() < 0.6) result[i] = 4000 + c; }
        for (int i = 0; i < (int)result.size(); i += 4)
            if (result[i] > 0 && rng.nextDouble() < 0.7) result[i] = 2000 + extractBase(result[i]);
        for (int i = 2; i < (int)result.size(); i += 4)
            if (result[i] == -1 && rng.nextDouble() < 0.4)
            {
                int ref = -1; for (int j = i-1; j >= 0; --j) if (result[j] > 0) { ref = extractBase(result[j]); break; }
                if (ref > 0) result[i] = ref;
            }
        if (rng.nextDouble() < 0.6)
            for (int i = 8; i < 16; ++i)
                if (result[i] > 0) { int b = extractBase(result[i]); double p = 0.1 + ((i-8)/8.0)*0.5; if (rng.nextDouble() < p) result[i] = 2000 + b; }
        return result;
    }

    Vec applyGenericTranceVars(Vec result, juce::Random& rng)
    {
        for (int i = 0; i < (int)result.size(); i += 4) if (result[i] > 0 && rng.nextDouble() < 0.7) result[i] = 2000 + extractBase(result[i]);
        for (int i = 0; i < (int)result.size() - 2; i += 4)
            if (result[i] > 0 && result[i+1] == -1 && result[i+2] == -1 && rng.nextDouble() < 0.4) { int b = extractBase(result[i]); result[i+1] = b + 5; result[i+2] = b + 7; }
        for (int i = 3; i < (int)result.size(); i += 4)
            if (result[i] > 0 && i+1 < (int)result.size() && result[i+1] > 0 && rng.nextDouble() < 0.5) result[i] = 4000 + extractBase(result[i]);
        for (int i = 1; i < (int)result.size(); i += 2)
            if (result[i] == -1 && i > 0 && result[i-1] > 0 && rng.nextDouble() < 0.3) result[i] = extractBase(result[i-1]);
        return result;
    }

    Vec applyStyleArticulations(Vec result, const juce::String& style, juce::Random& rng)
    {
        // Порядок ключей как в оригинале
        enum Art { Funk, Slap, Jazz, Walking, Rock, Metal, Reggae, Default };
        Art a = Default;
        if (style.containsIgnoreCase("Funk")) a = Funk;
        else if (style.containsIgnoreCase("Slap")) a = Slap;
        else if (style.containsIgnoreCase("Jazz")) a = Jazz;
        else if (style.containsIgnoreCase("Walking Bass")) a = Walking;
        else if (style.containsIgnoreCase("Rock")) a = Rock;
        else if (style.containsIgnoreCase("Metal")) a = Metal;
        else if (style.containsIgnoreCase("Reggae")) a = Reggae;

        for (int i = 0; i < (int)result.size(); ++i)
            if (result[i] > 0 && result[i] < 1000)
            {
                int note = result[i]; bool strong = (i % 4 == 0);
                switch (a)
                {
                    case Funk:    result[i] = strong ? (rng.nextDouble() < 0.7 ? 1000+note : note) : (rng.nextDouble() < 0.3 ? 1000+note : note); break;
                    case Slap:    result[i] = strong ? (rng.nextDouble() < 0.8 ? 1000+note : note) : (rng.nextDouble() < 0.4 ? 1000+note : note); break;
                    case Jazz:    result[i] = (i % 4 == 3) ? (rng.nextDouble() < 0.4 ? 2000+note : note) : note; break;
                    case Walking: result[i] = (i % 4 == 3) ? (rng.nextDouble() < 0.3 ? 2000+note : note) : note; break;
                    case Rock:    result[i] = strong ? (rng.nextDouble() < 0.2 ? 3000+note : note) : note; break;
                    case Metal:   result[i] = strong ? (rng.nextDouble() < 0.5 ? 2000+note : note) : (rng.nextDouble() < 0.3 ? 3000+note : note); break;
                    case Reggae:  result[i] = !strong ? (rng.nextDouble() < 0.4 ? 3000+note : note) : note; break;
                    default: break;
                }
            }
        return result;
    }

    // Диспетчер (порт AddEnhancedBassVariations)
    Vec addEnhancedBassVariations(Vec bassLine, const Vec& scale, const juce::String& styleHint, juce::Random& rng)
    {
        // Коррекция нот по гамме с сохранением артикуляции
        for (auto& v : bassLine)
        {
            if (v <= 0) continue;
            int base = extractBase(v);
            if (!isNoteInScale(base, scale))
            {
                int c = findClosestInScale(base, scale);
                if (v >= 4000) v = 4000 + c; else if (v >= 3000) v = 3000 + c;
                else if (v >= 2000) v = 2000 + c; else if (v >= 1000) v = 1000 + c; else v = c;
            }
        }

        Vec result;
        if (styleHint.containsIgnoreCase("PsyTrance")) result = applyPsyTranceVars(bassLine, scale, rng);
        else if (styleHint.containsIgnoreCase("Progressive Trance")) result = applyProgressiveTranceVars(bassLine, rng);
        else if (styleHint.containsIgnoreCase("Uplifting Trance")) result = applyUpliftingTranceVars(bassLine, rng);
        else if (styleHint.containsIgnoreCase("Goa Trance")) result = applyGoaTranceVars(bassLine, rng);
        else if (styleHint.containsIgnoreCase("Tech Trance")) result = applyTechTranceVars(bassLine, rng);
        else if (styleHint.containsIgnoreCase("Progressive House")) result = applyProgressiveHouseVars(bassLine, rng);
        else if (styleHint.containsIgnoreCase("Trance")) result = applyGenericTranceVars(bassLine, rng);
        else if (styleHint.containsIgnoreCase("Funk") || styleHint.containsIgnoreCase("Soul") || styleHint.containsIgnoreCase("R&B")) result = applyFunkSoulRB(bassLine, scale, rng);
        else if (styleHint.containsIgnoreCase("Hip-Hop") || styleHint.containsIgnoreCase("Trap")) result = applyHipHopTrap(bassLine, scale, rng);
        else if (styleHint.containsIgnoreCase("Rock") || styleHint.containsIgnoreCase("Metal")) result = applyRockMetal(bassLine, scale, rng);
        else if (styleHint.containsIgnoreCase("Reggae") || styleHint.containsIgnoreCase("Dub")) result = applyReggaeDub(bassLine, scale, rng);
        else if (styleHint.containsIgnoreCase("Jazz") || styleHint.containsIgnoreCase("Blues") || styleHint.containsIgnoreCase("Bebop")) result = applyJazzBlues(bassLine, scale, rng);
        else if (styleHint.containsIgnoreCase("Latin") || styleHint.containsIgnoreCase("Salsa") || styleHint.containsIgnoreCase("Bossa Nova") || styleHint.containsIgnoreCase("Samba")) result = applyLatino(bassLine, scale, rng);
        else if (styleHint.containsIgnoreCase("House") || styleHint.containsIgnoreCase("Techno") || styleHint.containsIgnoreCase("Electro")) result = applyElectronic(bassLine, scale, rng);
        else result = bassLine;

        result = applyStyleArticulations(result, styleHint, rng);
        return result;
    }

    Vec getScaleNotes(const juce::String& tonality, const juce::Array<int>& steps)
    {
        Vec absI; absI.push_back(0);
        for (int s : steps) absI.push_back(absI.back() + s);
        juce::String base = tonality.endsWith("m") ? tonality.dropLastCharacters(1) : tonality;
        int off = nameToOffset(base);
        Vec notes;
        for (int oct = 1; oct <= 3; ++oct)
        {
            int baseMidi = 12 * oct + off;
            for (int interval : absI) { int n = baseMidi + interval; if (n >= 24 && n <= 59) notes.push_back(n); }
        }
        std::sort(notes.begin(), notes.end());
        return notes;
    }

    // Прогрессия аккордов (порт; count = уникальные такты)
    bool allNotesInScale(const Vec& iv, int rootMidi, const Vec& scale)
    {
        for (int interval : iv) { int nm = (rootMidi + interval) % 12; bool f = false; for (int n : scale) if (n % 12 == nm) { f = true; break; } if (!f) return false; }
        return true;
    }
    std::vector<juce::String> getCompatibleChordTypes(const juce::String& scaleName, bool isMinor, const Vec& scale)
    {
        std::vector<juce::String> result = { "Major","Minor","Major7","Minor7","Dominant7" };
        std::vector<juce::String> filtered;
        int rootMidi = isMinor ? 9 : 0;
        for (auto& c : chordTypes()) if (allNotesInScale(c.intervals, rootMidi, scale)) filtered.push_back(c.name);
        if (!filtered.empty()) result = filtered;
        auto addIf = [&](std::vector<juce::String> add)
        {
            for (auto& type : add) { auto* iv = chordIntervals(type); if (iv && allNotesInScale(*iv, rootMidi, scale) && std::find(result.begin(), result.end(), type) == result.end()) result.push_back(type); }
        };
        if (scaleName.contains("Major") || scaleName.contains("Ionian")) addIf({ "Major9","Major13","6","Add9","Quartal" });
        else if (scaleName.contains("Minor") || scaleName.contains("Aeolian")) addIf({ "Minor9","Minor11","m6","Half-Diminished7","Diminished" });
        else if (scaleName.contains("Dorian")) addIf({ "Minor9","Minor13","m6","Dominant9","So What" });
        return result;
    }
    using Chord = std::pair<juce::String, juce::String>;
    std::vector<Chord> generateChordProgression(const juce::String& tonica, const juce::String& scaleName, int count, const Vec& scale, juce::Random& rng)
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
                Vec good = isMinor ? Vec{ 3,4,5,7,8,9 } : Vec{ 5,7,9,2,4,10 };
                int step = good[rng.nextInt((int)good.size())];
                int newOff = (nameToOffset(prog[i-1].first) + step) % 12;
                juce::String type = compatible[rng.nextInt((int)compatible.size())];
                prog.push_back({ offsetToName(newOff), type });
            }
            prog.push_back({ root, isMinor ? "Minor" : "Major" });
        }
        return prog;
    }

    // GenerateRhythmicBassLine (ядро; без жанровой орнаментации)
    Vec generateRhythmicBassLine(const juce::String& rootNote, const juce::String& chordType,
                                 const juce::String& patternName, const Vec& scale, int uniqueNotesCount,
                                 juce::Random& rng)
    {
        Vec measure(16, -1);
        int rootOff = nameToOffset(rootNote);
        int baseNote = 12 + rootOff;
        if (!isNoteInScale(baseNote, scale)) baseNote = findClosestInScale(baseNote, scale);

        auto* patternIntervals = findBassPattern(patternName);
        static const Vec deflt = { 0,4,7 };
        const Vec& chordNotes = chordIntervals(chordType) ? *chordIntervals(chordType) : deflt;
        const Vec& pat = patternIntervals ? *patternIntervals : deflt;

        measure[0] = baseNote;

        // GeneratePossibleNotes
        Vec possible;
        for (int interval : chordNotes) { int n = baseNote + interval; if (isNoteInScale(n, scale) && n >= 28 && n <= 55 && !contains(possible, n)) possible.push_back(n); }
        for (int interval : pat) { int n = baseNote + interval; if (isNoteInScale(n, scale) && n >= 28 && n <= 55 && !contains(possible, n)) possible.push_back(n); }
        while ((int)possible.size() < uniqueNotesCount)
        {
            Vec add; for (int n : scale) if (n >= 28 && n <= 55 && !contains(possible, n)) add.push_back(n);
            if (add.empty()) break;
            possible.push_back(add[rng.nextInt((int)add.size())]);
        }

        const Vec& rhythm = selectRhythmTemplate(patternName, rng);

        Vec scaleCompat;
        for (int n : possible) if (isNoteInScale(n, scale) && n >= 28 && n <= 55) scaleCompat.push_back(n);
        if (scaleCompat.empty()) for (int n : scale) if (n >= 28 && n <= 55) scaleCompat.push_back(n);

        auto pickNote = [&]() -> int
        {
            Vec chordBased;
            for (int n : scaleCompat) { bool m = false; for (int cn : chordNotes) if ((baseNote + cn) % 12 == n % 12) { m = true; break; } if (m && n != baseNote) chordBased.push_back(n); }
            if (!chordBased.empty()) return chordBased[rng.nextInt((int)chordBased.size())];
            Vec avail; for (int n : scaleCompat) if (n != baseNote) avail.push_back(n);
            if (!avail.empty()) return avail[rng.nextInt((int)avail.size())];
            return baseNote;
        };

        int noteCount = 1;
        for (int i = 1; i < 16; ++i)
            if (rhythm[i] == 1) { measure[i] = pickNote(); noteCount++; }

        if (noteCount < uniqueNotesCount)
            for (int i = 1; i < 16 && noteCount < uniqueNotesCount; ++i)
                if (measure[i] == -1) { measure[i] = pickNote(); noteCount++; }

        // Применяем жанровые вариации (как в оригинале — в конце GenerateRhythmicBassLine)
        return addEnhancedBassVariations(measure, scale, patternName, rng);
    }
}

const juce::StringArray& mg::bassPatternNames()
{
    static juce::StringArray names = []
    {
        juce::StringArray a;
        for (auto& b : bassPatterns()) a.add(b.name);
        return a;
    }();
    return names;
}

Clip mg::generateBass(const juce::String& tonality, const juce::String& scaleName,
                      const juce::Array<int>& steps, const juce::String& bassPatternName,
                      int bassNotesCount, int measuresUnique, int repeats,
                      juce::Random& rng, juce::StringArray& chordInfoOut)
{
    Clip clip;
    Vec scale = getScaleNotes(tonality, steps);
    if (scale.empty()) return clip;

    auto prog = generateChordProgression(tonality, scaleName, measuresUnique, scale, rng);

    std::vector<Vec> unique;
    for (int i = 0; i < measuresUnique; ++i)
    {
        auto chord = prog[i % (int)prog.size()];
        Vec m = generateRhythmicBassLine(chord.first, chord.second, bassPatternName, scale, bassNotesCount, rng);
        // Второй проход вариаций (как в GenerateUniqueBassLines оригинала)
        m = addEnhancedBassVariations(m, scale, bassPatternName, rng);
        if ((int)m.size() != 16) m.resize(16, -1);
        unique.push_back(m);
    }

    // Повторы + склейка (total = unique + repeats)
    std::vector<Vec> repeated;
    if (!unique.empty())
        for (int i = 0; i < repeats; ++i)
            repeated.push_back(unique[rng.nextInt((int)unique.size())]);
    std::vector<Vec> melody = unique;
    for (auto& r : repeated)
        melody.insert(melody.begin() + rng.nextInt((int)melody.size() + 1), r);

    for (int i = 0; i < (int)prog.size(); ++i)
        chordInfoOut.add(juce::String::fromUTF8("Chord ") + juce::String(i + 1) + ": " + prog[i].first + chordSuffix(prog[i].second));

    Vec dur = styleDurations(bassPatternName);
    int globalPos = 0;
    for (auto& measure : melody)
    {
        for (int i = 0; i < 16 && i < (int)measure.size(); ++i)
        {
            int base = extractBase(measure[i]);
            if (base > 0)
            {
                int len = juce::jmax(1, dur[i]);
                clip.add({ base, globalPos + i, len });
            }
        }
        globalPos += 16;
    }
    return clip;
}
