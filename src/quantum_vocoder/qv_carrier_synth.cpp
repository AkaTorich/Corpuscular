/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "qv_carrier_synth.h"
#include <algorithm>
#include <cmath>

namespace wv
{
    static double noteToHz(double note) { return 440.0 * std::pow(2.0, (note - 69.0) / 12.0); }

    void CarrierSynth::prepare(double sampleRate)
    {
        sr = sampleRate;
        ampCoef = 1.0 - std::exp(-1.0 / (sr * 0.008));   // ~8 мс на вход и выход ноты, без щелчков
        for (auto& v : voices) v = Voice();
        levelCounter = 0;
    }

    void CarrierSynth::setPoly(bool p)
    {
        if (p == poly) return;
        poly = p;
        for (auto& v : voices) v = Voice();
    }

    void CarrierSynth::startVoice(Voice& v, double hz, float velocity, bool glide)
    {
        const double target = std::log(hz);
        if (!v.active || !glide)
        {
            v.logFreq = target;
            if (!v.active)
                for (int u = 0; u < maxUnison; ++u) v.phase[u] = std::fmod(0.137 * u, 1.0);
        }
        v.logTarget = target;
        v.active = v.held = true;
        v.velocity = velocity;
        v.age = clock++;
        v.level = Wavetable::levelFor(hz * unisonRatio[std::max(0, unison - 1)], sr);
    }

    void CarrierSynth::setMonoFrequency(float hz)
    {
        if (poly || hz <= 0) return;
        Voice& v = voices[0];
        if (!v.active || !v.held) startVoice(v, hz, 1.0f, true);
        else v.logTarget = std::log((double) hz);
    }

    void CarrierSynth::noteOn(int note, float velocity)
    {
        if (!poly) return;
        Voice* pick = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) { pick = &v; break; }
        if (pick == nullptr) for (auto& v : voices) if (!v.active) { pick = &v; break; }
        if (pick == nullptr)
        {
            pick = &voices[0];
            for (auto& v : voices) if (v.age < pick->age) pick = &v;   // забираем самый старый
        }
        pick->note = note;
        startVoice(*pick, noteToHz(note + 12.0 * octave), velocity, true);
    }

    void CarrierSynth::noteOff(int note)
    {
        for (auto& v : voices) if (v.active && v.note == note) v.held = false;
    }

    void CarrierSynth::allNotesOff()
    {
        for (auto& v : voices) v.held = false;
    }

    void CarrierSynth::setParams(float newPosition, int newUnison, float detuneCents, float glideMs, int newOctave)
    {
        position = std::clamp(newPosition, 0.0f, 1.0f);
        unison = std::clamp(newUnison, 1, maxUnison);
        detune = detuneCents;
        glideCoef = glideMs <= 0.5f ? 1.0 : 1.0 - std::exp(-1.0 / (sr * glideMs * 0.001));

        for (int u = 0; u < maxUnison; ++u)
        {
            const double spread = unison > 1 ? -1.0 + 2.0 * u / (unison - 1) : 0.0;
            unisonRatio[u] = std::pow(2.0, spread * detune / 1200.0);
        }

        if (newOctave != octave)
        {
            octave = newOctave;
            if (poly)
                for (auto& v : voices)
                    if (v.active && v.note >= 0) v.logTarget = std::log(noteToHz(v.note + 12.0 * octave));
        }
    }

    float CarrierSynth::renderVoice(Voice& v)
    {
        const double freq = std::exp(v.logFreq);
        const int frames = table->getNumFrames();
        const float fpos = position * (float) (frames - 1);
        const int f0 = (int) fpos;
        const int f1 = std::min(f0 + 1, frames - 1);
        const float fr = fpos - (float) f0;
        const float* a = table->data(f0, v.level);
        const float* b = table->data(f1, v.level);

        float sum = 0;
        for (int u = 0; u < unison; ++u)
        {
            const double p = v.phase[u] * Wavetable::frameSize;
            const int i = (int) p;
            const float t = (float) (p - i);
            const float sa = a[i] + (a[i + 1] - a[i]) * t;
            const float sb = b[i] + (b[i + 1] - b[i]) * t;
            sum += sa + (sb - sa) * fr;

            v.phase[u] += freq * unisonRatio[u] / sr;
            while (v.phase[u] >= 1.0) v.phase[u] -= 1.0;
        }
        return sum / std::sqrt((float) unison);
    }

    float CarrierSynth::process()
    {
        if (table == nullptr) return 0;

        const bool updateLevels = ++levelCounter >= 64;
        if (updateLevels) levelCounter = 0;

        float out = 0;
        for (auto& v : voices)
        {
            if (!v.active) continue;
            const double targetAmp = v.held ? v.velocity : 0.0;
            v.amp += (float) ((targetAmp - v.amp) * ampCoef);
            if (!v.held && v.amp < 1e-4f) { v.active = false; v.note = -1; continue; }

            v.logFreq += (v.logTarget - v.logFreq) * glideCoef;
            if (updateLevels)
                v.level = Wavetable::levelFor(std::exp(v.logFreq) * unisonRatio[unison - 1], sr);
            out += renderVoice(v) * v.amp;
        }
        return out;
    }
}
