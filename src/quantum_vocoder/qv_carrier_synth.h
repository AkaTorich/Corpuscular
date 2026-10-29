/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include "qv_wavetable.h"
#include <cstdint>

namespace wv
{
    // Несущая вокодера: голоса, читающие волновую таблицу.
    // Моно-режим (одна нота ручкой или высота голоса) — один голос с заданной частотой;
    // поли-режим — до 8 нот с MIDI.
    class CarrierSynth
    {
    public:
        static constexpr int maxVoices = 8;
        static constexpr int maxUnison = 7;

        void prepare(double sampleRate);
        void setTable(const Wavetable* t) { table = t; }

        void setPoly(bool poly);
        void setMonoFrequency(float hz);
        void noteOn(int note, float velocity);
        void noteOff(int note);
        void allNotesOff();

        // octave сдвигает MIDI-ноты; частоту моно-голоса задают уже сдвинутой
        void setParams(float position, int unison, float detuneCents, float glideMs, int octave);

        float process();

    private:
        struct Voice
        {
            bool active = false, held = false;
            int note = -1;
            float velocity = 1, amp = 0;
            double logFreq = 0, logTarget = 0;
            double phase[maxUnison] {};
            uint32_t age = 0;
            int level = 0;
        };

        void startVoice(Voice& v, double hz, float velocity, bool glide);
        float renderVoice(Voice& v);

        double sr = 44100;
        const Wavetable* table = nullptr;
        Voice voices[maxVoices];
        bool poly = false;
        float position = 0, detune = 0;
        int unison = 1, octave = 0;
        double glideCoef = 1, ampCoef = 0.002;
        double unisonRatio[maxUnison] {};
        uint32_t clock = 0;
        int levelCounter = 0;
    };
}
