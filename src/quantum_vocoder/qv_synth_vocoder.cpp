/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "qv_synth_vocoder.h"
#include <algorithm>
#include <cmath>

namespace wv
{
    static double synthVocoderNoteToHz(double note) { return 440.0 * std::pow(2.0, (note - 69.0) / 12.0); }

    static float synthVocoderCoef(double sampleRate, double seconds)
    {
        return (float) (1.0 - std::exp(-1.0 / (sampleRate * seconds)));
    }

    void SynthVocoder::prepare(double sampleRate)
    {
        sr = sampleRate;
        carrier.prepare(sr);
        carrier.setPoly(false);
        tracker.prepare(sr);
        for (auto& f : airF) f.setHighPass(sr, 4500.0, 0.707);
        for (auto& f : hissF) f.setHighPass(sr, 4000.0, 0.707);
        hissCoef = synthVocoderCoef(sr, 0.008);
        unvoicedCoef = synthVocoderCoef(sr, 0.012);
        gateCoef = synthVocoderCoef(sr, 0.01);
        presentCoef = synthVocoderCoef(sr, 0.02);
        silentLimit = (int) (sr * 2.0);

        levelCoef = synthVocoderCoef(sr, 0.25);
        levelUp = synthVocoderCoef(sr, 1.0);
        levelDown = synthVocoderCoef(sr, 0.05);
        inEnvAttack = synthVocoderCoef(sr, 0.02);
        outEnvAttack = synthVocoderCoef(sr, 0.005);
        outEnvRel = synthVocoderCoef(sr, 0.03);
        limAttack = synthVocoderCoef(sr, 0.0005);
        limRelease = synthVocoderCoef(sr, 0.04);
        lastReleaseMs = -1;
        dirty = true;
        reset();
    }

    void SynthVocoder::reset()
    {
        for (int i = 0; i < maxBands; ++i)
        {
            env[i] = 0;
            for (int j = 0; j < 2; ++j)
            {
                voiceF[i][j].reset();
                carrierF[0][i][j].reset();
                carrierF[1][i][j].reset();
            }
            bandLevels[i].store(0.0f);
        }
        for (auto& f : airF) f.reset();
        for (auto& f : hissF) f.reset();
        hissEnv = voiceEnv = unvoiced = gate = 0;
        silentCount = silentLimit;
        voicePresent = 0;
        inPower = wetPower = 0;
        levelGain = 1;
        inEnv = outEnv = 0;
        limGain = 1;
        pitchMode = -1;
        autoLocked = false;
        acceptedNote = pendingNote = -1;
        pendingCount = 0;
        lastTrackerVersion = -1;
    }

    void SynthVocoder::setup(const SynthVocoderParams& p)
    {
        int bands = std::clamp(p.bands, 8, maxBands);
        float newWidth = std::clamp(p.width, 0.25f, 4.0f);
        if (bands != numBands || newWidth != width || p.formantOct != formant || p.tilt != tilt || p.spread != spread)
        {
            if (bands > numBands)
                for (int i = numBands; i < bands; ++i) env[i] = 0;
            numBands = bands;
            width = newWidth;
            formant = p.formantOct;
            tilt = p.tilt;
            spread = p.spread;
            dirty = true;
        }
        if (dirty) updateFilters();

        attackCoef = synthVocoderCoef(sr, std::max(0.1f, p.attackMs) * 0.001);
        releaseCoef = synthVocoderCoef(sr, std::max(1.0f, p.releaseMs) * 0.001);
        if (p.releaseMs != lastReleaseMs)
        {
            lastReleaseMs = p.releaseMs;
            inEnvRel = synthVocoderCoef(sr, std::clamp(p.releaseMs * 0.001, 0.12, 0.25));
        }

        if (p.pitchMode != pitchMode)
        {
            pitchMode = p.pitchMode;
            autoLocked = false;
            acceptedNote = pendingNote = -1;
            pendingCount = 0;
        }
        carrier.setParams(p.position, p.unison, p.detuneCents, p.glideMs, p.octave);
    }

    void SynthVocoder::updateFilters()
    {
        dirty = false;
        const double top = std::min(highFreq, sr * 0.45);
        const double ratio = std::pow(top / lowFreq, 1.0 / (numBands - 1));
        const double s = std::sqrt(ratio);
        const double q = 1.0 / (s - 1.0 / s) / std::max(0.25f, width);
        const double shift = std::pow(2.0, (double) formant);

        for (int i = 0; i < numBands; ++i)
        {
            const double f = lowFreq * std::pow(ratio, i);
            const double fc = std::clamp(f * shift, 30.0, sr * 0.46);
            for (int j = 0; j < 2; ++j)
            {
                voiceF[i][j].setBandPass(sr, f, q);
                carrierF[0][i][j].setBandPass(sr, fc, q);
                carrierF[1][i][j].setBandPass(sr, fc, q);
            }

            // Tilt: up to 9 dB at the ends; spread: equal-power pan, odd bands right, even bands left
            const double position = numBands > 1 ? (double) i / (numBands - 1) : 0.5;
            const double tiltGain = std::pow(10.0, tilt * 9.0 * (position - 0.5) * 2.0 / 20.0);
            const double pan = spread * ((i % 2) ? 1.0 : -1.0);
            const double angle = (pan + 1.0) * kPi * 0.25;
            bandGain[i][0] = (float) (tiltGain * std::cos(angle) * std::sqrt(2.0));
            bandGain[i][1] = (float) (tiltGain * std::sin(angle) * std::sqrt(2.0));
        }
        // More bands: each narrower and quieter; the level matching does the rest
        bankGain = 6.0f * std::sqrt(24.0f / (float) numBands);
    }

    void SynthVocoder::updateAutoPitch(const SynthVocoderParams& p)
    {
        lastTrackerVersion = tracker.getVersion();
        if (!tracker.isVoiced()) return;
        double note = std::round(69.0 + 12.0 * std::log2(tracker.getFrequency() / 440.0));

        if (acceptedNote >= 0 && std::abs(note - acceptedNote) > 7.0)
        {
            if (pendingNote >= 0 && std::abs(note - pendingNote) <= 1.0) ++pendingCount;
            else { pendingNote = note; pendingCount = 1; }
            if (pendingCount < 2) return;   // a single jump (octave error, a hiss) is ignored
        }
        acceptedNote = note;
        pendingNote = -1;
        pendingCount = 0;
        carrier.setMonoFrequency((float) std::clamp(synthVocoderNoteToHz(note + 12.0 * p.octave), 20.0, 5000.0));
        autoLocked = true;
    }

    void SynthVocoder::process(const float* voice, float* left, float* right, int numSamples,
                               const SynthVocoderParams& p, float keyNote)
    {
        if (numSamples <= 0) return;
        setup(p);

        if (p.tableCarrier)
        {
            // FIXED: the knob; KEY: the last played note; AUTO: the knob until the voice has a pitch
            if (p.pitchMode == 1 || (p.pitchMode == 0 && !autoLocked))
                carrier.setMonoFrequency((float) synthVocoderNoteToHz(p.note + 12.0 * p.octave));
            else if (p.pitchMode == 2 && keyNote > 0.0f)
                carrier.setMonoFrequency((float) synthVocoderNoteToHz(keyNote + 12.0 * p.octave));
        }

        const float mix = std::clamp(p.mix, 0.0f, 1.0f);
        const float outGain = std::pow(10.0f, p.outputDb / 20.0f);
        const float gateLevel = std::pow(10.0f, p.gateDb / 10.0f);   // as power
        const bool stereoCarrier = !p.tableCarrier;

        for (int i = 0; i < numSamples; ++i)
        {
            const float external = voice[i];

            // Two seconds of digital silence mean no voice is connected (pauses between phrases are shorter)
            if (std::abs(external) > 1e-6f) silentCount = 0;
            else if (silentCount < silentLimit) ++silentCount;
            voicePresent += presentCoef * ((silentCount < silentLimit ? 1.0f : 0.0f) - voicePresent);

            // Without a voice the synth speaks through itself: its own sound is the modulator
            const float v = external * voicePresent + 0.5f * (left[i] + right[i]) * (1.0f - voicePresent);
            const float wetMix = mix;

            if (p.tableCarrier && p.pitchMode == 0)
            {
                tracker.push(v);
                if (tracker.getVersion() != lastTrackerVersion) updateAutoPitch(p);
            }

            // Consonant detector: how much of the voice sits above 4 kHz
            const float hiss = hissF[1].process(hissF[0].process(v));
            hissEnv += hissCoef * (hiss * hiss - hissEnv);
            voiceEnv += hissCoef * (v * v - voiceEnv);
            const float ratio = voiceEnv > 1e-9f ? hissEnv / voiceEnv : 0.0f;
            const float target = std::clamp((ratio - 0.2f) / 0.35f, 0.0f, 1.0f);
            unvoiced += unvoicedCoef * (target - unvoiced);

            // Gate: below the threshold the bands close
            gate += gateCoef * ((voiceEnv > gateLevel ? 1.0f : 0.0f) - gate);

            noiseState ^= noiseState << 13; noiseState ^= noiseState >> 17; noiseState ^= noiseState << 5;
            const float white = (float) noiseState * (2.0f / 4294967296.0f) - 1.0f;
            const float noise = white * p.noise * unvoiced;

            float carrierL, carrierR;
            if (p.tableCarrier)
                carrierL = carrierR = carrier.process() + noise;
            else
            {
                carrierL = left[i] + noise;
                carrierR = right[i] + noise;
            }

            float wetL = 0, wetR = 0;
            for (int b = 0; b < numBands; ++b)
            {
                const float band = voiceF[b][1].process(voiceF[b][0].process(v));
                if (!p.freeze)
                {
                    const float a = std::abs(band) * gate;
                    env[b] += (a > env[b] ? attackCoef : releaseCoef) * (a - env[b]);
                }
                const float cL = carrierF[0][b][1].process(carrierF[0][b][0].process(carrierL));
                const float cR = stereoCarrier ? carrierF[1][b][1].process(carrierF[1][b][0].process(carrierR)) : cL;
                wetL += cL * env[b] * bandGain[b][0];
                wetR += cR * env[b] * bandGain[b][1];
            }
            wetL *= bankGain;
            wetR *= bankGain;

            // Keep the vocoder at the loudness of the voice whatever the carrier is
            const float voicePower = p.freeze ? inPower : v * v;
            inPower += levelCoef * (voicePower - inPower);
            const float wetMean = 0.5f * (wetL * wetL + wetR * wetR);
            wetPower += levelCoef * (wetMean - wetPower);
            if (inPower > 1e-7f)
            {
                const float level = std::clamp(std::sqrt(inPower / (wetPower + 1e-9f)), 0.25f, 8.0f);
                levelGain += (level > levelGain ? levelUp : levelDown) * (level - levelGain);
            }

            // Limiter against howls at word ends (held when frozen, the voice is no reference then)
            const float outPower = wetMean * levelGain * levelGain;
            if (!p.freeze)
            {
                inEnv += (voicePower > inEnv ? inEnvAttack : inEnvRel) * (voicePower - inEnv);
                outEnv += (outPower > outEnv ? outEnvAttack : outEnvRel) * (outPower - outEnv);
                const float limit = 1.5625f * inEnv + 1e-9f;
                const float limTarget = outEnv > limit ? std::sqrt(limit / outEnv) : 1.0f;
                limGain += (limTarget < limGain ? limAttack : limRelease) * (limTarget - limGain);
            }

            const float air = airF[1].process(airF[0].process(v)) * p.air * gate;
            const float gain = levelGain * limGain;
            float outL = (left[i] * (1.0f - wetMix) + (wetL * gain + air) * wetMix) * outGain;
            float outR = (right[i] * (1.0f - wetMix) + (wetR * gain + air) * wetMix) * outGain;

            // Soft ceiling on the vocoded sound; a fully dry mix stays as it is
            if (wetMix > 0.0f)
            {
                for (float* out : { &outL, &outR })
                {
                    const float a = std::abs(*out);
                    if (a > 0.9f) *out = std::copysign(0.9f + 0.1f * std::tanh((a - 0.9f) / 0.1f), *out);
                }
            }
            left[i] = outL;
            right[i] = outR;
        }

        numBandsForUi.store(numBands);
        for (int b = 0; b < maxBands; ++b)
            bandLevels[b].store(b < numBands ? env[b] : 0.0f);
        voiceLevelForUi.store(std::sqrt(voiceEnv));
    }
}
