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

namespace wv
{
    int Wavetable::levelFor(double freq, double sampleRate)
    {
        if (freq <= 0) return 0;
        const double allowed = 0.45 * sampleRate / freq;   // столько гармоник ещё не отражается
        int level = 0;
        while (level < numLevels - 1 && (maxHarmonic >> level) > allowed) ++level;
        return level;
    }

    std::unique_ptr<Wavetable> Wavetable::fromSpectra(const std::vector<Spectrum>& frames, const juce::String& tableName)
    {
        auto t = std::make_unique<Wavetable>();
        t->name = tableName;
        t->numFrames = (int) std::min<size_t>(frames.size(), (size_t) maxFrames);
        if (t->numFrames == 0) return nullptr;
        t->samples.assign((size_t) t->numFrames * numLevels * (frameSize + 1), 0.0f);

        std::vector<std::complex<double>> work(frameSize);
        std::vector<double> level0(frameSize);
        for (int f = 0; f < t->numFrames; ++f)
        {
            const auto& spec = frames[(size_t) f];
            double scale = 0;
            for (int l = 0; l < numLevels; ++l)
            {
                const int top = std::min((maxHarmonic >> l), maxHarmonic - 1);
                std::fill(work.begin(), work.end(), std::complex<double>());
                for (int k = 1; k <= top && k < (int) spec.size(); ++k)
                {
                    work[(size_t) k] = spec[(size_t) k];
                    work[(size_t) (frameSize - k)] = std::conj(spec[(size_t) k]);
                }
                fft(work, true);

                if (l == 0)
                {
                    double peak = 0;
                    for (int i = 0; i < frameSize; ++i) peak = std::max(peak, std::abs(work[(size_t) i].real()));
                    scale = peak > 1e-12 ? 1.0 / peak : 0.0;   // каждый кадр — в полный размах
                }
                float* out = const_cast<float*>(t->data(f, l));
                for (int i = 0; i < frameSize; ++i) out[i] = (float) (work[(size_t) i].real() * scale);
                out[frameSize] = out[0];
            }
        }
        return t;
    }

    std::unique_ptr<Wavetable> Wavetable::fromSamples(const std::vector<float>& audio, int frameLen, const juce::String& tableName)
    {
        if (audio.empty()) return nullptr;
        if (frameLen <= 0 || (size_t) frameLen > audio.size()) frameLen = (int) audio.size();

        const int total = (int) (audio.size() / (size_t) frameLen);
        const int count = std::min(total, maxFrames);
        std::vector<Spectrum> spectra((size_t) count);

        const bool pow2 = isPowerOfTwo(frameLen);
        const int fftLen = pow2 ? frameLen : frameSize;
        std::vector<std::complex<double>> work((size_t) fftLen);

        for (int f = 0; f < count; ++f)
        {
            // Если кадров больше 256 — берём равномерно через всю таблицу
            const int src = count < total ? (int) ((int64_t) f * total / count) : f;
            const float* frame = audio.data() + (size_t) src * (size_t) frameLen;

            if (pow2)
                for (int i = 0; i < fftLen; ++i) work[(size_t) i] = frame[i];
            else
                for (int i = 0; i < fftLen; ++i)
                {
                    const double pos = (double) i * frameLen / fftLen;
                    const int i0 = (int) pos;
                    const int i1 = (i0 + 1) % frameLen;
                    const double fr = pos - i0;
                    work[(size_t) i] = frame[i0] + (frame[i1] - frame[i0]) * fr;
                }
            fft(work, false);

            auto& spec = spectra[(size_t) f];
            spec.assign((size_t) maxHarmonic + 1, std::complex<double>());
            const int top = std::min(fftLen / 2 - 1, maxHarmonic - 1);
            for (int k = 1; k <= top; ++k) spec[(size_t) k] = work[(size_t) k];
        }
        return fromSpectra(spectra, tableName);
    }

}
