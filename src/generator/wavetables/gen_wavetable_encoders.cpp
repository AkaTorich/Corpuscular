/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_wavetable_encoders.h"

#include "../gen_random.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace corpuscular {
namespace gen {

  namespace WavetableEncoders {
    typedef std::vector<uint8_t> EncoderBytes;

    static void encoderPutU16(EncoderBytes& out, uint16_t value) {
      out.push_back(static_cast<uint8_t>(value & 0xff));
      out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    }

    static void encoderPutU32(EncoderBytes& out, uint32_t value) {
      for (int i = 0; i < 4; ++i)
        out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xff));
    }

    static void encoderPutFloat(EncoderBytes& out, float value) {
      uint32_t bits = 0;
      std::memcpy(&bits, &value, sizeof(bits));
      encoderPutU32(out, bits);
    }

    static void encoderPutText(EncoderBytes& out, const std::string& text) {
      out.insert(out.end(), text.begin(), text.end());
    }

    static void encoderPutBytes(EncoderBytes& out, const EncoderBytes& bytes) {
      out.insert(out.end(), bytes.begin(), bytes.end());
    }

    // 16-bit sample, as Math.Round(clamp(v) * 32767f)
    static int16_t encoderToInt16(float value) {
      float clamped = std::max(-1.0f, std::min(1.0f, value));
      float scaled = clamped * 32767.0f;
      return static_cast<int16_t>(roundHalfEven(static_cast<double>(scaled)));
    }

    static float encoderSampleAt(const std::vector<float>& frame, int index) {
      return index < static_cast<int>(frame.size()) ? frame[index] : 0.0f;
    }

    // RIFF chunk: id, size, payload padded to even length
    static EncoderBytes encoderChunk(const char* id, const EncoderBytes& payload) {
      EncoderBytes chunk;
      chunk.reserve(8 + payload.size() + 1);
      chunk.insert(chunk.end(), id, id + 4);
      encoderPutU32(chunk, static_cast<uint32_t>(payload.size()));
      encoderPutBytes(chunk, payload);
      if (payload.size() & 1)
        chunk.push_back(0);
      return chunk;
    }

    // Printable ASCII only; empty becomes the program name
    static std::string encoderSafe(const std::string& text) {
      bool blank = true;
      for (char c : text) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
          blank = false;
      }
      if (blank)
        return "WavetableGenerator";

      std::string result;
      for (unsigned char c : text)
        result.push_back(c < 128 && c >= 32 ? static_cast<char>(c) : '_');
      return result;
    }

    static EncoderBytes encoderZeroTerminated(const std::string& text) {
      EncoderBytes bytes(text.begin(), text.end());
      bytes.push_back(0);
      return bytes;
    }

    static EncoderBytes encoderFmtChunk(bool float32, int bytes_per_sample) {
      EncoderBytes payload;
      encoderPutU16(payload, static_cast<uint16_t>(float32 ? 3 : 1));    // IEEE float / PCM
      encoderPutU16(payload, 1);                                         // mono
      encoderPutU32(payload, 44100);
      encoderPutU32(payload, static_cast<uint32_t>(44100 * bytes_per_sample));
      encoderPutU16(payload, static_cast<uint16_t>(bytes_per_sample));
      encoderPutU16(payload, static_cast<uint16_t>(bytes_per_sample * 8));
      return encoderChunk("fmt ", payload);
    }

    static EncoderBytes encoderInfoChunk(const std::string& author, const std::string& name,
                                         const std::string& type_name, int frame_count, int frame_size) {
      EncoderBytes payload;
      encoderPutText(payload, "INFO");
      encoderPutBytes(payload, encoderChunk("IART", encoderZeroTerminated(encoderSafe(author))));
      encoderPutBytes(payload, encoderChunk("ISFT", encoderZeroTerminated("WavetableGenerator")));
      encoderPutBytes(payload, encoderChunk("INAM", encoderZeroTerminated(encoderSafe(name))));
      std::string comment = type_name + " " + std::to_string(frame_count) + "x" + std::to_string(frame_size);
      encoderPutBytes(payload, encoderChunk("ICMT", encoderZeroTerminated(comment)));
      return encoderChunk("LIST", payload);
    }

    EncoderBytes wav(const std::vector<std::vector<float>>& frames, int frame_size, bool float32, bool clm_chunk,
                     const std::string& author, const std::string& name, const std::string& type_name) {
      int frame_count = static_cast<int>(frames.size());
      int bytes_per_sample = float32 ? 4 : 2;

      EncoderBytes data;
      data.reserve(static_cast<size_t>(frame_size) * frame_count * bytes_per_sample);
      for (const std::vector<float>& frame : frames) {
        for (int i = 0; i < frame_size; ++i) {
          float value = encoderSampleAt(frame, i);
          if (float32)
            encoderPutFloat(data, value);
          else
            encoderPutU16(data, static_cast<uint16_t>(encoderToInt16(value)));
        }
      }

      EncoderBytes clm;
      if (clm_chunk) {
        char size_text[16];
        std::snprintf(size_text, sizeof(size_text), "%04d", frame_size);
        std::string text = std::string("<!>") + size_text + " 10000000 wavetable (" + encoderSafe(author) + ")";
        clm = encoderChunk("clm ", EncoderBytes(text.begin(), text.end()));
      }
      EncoderBytes info = encoderInfoChunk(author, name, type_name, frame_count, frame_size);
      EncoderBytes fmt = encoderFmtChunk(float32, bytes_per_sample);
      EncoderBytes data_chunk = encoderChunk("data", data);

      EncoderBytes out;
      uint32_t riff_size = static_cast<uint32_t>(4 + fmt.size() + clm.size() + info.size() + data_chunk.size());
      out.reserve(8 + riff_size);
      encoderPutText(out, "RIFF");
      encoderPutU32(out, riff_size);
      encoderPutText(out, "WAVE");
      encoderPutBytes(out, fmt);
      encoderPutBytes(out, clm);
      encoderPutBytes(out, info);
      encoderPutBytes(out, data_chunk);
      return out;
    }

    EncoderBytes wt(const std::vector<std::vector<float>>& frames, int frame_size) {
      const uint16_t kFlagInt16 = 4;
      const uint16_t kFlagInt16FullRange = 8;

      EncoderBytes out;
      out.reserve(12 + static_cast<size_t>(frame_size) * frames.size() * 2);
      encoderPutText(out, "vawt");
      encoderPutU32(out, static_cast<uint32_t>(frame_size));
      encoderPutU16(out, static_cast<uint16_t>(frames.size()));
      encoderPutU16(out, static_cast<uint16_t>(kFlagInt16 | kFlagInt16FullRange));
      for (const std::vector<float>& frame : frames) {
        for (int i = 0; i < frame_size; ++i)
          encoderPutU16(out, static_cast<uint16_t>(encoderToInt16(encoderSampleAt(frame, i))));
      }
      return out;
    }

    EncoderBytes raw(const std::vector<std::vector<float>>& frames, int frame_size) {
      EncoderBytes out;
      out.reserve(static_cast<size_t>(frame_size) * frames.size() * 4);
      for (const std::vector<float>& frame : frames) {
        for (int i = 0; i < frame_size; ++i)
          encoderPutFloat(out, encoderSampleAt(frame, i));
      }
      return out;
    }

    std::vector<float> resampleFrame(const std::vector<float>& frame, int size) {
      int source_size = static_cast<int>(frame.size());
      if (source_size == size)
        return frame;
      std::vector<float> result(size, 0.0f);
      if (source_size == 0)
        return result;

      for (int i = 0; i < size; ++i) {
        double position = static_cast<double>(i) * source_size / size;
        int index = static_cast<int>(position);
        double t = position - index;
        float a = frame[index % source_size];
        float b = frame[(index + 1) % source_size];
        result[i] = static_cast<float>(a + (b - a) * t);
      }
      return result;
    }
  }

} // namespace gen
} // namespace corpuscular
