// LoopRender — plays one note over and over through the installed Corpuscular.vst3, the way a DAW plays a looped
// clip (host position jumping back to the loop start, blocks of 1024), with no sound card: writes the WAVs and
// prints how loud each repeat starts against the steady note. Usage:
//   LoopRender.exe <out folder> [plugin.vst3]

#include <JuceHeader.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
    constexpr double kRate = 48000.0;
    constexpr int kBlock = 1024;
    constexpr double kBpm = 120.0;
    constexpr int kNote = 36;          // C1 in Cubase
    constexpr int kVelocity = 100;
    constexpr int kLoop = 96000;       // one bar at 120 BPM
    constexpr int kRepeats = 6;

    enum class Seam { offThenOn, onThenOff, allNotesOff, allSoundOff };

    struct Head : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override { return info; }
        PositionInfo info;
    };

    void place (Head& head, juce::int64 loopPosition)
    {
        const double ppq = (double) loopPosition / kRate * kBpm / 60.0;
        head.info = {};
        head.info.setBpm (kBpm);
        head.info.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
        head.info.setIsPlaying (true);
        head.info.setIsLooping (true);
        head.info.setLoopPoints (juce::AudioPlayHead::LoopPoints { 0.0, 4.0 });
        head.info.setTimeInSamples (loopPosition);
        head.info.setTimeInSeconds ((double) loopPosition / kRate);
        head.info.setPpqPosition (ppq);
        head.info.setPpqPositionOfLastBarStart (0.0);
        head.info.setBarCount (0);
    }

    double rms (const std::vector<float>& x, int from, int n)
    {
        double s = 0.0;
        for (int i = from; i < from + n; ++i)
            s += (double) x[(size_t) i] * x[(size_t) i];
        return std::sqrt (s / n);
    }

    double peak (const std::vector<float>& x, int from, int n)
    {
        double p = 0.0;
        for (int i = from; i < from + n; ++i)
            p = std::max (p, (double) std::abs (x[(size_t) i]));
        return p;
    }

    // the loudest bit of the changes from sample to sample: what a click is
    double edge (const std::vector<float>& x, int from, int n)
    {
        const int w = (int) (kRate / 500);
        double best = 0.0;
        for (int i = std::max (1, from); i + w < from + n; i += w / 2)
        {
            double s = 0.0;
            for (int j = i; j < i + w; ++j)
            {
                const double d = (double) x[(size_t) j] - x[(size_t) j - 1];
                s += d * d;
            }
            best = std::max (best, std::sqrt (s / w));
        }
        return best;
    }

    struct Metrics { double over = 0, loud = 0, quiet = 1e9, click = 0; };

    Metrics measure (const std::vector<float>& x, int point)
    {
        const int steady = point + kLoop / 2;
        const int window = (int) (kRate / 5);
        const double sp = std::max (peak (x, steady, window), 1e-9);
        const double sr = std::max (rms (x, steady, window), 1e-9);
        const double se = std::max (edge (x, steady, window), 1e-9);

        Metrics m;
        m.over = peak (x, point, window) / sp;
        m.click = edge (x, point - (int) (kRate / 100), (int) (kRate / 20)) / se;
        const int period = (int) std::ceil (kRate / 65.4);   // one period of C1
        for (int i = point + (int) (kRate / 200); i < point + window; i += (int) (kRate / 1000))
        {
            const double r = rms (x, i, period) / sr;
            m.loud = std::max (m.loud, r);
            m.quiet = std::min (m.quiet, r);
        }
        return m;
    }

    bool setVoices (juce::AudioPluginInstance& p, int voices)
    {
        for (auto* param : p.getParameters())
        {
            const auto name = param->getName (64);
            if (name.equalsIgnoreCase ("Voices") || name.equalsIgnoreCase ("Polyphony"))
            {
                for (int i = 0; i <= 1000; ++i)
                {
                    const float v = (float) i / 1000.0f;
                    if (param->getText (v, 32).getIntValue() == voices)
                    {
                        param->setValueNotifyingHost (v);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    std::vector<float> render (juce::AudioPluginFormatManager& formats, const juce::PluginDescription& d,
                               int voices, Seam seam, juce::String& error)
    {
        auto p = formats.createPluginInstance (d, kRate, kBlock, error);
        if (p == nullptr)
            return {};
        auto layout = p->getBusesLayout();
        if (! layout.outputBuses.isEmpty())
            layout.outputBuses.getReference (0) = juce::AudioChannelSet::stereo();
        p->setBusesLayout (layout);
        Head head;
        p->setPlayHead (&head);
        p->prepareToPlay (kRate, kBlock);
        if (! setVoices (*p, voices))
            std::cout << "  (no Voices parameter found)" << std::endl;

        juce::AudioBuffer<float> buffer (std::max (2, p->getTotalNumOutputChannels()), kBlock);
        juce::MidiBuffer midi;

        // a second of silence first: the parameter settles
        juce::int64 song = 0;
        for (int i = 0; i < (int) kRate; i += kBlock)
        {
            place (head, 0);
            buffer.clear();
            midi.clear();
            p->processBlock (buffer, midi);
        }

        const juce::int64 total = (juce::int64) kLoop * kRepeats;
        std::vector<float> out ((size_t) (total + kBlock), 0.0f);
        bool first = true;
        for (song = 0; song < total; song += kBlock)
        {
            place (head, song % kLoop);
            midi.clear();
            for (int i = 0; i < kBlock; ++i)
            {
                const auto at = song + i;
                if (at % kLoop != 0)
                    continue;
                if (first)
                {
                    midi.addEvent (juce::MidiMessage::noteOn (1, kNote, (juce::uint8) kVelocity), i);
                    first = false;
                    continue;
                }
                const auto off = juce::MidiMessage::noteOff (1, kNote);
                const auto on = juce::MidiMessage::noteOn (1, kNote, (juce::uint8) kVelocity);
                switch (seam)
                {
                    case Seam::offThenOn:   midi.addEvent (off, i); midi.addEvent (on, i); break;
                    case Seam::onThenOff:   midi.addEvent (on, i); midi.addEvent (off, i); break;
                    case Seam::allNotesOff: midi.addEvent (off, i); midi.addEvent (juce::MidiMessage::allNotesOff (1), i);
                                            midi.addEvent (on, i); break;
                    case Seam::allSoundOff: midi.addEvent (off, i); midi.addEvent (juce::MidiMessage::allSoundOff (1), i);
                                            midi.addEvent (on, i); break;
                }
            }
            buffer.clear();
            p->processBlock (buffer, midi);
            for (int i = 0; i < kBlock; ++i)
                out[(size_t) (song + i)] = buffer.getSample (0, i);
        }
        p->releaseResources();
        out.resize ((size_t) total);
        return out;
    }

    void writeWav (const juce::File& file, const std::vector<float>& x)
    {
        file.deleteFile();
        auto stream = file.createOutputStream();
        if (stream == nullptr)
            return;
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (stream.get(), kRate, 1, 24, {}, 0));
        if (w == nullptr)
            return;
        stream.release();
        const float* ch[1] = { x.data() };
        w->writeFromFloatArrays (ch, 1, (int) x.size());
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File folder = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "loop_render");
    const juce::String plugin = argc > 2 ? juce::String (juce::CharPointer_UTF8 (argv[2]))
                                         : juce::String ("C:\\Program Files\\Common Files\\VST3\\Corpuscular.vst3");
    folder.createDirectory();

    juce::AudioPluginFormatManager formats;
    juce::addDefaultFormatsToManager (formats);
    juce::OwnedArray<juce::PluginDescription> found;
    for (auto* f : formats.getFormats())
        if (f->fileMightContainThisPluginType (plugin))
            f->findAllTypesForFile (found, plugin);
    if (found.isEmpty())
    {
        std::cout << "No plugin in " << plugin << std::endl;
        return 1;
    }

    struct Case { int voices; Seam seam; const char* name; };
    const Case cases[] = {
        { 1, Seam::offThenOn, "v1_off_on" },
        { 1, Seam::onThenOff, "v1_on_off" },
        { 1, Seam::allNotesOff, "v1_allnotesoff" },
        { 1, Seam::allSoundOff, "v1_allsoundoff" },
        { 8, Seam::offThenOn, "v8_off_on" },
    };

    std::cout << "case             first: over loud click | repeats (worst): over loud quiet click" << std::endl;
    for (const auto& c : cases)
    {
        juce::String error;
        auto x = render (formats, *found[0], c.voices, c.seam, error);
        if (x.empty())
        {
            std::cout << c.name << ": " << error << std::endl;
            continue;
        }
        writeWav (folder.getChildFile (juce::String (c.name) + ".wav"), x);
        const auto f = measure (x, 0);
        Metrics w;
        for (int k = 1; k < kRepeats - 1; ++k)
        {
            const auto m = measure (x, k * kLoop);
            w.over = std::max (w.over, m.over);
            w.loud = std::max (w.loud, m.loud);
            w.quiet = std::min (w.quiet, m.quiet);
            w.click = std::max (w.click, m.click);
        }
        std::cout << juce::String (c.name).paddedRight (' ', 17)
                  << juce::String (f.over, 2) << " " << juce::String (f.loud, 2) << " " << juce::String (f.click, 2)
                  << "   |   " << juce::String (w.over, 2) << " " << juce::String (w.loud, 2) << " "
                  << juce::String (w.quiet, 2) << " " << juce::String (w.click, 2) << std::endl;
    }
    return 0;
}
