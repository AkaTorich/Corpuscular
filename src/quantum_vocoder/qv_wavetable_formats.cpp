/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "qv_wavetable.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Чтение таблиц разных синтезаторов:
//   .wav/.aif/.flac — Serum, Vital, Ableton, Phase Plant, Surge, одноцикловые наборы (AKWF и т. п.)
//   .wt             — Surge, Bitwig
//   .raw/.f32       — голые float32 (WavetableGenerator «Raw»)
//   .vitaltable     — родной формат Vital (JSON с волнами в base64)
namespace wv
{
    namespace
    {
        // Размер кадра по длине, когда служебной пометки нет
        int guessFrameSize(int len)
        {
            if (len % 2048 == 0) return 2048;
            for (int s : { 1024, 512, 256, 4096, 128 })
                if (len % s == 0 && len / s <= Wavetable::maxFrames) return s;
            if (len <= 8192) return len;   // одна волна целиком
            return 2048;
        }

        // Пометки в RIFF: clm (Serum: "<!>2048 …") и srge/srgo (Surge: версия, размер кадра). 0 — пометки нет.
        int readWavFrameSize(const juce::File& f)
        {
            juce::FileInputStream in(f);
            if (!in.openedOk()) return 0;
            char id[4];
            if (in.read(id, 4) != 4 || memcmp(id, "RIFF", 4) != 0) return 0;
            in.readInt();
            if (in.read(id, 4) != 4 || memcmp(id, "WAVE", 4) != 0) return 0;

            while (!in.isExhausted())
            {
                if (in.read(id, 4) != 4) break;
                const int size = in.readInt();
                if (size < 0) break;
                const juce::int64 next = in.getPosition() + size + (size & 1);

                if (memcmp(id, "clm ", 4) == 0 && size < 4096)
                {
                    juce::MemoryBlock mb;
                    in.readIntoMemoryBlock(mb, size);
                    const auto s = juce::String::fromUTF8(static_cast<const char*>(mb.getData()), (int) mb.getSize());
                    const int at = s.indexOf("<!>");
                    if (at >= 0)
                    {
                        const int v = s.substring(at + 3).getIntValue();
                        if (v >= 16 && v <= 65536) return v;
                    }
                }
                else if ((memcmp(id, "srge", 4) == 0 || memcmp(id, "srgo", 4) == 0) && size >= 8)
                {
                    in.readInt();
                    const int v = in.readInt();
                    if (v >= 16 && v <= 65536) return v;
                }
                if (!in.setPosition(next)) break;
            }
            return 0;
        }

        std::unique_ptr<Wavetable> loadAudio(const juce::File& f, juce::String& error)
        {
            juce::AudioFormatManager fm;
            fm.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> r(fm.createReaderFor(f));
            if (r == nullptr) { error = "unsupported audio file"; return nullptr; }

            const int len = (int) std::min<juce::int64>(r->lengthInSamples, 8 * 1024 * 1024);
            if (len <= 0) { error = "empty file"; return nullptr; }
            const int channels = (int) juce::jmax(1u, r->numChannels);
            juce::AudioBuffer<float> buf(channels, len);
            r->read(&buf, 0, len, 0, true, true);

            std::vector<float> audio((size_t) len, 0.0f);   // стерео — в моно
            for (int c = 0; c < channels; ++c)
            {
                const float* src = buf.getReadPointer(c);
                for (int i = 0; i < len; ++i) audio[(size_t) i] += src[i] / (float) channels;
            }

            int frame = f.hasFileExtension("wav") ? readWavFrameSize(f) : 0;
            if (frame <= 0) frame = guessFrameSize(len);
            return Wavetable::fromSamples(audio, frame, f.getFileNameWithoutExtension());
        }

        std::unique_ptr<Wavetable> loadWt(const juce::File& f, juce::String& error)
        {
            juce::MemoryBlock mb;
            if (!f.loadFileAsData(mb) || mb.getSize() < 12) { error = "cannot read file"; return nullptr; }
            const auto* b = static_cast<const uint8_t*>(mb.getData());
            if (memcmp(b, "vawt", 4) != 0) { error = "not a .wt wavetable"; return nullptr; }

            const int size = (int) juce::ByteOrder::littleEndianInt(b + 4);
            const int count = (int) juce::ByteOrder::littleEndianShort(b + 8);
            const int flags = (int) juce::ByteOrder::littleEndianShort(b + 10);
            const bool int16 = (flags & 4) != 0;
            const size_t need = (size_t) size * (size_t) count * (int16 ? 2u : 4u);
            if (size <= 0 || count <= 0 || mb.getSize() < 12 + need) { error = "broken .wt header"; return nullptr; }

            std::vector<float> audio((size_t) size * (size_t) count);
            const uint8_t* d = b + 12;
            const float scale = (flags & 8) ? 1.0f / 32768.0f : 1.0f / 16384.0f;
            for (size_t i = 0; i < audio.size(); ++i)
            {
                if (int16) audio[i] = (float) (int16_t) juce::ByteOrder::littleEndianShort(d + i * 2) * scale;
                else
                {
                    const uint32_t bits = juce::ByteOrder::littleEndianInt(d + i * 4);
                    float v; memcpy(&v, &bits, 4); audio[i] = v;
                }
            }
            return Wavetable::fromSamples(audio, size, f.getFileNameWithoutExtension());
        }

        std::unique_ptr<Wavetable> loadRaw(const juce::File& f, juce::String& error)
        {
            juce::MemoryBlock mb;
            if (!f.loadFileAsData(mb) || mb.getSize() < 64) { error = "cannot read file"; return nullptr; }
            const int len = (int) (mb.getSize() / 4);
            std::vector<float> audio((size_t) len);
            const auto* d = static_cast<const uint8_t*>(mb.getData());
            for (int i = 0; i < len; ++i)
            {
                const uint32_t bits = juce::ByteOrder::littleEndianInt(d + (size_t) i * 4);
                float v; memcpy(&v, &bits, 4);
                audio[(size_t) i] = std::isfinite(v) ? juce::jlimit(-4.0f, 4.0f, v) : 0.0f;
            }
            return Wavetable::fromSamples(audio, guessFrameSize(len), f.getFileNameWithoutExtension());
        }

        std::vector<float> decodeFloats(const juce::String& base64)
        {
            juce::MemoryOutputStream out;
            if (base64.isEmpty() || !juce::Base64::convertFromBase64(out, base64)) return {};
            std::vector<float> v(out.getDataSize() / 4);
            if (!v.empty()) memcpy(v.data(), out.getData(), v.size() * 4);
            return v;
        }

        // Vital: groups → components → keyframes[].wave_data (2048 float32 в base64),
        // либо компонент «Audio File Source»: audio_file + window_size
        std::unique_ptr<Wavetable> loadVital(const juce::File& f, juce::String& error)
        {
            const auto json = juce::JSON::parse(f.loadFileAsString());
            const auto* groups = json["groups"].getArray();
            if (groups == nullptr) { error = "not a .vitaltable wavetable"; return nullptr; }

            std::vector<float> audio;
            int frameLen = 2048;
            for (const auto& g : *groups)
            {
                const auto* comps = g["components"].getArray();
                if (comps == nullptr) continue;
                for (const auto& c : *comps)
                {
                    if (const auto* keys = c["keyframes"].getArray())
                    {
                        std::vector<std::pair<double, std::vector<float>>> frames;
                        for (const auto& k : *keys)
                        {
                            auto w = decodeFloats(k["wave_data"].toString());
                            if (w.size() == 2048) frames.push_back({ (double) k["position"], std::move(w) });
                        }
                        std::stable_sort(frames.begin(), frames.end(),
                                         [](const auto& a, const auto& b) { return a.first < b.first; });
                        for (auto& fr : frames) audio.insert(audio.end(), fr.second.begin(), fr.second.end());
                    }
                    if (audio.empty() && c.hasProperty("audio_file"))
                    {
                        audio = decodeFloats(c["audio_file"].toString());
                        frameLen = juce::jmax(16, (int) c.getProperty("window_size", 2048));
                    }
                    if (!audio.empty()) break;
                }
                if (!audio.empty()) break;
            }
            if (audio.empty()) { error = "no waves in .vitaltable"; return nullptr; }

            auto name = json["name"].toString();
            return Wavetable::fromSamples(audio, frameLen, name.isNotEmpty() ? name : f.getFileNameWithoutExtension());
        }
    }

    bool Wavetable::isSupportedFile(const juce::File& f)
    {
        return f.hasFileExtension("wav;aif;aiff;flac;wt;raw;f32;vitaltable");
    }

    std::unique_ptr<Wavetable> Wavetable::loadFile(const juce::File& f, juce::String& error)
    {
        std::unique_ptr<Wavetable> t;
        if (f.hasFileExtension("wt")) t = loadWt(f, error);
        else if (f.hasFileExtension("raw;f32")) t = loadRaw(f, error);
        else if (f.hasFileExtension("vitaltable")) t = loadVital(f, error);
        else t = loadAudio(f, error);

        if (t == nullptr) { if (error.isEmpty()) error = "no frames"; return nullptr; }
        t->file = f;
        return t;
    }
}
