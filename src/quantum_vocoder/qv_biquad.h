/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <cmath>
#include "qv_fft.h"

namespace wv
{
    // Фильтр второго порядка по формулам RBJ (Audio EQ Cookbook)
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1 = 0, z2 = 0;

        void reset() { z1 = z2 = 0; }

        inline float process(float x)
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }

        // Полоса с усилением 0 дБ на центральной частоте
        void setBandPass(double sr, double freq, double q)
        {
            const double w = 2 * kPi * freq / sr, alpha = std::sin(w) / (2 * q), a0 = 1 + alpha;
            set(alpha / a0, 0, -alpha / a0, -2 * std::cos(w) / a0, (1 - alpha) / a0);
        }

        void setHighPass(double sr, double freq, double q)
        {
            const double w = 2 * kPi * freq / sr, alpha = std::sin(w) / (2 * q), c = std::cos(w), a0 = 1 + alpha;
            set((1 + c) / 2 / a0, -(1 + c) / a0, (1 + c) / 2 / a0, -2 * c / a0, (1 - alpha) / a0);
        }

        void setLowPass(double sr, double freq, double q)
        {
            const double w = 2 * kPi * freq / sr, alpha = std::sin(w) / (2 * q), c = std::cos(w), a0 = 1 + alpha;
            set((1 - c) / 2 / a0, (1 - c) / a0, (1 - c) / 2 / a0, -2 * c / a0, (1 - alpha) / a0);
        }

    private:
        void set(double nb0, double nb1, double nb2, double na1, double na2)
        {
            b0 = (float) nb0; b1 = (float) nb1; b2 = (float) nb2; a1 = (float) na1; a2 = (float) na2;
        }
    };
}
