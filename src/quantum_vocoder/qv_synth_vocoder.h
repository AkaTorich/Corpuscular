/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include "qv_biquad.h"
#include "qv_carrier_synth.h"
#include "qv_pitch_tracker.h"
#include <atomic>

// Quantum Vocoder inside Corpuscular's effect chain.
// The voice comes from the audio input (sidechain in a DAW, microphone in the app); what speaks is either the synth
// itself (SYNTH, stereo) or a wavetable carrier (TABLE) whose pitch follows the voice (AUTO), a knob (FIXED) or the
// last played note (KEY). On top of the original plugin: stereo bands, noise only on consonants, freeze, gate,
// tilt and band spread.
namespace wv
{
    struct SynthVocoderParams
    {
        bool tableCarrier = false;
        int pitchMode = 0;          // 0 AUTO, 1 FIXED, 2 KEY
        float note = 48;
        int octave = 0;
        float position = 0;
        int unison = 1;
        float detuneCents = 12;
        float glideMs = 25;

        int bands = 24;
        float width = 1;
        float formantOct = 0;
        float attackMs = 4;
        float releaseMs = 60;

        float air = 0.25f;
        float noise = 0.5f;         // added to the carrier on consonants only
        float gateDb = -70;
        float tilt = 0;             // -1..1: tilts the band levels by up to 9 dB at the ends
        float spread = 0;           // 0..1: neighbouring bands to opposite sides
        bool freeze = false;        // hold the current voice spectrum
        float mix = 1;
        float outputDb = 0;
    };

    class SynthVocoder
    {
    public:
        static constexpr int maxBands = 40;
        static constexpr double lowFreq = 80.0, highFreq = 12000.0;

        void prepare(double sampleRate);
        void reset();
        void setTable(const Wavetable* table) { carrier.setTable(table); }

        // voice: mono modulator; left/right: the synth in, the vocoder out
        void process(const float* voice, float* left, float* right, int numSamples,
                     const SynthVocoderParams& p, float keyNote);

        int getNumBands() const { return numBandsForUi.load(); }
        float getBandLevel(int band) const { return bandLevels[band].load(); }
        float getVoiceLevel() const { return voiceLevelForUi.load(); }

    private:
        void setup(const SynthVocoderParams& p);
        void updateFilters();
        void updateAutoPitch(const SynthVocoderParams& p);

        double sr = 44100;
        CarrierSynth carrier;
        PitchTracker tracker;

        // Bank: the voice once, the carrier per side
        int numBands = 24;
        float width = 1, formant = 0, tilt = 0, spread = 0;
        bool dirty = true;
        Biquad voiceF[maxBands][2], carrierF[2][maxBands][2];
        float env[maxBands] {};
        float bandGain[maxBands][2] {};
        float bankGain = 1;
        float attackCoef = 0.1f, releaseCoef = 0.01f;

        // Consonants: share of the voice above 4 kHz
        Biquad airF[2], hissF[2];
        float hissEnv = 0, voiceEnv = 0, unvoiced = 0, hissCoef = 0, unvoicedCoef = 0;

        // Gate on the voice level
        float gate = 0, gateCoef = 0;

        // No voice at all (nothing connected): the synth's own sound becomes the modulator
        int silentCount = 0, silentLimit = 88200;
        float voicePresent = 0, presentCoef = 0;

        // Level matching and limiter from the original plugin
        float inPower = 0, wetPower = 0, levelGain = 1, levelCoef = 0.0001f, levelUp = 0, levelDown = 0;
        float inEnv = 0, outEnv = 0, limGain = 1, inEnvAttack = 0, outEnvAttack = 0, inEnvRel = 0, outEnvRel = 0;
        float limAttack = 0, limRelease = 0, lastReleaseMs = -1;

        // Carrier pitch
        int pitchMode = -1;
        bool autoLocked = false;
        int lastTrackerVersion = -1;
        double acceptedNote = -1, pendingNote = -1;
        int pendingCount = 0;
        uint32_t noiseState = 22222;

        std::atomic<float> bandLevels[maxBands] {};
        std::atomic<int> numBandsForUi { 24 };
        std::atomic<float> voiceLevelForUi { 0 };
    };
}
