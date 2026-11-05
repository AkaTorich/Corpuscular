/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "MusicData.h"
#include "BinaryData.h"
#include <map>

using namespace mg;

//==============================================================================
// Формулы аккордов: тип -> интервалы в полутонах (порт chordFormulas)
//==============================================================================
namespace
{
    struct ChordFormula { const char* type; std::vector<int> intervals; };

    const std::vector<ChordFormula>& chordFormulas()
    {
        static const std::vector<ChordFormula> f = {
            { "",     { 0, 4, 7 } },
            { "maj",  { 0, 4, 7 } },
            { "m",    { 0, 3, 7 } },
            { "dim",  { 0, 3, 6 } },
            { "aug",  { 0, 4, 8 } },
            { "sus4", { 0, 5, 7 } },
            { "add9", { 0, 4, 7, 14 } },
            { "7",    { 0, 4, 7, 10 } },
            { "maj7", { 0, 4, 7, 11 } },
            { "m7",   { 0, 3, 7, 10 } },
            { "dim7", { 0, 3, 6, 9 } },
            { "m7b5", { 0, 3, 6, 10 } },
            { "m9",   { 0, 3, 7, 10, 14 } },
            { "maj9", { 0, 4, 7, 11, 14 } },
            { "9",    { 0, 4, 7, 10, 14 } },
            { "aug7", { 0, 4, 8, 10 } },
            { "5",    { 0, 7 } },
        };
        return f;
    }

    const std::vector<int>* findFormula(const juce::String& type)
    {
        for (auto& f : chordFormulas())
            if (type == f.type)
                return &f.intervals;
        return nullptr;
    }

    int midiNumberForRoot(const juce::String& note)
    {
        static const std::map<juce::String, int> m = {
            {"C",60},{"C#",61},{"Db",61},{"D",62},{"D#",63},{"Eb",63},
            {"E",64},{"F",65},{"F#",66},{"Gb",66},{"G",67},{"G#",68},
            {"Ab",68},{"A",69},{"A#",70},{"Bb",70},{"B",71}
        };
        auto it = m.find(note);
        return it != m.end() ? it->second : -1;
    }

    // Транспонировать ноту на полутоны (порт Transpose)
    juce::String transpose(const juce::String& note, int semitones)
    {
        static const char* sharp[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        static const char* flat[]  = {"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};

        int index = -1; bool isSharp = true;
        for (int i = 0; i < 12; ++i) if (note == sharp[i]) { index = i; break; }
        if (index < 0) { isSharp = false; for (int i = 0; i < 12; ++i) if (note == flat[i]) { index = i; break; } }
        if (index < 0) return "C";

        int n = ((index + semitones) % 12 + 12) % 12;
        return isSharp ? sharp[n] : flat[n];
    }

    juce::String romanNumeral(int number, bool isMinor)
    {
        static const char* maj[] = {"I","II","III","IV","V","VI","VII"};
        static const char* min[] = {"i","ii","iii","iv","v","vi","vii"};
        if (number >= 1 && number <= 7)
            return isMinor ? min[number - 1] : maj[number - 1];
        return "I";
    }

    // Качества и ступени аккордов из интервалов гаммы (порт GenerateChordsFromIntervals)
    void deriveChords(const juce::Array<int>& steps,
                      juce::StringArray& degrees, juce::StringArray& qualities)
    {
        std::vector<int> absI; absI.push_back(0);
        for (int s : steps) absI.push_back(absI.back() + s);

        int noteCount = (int)absI.size() - 1;
        for (int i = 0; i < noteCount; ++i)
            degrees.add(romanNumeral(i + 1, absI[i] % 12 >= 6));

        for (int i = 0; i < noteCount; ++i)
        {
            int root = absI[i] % 12;
            int thirdIndex   = (i + 2) % noteCount;
            int fifthIndex   = (i + 4) % noteCount;
            int seventhIndex = (i + 6) % noteCount;

            int third   = absI[thirdIndex]   % 12 + (thirdIndex   < i ? 12 : 0);
            int fifth   = absI[fifthIndex]   % 12 + (fifthIndex   < i ? 12 : 0);
            int seventh = absI[seventhIndex] % 12 + (seventhIndex < i ? 12 : 0);

            int t3 = ((third   - root) % 12 + 12) % 12;
            int t5 = ((fifth   - root) % 12 + 12) % 12;
            int t7 = ((seventh - root) % 12 + 12) % 12;

            juce::String q;
            auto hasSeventh = (noteCount > 5 && seventhIndex < noteCount);

            if (t3 == 4 && t5 == 7)
                q = hasSeventh ? (t7 == 11 ? "maj7" : t7 == 10 ? "7" : (t7 == 14 || t7 == 2) ? "add9" : "maj") : "maj";
            else if (t3 == 3 && t5 == 7)
                q = hasSeventh ? (t7 == 10 ? "m7" : (t7 == 14 || t7 == 2) ? "m9" : "m") : "m";
            else if (t3 == 3 && t5 == 6)
                q = hasSeventh ? (t7 == 9 ? "dim7" : t7 == 10 ? "m7b5" : "dim") : "dim";
            else if (t3 == 4 && t5 == 8)
                q = hasSeventh ? (t7 == 10 ? "aug7" : "aug") : "aug";
            else if (t3 == 5 && t5 == 7)
                q = "sus4";
            else
                q = "m";

            qualities.add(q);
        }
    }
}

//==============================================================================
// MusicData
//==============================================================================
MusicData::MusicData()
{
    auto jsonText = juce::String::fromUTF8(BinaryData::scales_json, BinaryData::scales_jsonSize);
    auto parsed = juce::JSON::parse(jsonText);

    if (auto* arr = parsed.getArray())
    {
        for (auto& catVar : *arr)
        {
            Category cat;
            cat.name = catVar.getProperty("category", {}).toString();
            if (auto* scalesArr = catVar.getProperty("scales", {}).getArray())
            {
                for (auto& sVar : *scalesArr)
                {
                    Scale sc;
                    sc.name = sVar.getProperty("name", {}).toString();
                    if (auto* stepsArr = sVar.getProperty("steps", {}).getArray())
                        for (auto& st : *stepsArr)
                            sc.steps.add((int)st);
                    if (sc.steps.size() > 0)
                        cat.scales.add(sc);
                }
            }
            std::sort(cat.scales.begin(), cat.scales.end(),
                      [](const Scale& a, const Scale& b) { return a.name < b.name; });
            if (cat.scales.size() > 0)
                cats.add(cat);
        }
    }
    std::sort(cats.begin(), cats.end(),
              [](const Category& a, const Category& b) { return a.name < b.name; });
}

const juce::StringArray& MusicData::tonalities()
{
    static const juce::StringArray t {
        "C","C#","D","D#","E","F","F#","G","G#","A","A#","B",
        "Cm","C#m","Dm","D#m","Em","Fm","F#m","Gm","G#m","Am","A#m","Bm" };
    return t;
}

const juce::StringArray& MusicData::chromatic()
{
    static const juce::StringArray c {
        "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    return c;
}

juce::StringArray MusicData::scaleNoteNames(int tonicPitchClass, const juce::Array<int>& steps)
{
    const auto& chrom = chromatic();
    juce::StringArray result;
    int index = ((tonicPitchClass % 12) + 12) % 12;
    result.add(chrom[index]);
    for (int step : steps)
    {
        index = (index + step) % 12;
        result.add(chrom[index]);
    }
    return result;
}

juce::StringArray MusicData::chordsForScale(const juce::String& tonality, const Scale& scale)
{
    juce::StringArray degrees, qualities, result;
    deriveChords(scale.steps, degrees, qualities);

    // The root of each chord is the scale's own note on that degree (Corpuscular: the roman numerals
    // counted major-scale steps, so a minor scale got C#, F#, G# in A minor)
    juce::String root = tonality.endsWith("m") ? tonality.dropLastCharacters(1) : tonality;
    int offset = 0;
    for (int i = 0; i < qualities.size(); ++i)
    {
        juce::String name = transpose(root, offset);
        if (qualities[i] != "maj")
            name += qualities[i];
        result.add(name);
        if (i < scale.steps.size())
            offset += scale.steps[i];
    }
    return result;
}

juce::Array<int> MusicData::midiNotesForChord(const juce::String& chord)
{
    juce::Array<int> notes;
    if (chord.isEmpty()) return notes;

    juce::String rootNote, chordType;
    auto first = chord[0];
    if (juce::CharacterFunctions::isLetter(first))
    {
        rootNote += first;
        if (chord.length() > 1 && (chord[1] == '#' || chord[1] == 'b'))
        {
            rootNote += chord[1];
            if (chord.length() > 2) chordType = chord.substring(2);
        }
        else if (chord.length() > 1)
            chordType = chord.substring(1);
    }
    else return notes;

    int rootMidi = midiNumberForRoot(rootNote);
    if (rootMidi < 0) return notes;

    auto* intervals = findFormula(chordType);
    if (intervals == nullptr) return notes;

    const int lower = 60, upper = lower + 24;
    for (int interval : *intervals)
    {
        int n = rootMidi + interval;
        while (n < lower)  n += 12;
        while (n >= upper) n -= 12;
        notes.add(n);
    }
    return notes;
}
