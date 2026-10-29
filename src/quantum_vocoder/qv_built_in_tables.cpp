/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "qv_wavetable.h"
#include "qv_fft.h"
#include <algorithm>
#include <cmath>

// Встроенные таблицы: строятся по гармоникам, чтобы плагин звучал и без загруженных файлов.
namespace wv
{
    namespace
    {
        constexpr int kFrames = 64;
        using Spectrum = Wavetable::Spectrum;

        Spectrum empty() { return Spectrum((size_t) Wavetable::maxHarmonic + 1); }

        // Пила переходит в синус (с пилы — самый разборчивый робот)
        Spectrum basicMorph(double t)
        {
            auto s = empty();
            for (int k = 1; k < 512; ++k)
            {
                const double saw = (k % 2 ? 1.0 : -1.0) / k;
                const double a = (k == 1 ? t : 0.0) + (1.0 - t) * saw;
                s[(size_t) k] = std::complex<double>(0, -a);
            }
            return s;
        }

        // Меандр сужается в узкий импульс
        Spectrum pulseSweep(double t)
        {
            auto s = empty();
            const double duty = 0.5 - 0.46 * t;
            for (int k = 1; k < 512; ++k)
                s[(size_t) k] = std::sin(kPi * k * duty) / k;
            return s;
        }

        // Гласные а → э → и → о → у: гармоники основы 110 Гц под пиками формант
        Spectrum vowelFormants(double t)
        {
            static const double F[5][3] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 },
                                            { 570, 840, 2410 }, { 300, 870, 2240 } };
            const double pos = t * 4.0;
            const int v0 = std::min(3, (int) pos);
            const double fr = pos - v0;
            const double bw[3] = { 90, 120, 170 }, gain[3] = { 1.0, 0.7, 0.4 };

            auto s = empty();
            for (int k = 1; k <= 45; ++k)
            {
                const double fk = 110.0 * k;
                double a = 0.04 / k;
                for (int j = 0; j < 3; ++j)
                {
                    const double fj = F[v0][j] + (F[v0 + 1][j] - F[v0][j]) * fr;
                    const double d = (fk - fj) / bw[j];
                    a += gain[j] * std::exp(-d * d);
                }
                s[(size_t) k] = a;
            }
            return s;
        }

        // Цифровой: наборы гармоник включаются и гаснут по битам номера кадра
        Spectrum digitalBits(double t)
        {
            auto s = empty();
            const int frame = (int) std::lround(t * (kFrames - 1));
            const int pattern = frame * 37 + 11;
            for (int k = 1; k < 160; ++k)
            {
                const bool on = k == 1 || ((k * 7 + pattern) % 5 == 0) || (((k ^ pattern) & 7) == 0);
                if (on) s[(size_t) k] = std::complex<double>(0, -1.0 / std::pow((double) k, 0.7));
            }
            return s;
        }

        struct Recipe { const char* name; Spectrum (*make)(double); };
        const Recipe kRecipes[] = {
            { "Basic Morph", basicMorph },
            { "Pulse Sweep", pulseSweep },
            { "Vowel Formants", vowelFormants },
            { "Digital Bits", digitalBits },
        };
    }

    int Wavetable::numBuiltIns() { return (int) (sizeof(kRecipes) / sizeof(kRecipes[0])); }

    juce::String Wavetable::builtInName(int index)
    {
        return kRecipes[((index % numBuiltIns()) + numBuiltIns()) % numBuiltIns()].name;
    }

    std::unique_ptr<Wavetable> Wavetable::builtIn(int index)
    {
        index = ((index % numBuiltIns()) + numBuiltIns()) % numBuiltIns();
        std::vector<Spectrum> frames;
        for (int f = 0; f < kFrames; ++f) frames.push_back(kRecipes[index].make((double) f / (kFrames - 1)));
        auto t = fromSpectra(frames, kRecipes[index].name);
        t->builtInIndex = index;
        return t;
    }
}
