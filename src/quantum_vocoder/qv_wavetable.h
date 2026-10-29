/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <complex>
#include <memory>
#include <vector>

namespace wv
{
    // Волновая таблица: до 256 кадров по 2048 отсчётов.
    // Для каждого кадра хранится 11 копий с урезанными гармониками (1024, 512 … 1),
    // чтобы высокие ноты не звенели отражёнными призвуками.
    class Wavetable
    {
    public:
        static constexpr int frameSize = 2048;
        static constexpr int numLevels = 11;
        static constexpr int maxFrames = 256;
        static constexpr int maxHarmonic = frameSize / 2;

        // Спектр кадра: индексы 0..maxHarmonic, нулевой (постоянная составляющая) не используется
        using Spectrum = std::vector<std::complex<double>>;

        juce::String name;
        juce::File file;          // пустой — встроенная таблица
        int builtInIndex = -1;

        int getNumFrames() const { return numFrames; }

        // frameSize + 1 отсчётов: последний повторяет первый, чтобы читать без проверки края
        const float* data(int frame, int level) const
        {
            return samples.data() + ((size_t) frame * numLevels + (size_t) level) * (frameSize + 1);
        }

        // Какую копию кадра брать для ноты этой частоты
        static int levelFor(double freq, double sampleRate);

        static std::unique_ptr<Wavetable> fromSpectra(const std::vector<Spectrum>& frames, const juce::String& name);
        static std::unique_ptr<Wavetable> fromSamples(const std::vector<float>& audio, int frameLen, const juce::String& name);

        // Форматы — WavetableFormats.cpp: .wav/.aif/.flac, .wt, .raw/.f32, .vitaltable
        static std::unique_ptr<Wavetable> loadFile(const juce::File& f, juce::String& error);
        static bool isSupportedFile(const juce::File& f);

        // Встроенные таблицы — BuiltInTables.cpp
        static int numBuiltIns();
        static juce::String builtInName(int index);
        static std::unique_ptr<Wavetable> builtIn(int index);

    private:
        int numFrames = 0;
        std::vector<float> samples;
    };
}
