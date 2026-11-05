/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Harmony.h"
#include <algorithm>

using namespace mg;

namespace
{
    // Diminished chords sound unsettled in a loop: they go in only when the scale has too few others
    bool isTense(const juce::String& chord)
    {
        return chord.contains("dim") || chord.contains("m7b5") || chord.contains("aug");
    }

    int pitchClass(int midi) { return ((midi % 12) + 12) % 12; }

    juce::Array<int> chordPitchClasses(const juce::String& chord)
    {
        juce::Array<int> classes;
        for (int n : MusicData::midiNotesForChord(chord))
            classes.addIfNotAlreadyThere(pitchClass(n));
        return classes;
    }

    int chordRootClass(const juce::String& chord)
    {
        auto notes = MusicData::midiNotesForChord(chord);
        return notes.isEmpty() ? 0 : pitchClass(notes[0]);
    }

    // A semitone away from a chord note: a quick note there rubs against the chord
    bool rubs(int pc, const juce::Array<int>& chord)
    {
        for (int c : chord)
        {
            int d = pitchClass(pc - c);
            if (d == 1 || d == 11)
                return true;
        }
        return false;
    }

    int sign(int value) { return (value > 0) - (value < 0); }

    // The nearest note with one of the pitch classes, keeping the way the melody moved
    int nearestOf(int midi, const juce::Array<int>& classes, int previousOut, int direction)
    {
        int best = midi, bestScore = 1 << 20;
        for (int n = midi - 7; n <= midi + 7; ++n)
        {
            if (!classes.contains(pitchClass(n)))
                continue;
            int score = 2 * std::abs(n - midi) + (n < midi ? 0 : 1);
            if (previousOut >= 0 && direction != 0 && sign(n - previousOut) != direction)
                score += 5;
            if (score < bestScore)
            {
                bestScore = score;
                best = n;
            }
        }
        return best;
    }
}

namespace
{
    // Scale degrees, from 0: tonic 0 (rest), going away 1, 2, 3, 5, dominant 4 and 6 (pull back home).
    // A phrase leaves the tonic, gathers tension and stops on a dominant; the next chord (the loop, or the
    // next phrase) brings it home.
    juce::Array<int> phraseDegrees(int length, int start, const juce::StringArray& chords, juce::Random& rng)
    {
        auto usable = [&chords](std::initializer_list<int> degrees) {
            juce::Array<int> calm, all;
            for (int d : degrees)
            {
                all.add(d);
                if (!isTense(chords[d]))
                    calm.add(d);
            }
            return calm.isEmpty() ? all : calm;
        };
        auto pick = [&rng](const juce::Array<int>& from, const juce::Array<int>& avoid, const juce::Array<int>& used) {
            juce::Array<int> fresh, allowed;
            for (int d : from)
            {
                if (avoid.contains(d))
                    continue;
                allowed.add(d);
                if (!used.contains(d))
                    fresh.add(d);
            }
            const auto& list = fresh.isEmpty() ? allowed : fresh;
            return list.isEmpty() ? from[0] : list[rng.nextInt(list.size())];
        };

        juce::Array<int> degrees;
        degrees.add(start);
        if (length < 2)
            return degrees;

        juce::Array<int> dominant = usable({ 4, 6 });
        juce::Array<int> before = usable({ 3, 1, 5 });
        juce::Array<int> away = usable({ 5, 2, 3, 1 });

        for (int i = 1; i < length - 2; ++i)
            degrees.add(pick(away, juce::Array<int>(degrees.getLast()), degrees));
        if (length >= 3)
            degrees.add(pick(before, juce::Array<int>(degrees.getLast()), degrees));
        degrees.add(pick(dominant, juce::Array<int>(degrees.getLast()), degrees));
        return degrees;
    }
}

juce::StringArray mg::harmonyProgression(const juce::String& tonality, const Scale& scale, int count, juce::Random& rng)
{
    juce::StringArray progression;
    juce::StringArray all;
    for (auto& chord : MusicData::chordsForScale(tonality, scale))
        if (MusicData::midiNotesForChord(chord).size() > 0)
            all.addIfNotAlreadyThere(chord);
    if (all.isEmpty() || count < 1)
        return progression;

    // Seven-note scales get tension and release: one phrase, or from 6 chords a question and an answer
    if (all.size() == 7)
    {
        int first = count >= 6 ? count / 2 : count;
        for (int d : phraseDegrees(first, 0, all, rng))
            progression.add(all[d]);
        if (count > first)
        {
            int start = rng.nextBool() ? 0 : 5;
            for (int d : phraseDegrees(count - first, start, all, rng))
                progression.add(all[d]);
        }
        return progression;
    }

    // Other scales (pentatonics and the like): different chords, the tonic first

    juce::String tonic = all[0];
    juce::StringArray calm, tense;
    for (int i = 1; i < all.size(); ++i)
        (isTense(all[i]) ? tense : calm).add(all[i]);

    auto shuffle = [&rng](juce::StringArray& list) {
        for (int i = list.size(); --i > 0;)
        {
            int j = rng.nextInt(i + 1);
            auto tmp = list[i];
            list.set(i, list[j]);
            list.set(j, tmp);
        }
    };
    shuffle(calm);
    shuffle(tense);
    juce::StringArray pool(calm);
    pool.addArray(tense);

    // Different chords first, as many as asked and as the scale has
    progression.add(tonic);
    for (int i = 0; i < pool.size() && progression.size() < count; ++i)
        progression.add(pool[i]);

    // More chords than the scale has: the chosen ones come back, never twice in a row
    juce::StringArray chosen(progression);
    int guard = 0;
    while (progression.size() < count && guard++ < 10000)
    {
        auto next = chosen[rng.nextInt(chosen.size())];
        bool last = progression.size() == count - 1;
        if (next == progression[progression.size() - 1] || (last && chosen.size() > 2 && next == tonic))
            continue;
        progression.add(next);
    }
    while (progression.size() < count)
        progression.add(tonic);
    return progression;
}

int mg::harmonyChordStart(int index, int count, int total)
{
    if (count <= 0)
        return 0;
    // On an eighth note, so a chord never changes between two quick notes of a pair
    return juce::roundToInt(index * total / (2.0 * count)) * 2;
}

Clip mg::fitToChords(const Clip& melody, const juce::StringArray& chords, int total, bool bass)
{
    if (chords.isEmpty() || melody.isEmpty() || total <= 0)
        return melody;

    juce::Array<juce::Array<int>> classes;
    juce::Array<int> roots;
    for (auto& chord : chords)
    {
        classes.add(chordPitchClasses(chord));
        roots.add(chordRootClass(chord));
    }

    Clip notes(melody);
    std::sort(notes.begin(), notes.end(), [](const NoteEvent& a, const NoteEvent& b) {
        return a.startSixteenth != b.startSixteenth ? a.startSixteenth < b.startSixteenth : a.midi < b.midi;
    });

    // Release: the melody starts on the tonic itself. Tension: its last note, over the last chord, sits a step
    // from the tonic (leading note or second), so the loop pulls it home
    const int tonic = roots[0];
    juce::Array<int> homeward;
    for (int pc : classes.getReference(chords.size() - 1))
    {
        int d = pitchClass(pc - tonic);
        if (d == 11 || d == 1 || d == 2)
            homeward.add(pc);
    }

    const int count = chords.size();
    int previousIn = -1, previousOut = -1, previousChord = -1;
    for (int i = 0; i < notes.size(); ++i)
    {
        auto& note = notes.getReference(i);
        const bool firstNote = i == 0;
        const bool lastNote = i == notes.size() - 1;
        int chord = 0;
        while (chord + 1 < count && note.startSixteenth % total >= harmonyChordStart(chord + 1, count, total))
            ++chord;
        const auto& tones = classes.getReference(chord);
        if (tones.isEmpty())
            continue;

        const int pc = pitchClass(note.midi);
        const int direction = previousIn < 0 ? 0 : sign(note.midi - previousIn);
        const bool firstOfChord = chord != previousChord;
        const bool strong = note.startSixteenth % 2 == 0 || note.lengthSixteenth >= 2;
        previousIn = note.midi;
        previousChord = chord;

        int out = note.midi;
        if (firstNote && chord == 0) {
            juce::Array<int> home;
            home.add(tonic);
            out = nearestOf(note.midi, home, -1, 0);
        }
        else if (lastNote && chord == count - 1 && !homeward.isEmpty() && !(bass && firstOfChord))
            out = nearestOf(note.midi, homeward, previousOut, direction);
        else if (bass && firstOfChord) {
            juce::Array<int> root;
            root.add(roots[chord]);
            out = nearestOf(note.midi, root, -1, 0);
        }
        else if (tones.contains(pc))
            out = note.midi;
        else if (strong || rubs(pc, tones))
            out = nearestOf(note.midi, tones, previousOut, direction);

        note.midi = juce::jlimit(0, 127, out);
        previousOut = note.midi;
    }

    // Two notes that met on one key at one time become one
    Clip result;
    for (auto& note : notes)
    {
        bool twin = false;
        for (auto& kept : result)
            twin = twin || (kept.startSixteenth == note.startSixteenth && kept.midi == note.midi);
        if (!twin)
            result.add(note);
    }
    return result;
}
