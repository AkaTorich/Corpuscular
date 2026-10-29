/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <complex>
#include <vector>
#include <cmath>
#include <utility>

namespace wv
{
    constexpr double kPi = 3.14159265358979323846;

    // БПФ по основанию 2 на месте. Обратное — без деления на размер.
    inline void fft(std::vector<std::complex<double>>& a, bool inverse)
    {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap(a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = 2.0 * kPi / (double) len * (inverse ? 1.0 : -1.0);
            const std::complex<double> wl(std::cos(ang), std::sin(ang));
            for (size_t i = 0; i < n; i += len)
            {
                std::complex<double> w(1.0, 0.0);
                for (size_t k = 0; k < len / 2; ++k)
                {
                    const auto u = a[i + k];
                    const auto v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
    }

    inline bool isPowerOfTwo(int n) { return n > 0 && (n & (n - 1)) == 0; }
}
