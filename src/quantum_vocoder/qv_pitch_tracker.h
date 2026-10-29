/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include "qv_biquad.h"
#include <vector>

namespace wv
{
    // Высота голоса по методу YIN: звук прореживается до ~12 кГц,
    // каждые ~20 мс ищется период повторения в окне ~40 мс. Голос 60–1000 Гц.
    class PitchTracker
    {
    public:
        void prepare(double sampleRate);
        void push(float x);

        float getFrequency() const { return freq; }   // 0 — ещё не было голоса
        bool isVoiced() const { return voiced; }
        int getVersion() const { return version; }     // растёт с каждым новым замером

    private:
        void analyse();

        double dsr = 12000;
        int decim = 4, decimCount = 0, window = 512, minLag = 12, maxLag = 200, hop = 256;
        Biquad lp1, lp2;
        std::vector<float> ring, work, diff;
        int writePos = 0, sinceLast = 0, filled = 0;
        float freq = 0;
        bool voiced = false;
        int version = 0;
    };
}
