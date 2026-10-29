/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "qv_pitch_tracker.h"
#include <algorithm>
#include <cmath>

namespace wv
{
    void PitchTracker::prepare(double sampleRate)
    {
        decim = std::max(1, (int) std::lround(sampleRate / 12000.0));
        dsr = sampleRate / decim;
        window = (int) (dsr * 0.042);
        minLag = std::max(2, (int) (dsr / 1000.0));
        maxLag = (int) (dsr / 60.0);
        hop = window / 2;

        lp1.setLowPass(sampleRate, std::min(2500.0, dsr * 0.4), 0.707);
        lp2 = lp1;
        lp1.reset(); lp2.reset();

        ring.assign((size_t) (window + maxLag) * 2, 0.0f);
        work.assign((size_t) (window + maxLag), 0.0f);
        diff.assign((size_t) maxLag + 2, 0.0f);
        writePos = sinceLast = filled = decimCount = 0;
        freq = 0; voiced = false;
    }

    void PitchTracker::push(float x)
    {
        const float y = lp2.process(lp1.process(x));
        if (++decimCount < decim) return;
        decimCount = 0;

        ring[(size_t) writePos] = y;
        writePos = (writePos + 1) % (int) ring.size();
        filled = std::min(filled + 1, (int) ring.size());
        if (++sinceLast >= hop && filled >= (int) work.size())
        {
            sinceLast = 0;
            analyse();
        }
    }

    void PitchTracker::analyse()
    {
        const int n = (int) work.size(), size = (int) ring.size();
        double energy = 0;
        for (int i = 0; i < n; ++i)
        {
            work[(size_t) i] = ring[(size_t) ((writePos - n + i + size) % size)];
            if (i < window) energy += work[(size_t) i] * work[(size_t) i];
        }
        ++version;
        if (std::sqrt(energy / window) < 0.008) { voiced = false; return; }

        // Разностная функция и её нормировка по накопленному среднему
        diff[0] = 1;
        double running = 0;
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            double d = 0;
            for (int j = 0; j < window; ++j)
            {
                const float e = work[(size_t) j] - work[(size_t) (j + tau)];
                d += e * e;
            }
            running += d;
            diff[(size_t) tau] = running > 0 ? (float) (d * tau / running) : 1.0f;
        }

        int best = -1;
        for (int tau = minLag; tau < maxLag; ++tau)
            if (diff[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < maxLag && diff[(size_t) tau + 1] < diff[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        if (best < 0) { voiced = false; return; }

        // Уточнение между отсчётами по параболе
        const double a = diff[(size_t) best - 1], b = diff[(size_t) best], c = diff[(size_t) best + 1];
        const double den = a - 2 * b + c;
        const double shift = std::abs(den) > 1e-9 ? 0.5 * (a - c) / den : 0.0;
        freq = (float) (dsr / (best + std::clamp(shift, -0.5, 0.5)));
        voiced = true;
    }
}
