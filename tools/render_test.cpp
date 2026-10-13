// Offline test harness for the engine (no JUCE). Renders WAVs and prints sanity metrics.
// Build: g++ -O2 -std=c++17 -I../Source tools/render_test.cpp -o render_test
#include "dsp/PolysixEngine.h"
#include <chrono>
#include <cstdio>
#include <functional>
#include <string>

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) std::lrint (p6::clampf (L[i], -1.f, 1.f) * 32767.f);
        int16_t b = (int16_t) std::lrint (p6::clampf (R[i], -1.f, 1.f) * 32767.f);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

struct Ev { double t; std::function<void (p6::Engine&)> fn; };

static void run (const std::string& name, double seconds, std::function<void (p6::Params&)> setup, std::vector<Ev> evs,
                 int sr = 48000, bool transport = false)
{
    p6::Engine e; setup (e.p); e.prepare (sr);
    const int total = (int) (seconds * sr), block = 128;
    std::vector<float> L ((size_t) total), R ((size_t) total);
    size_t ei = 0; double ppq = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        while (ei < evs.size() && evs[ei].t * sr <= pos) { evs[ei].fn (e); ++ei; }
        if (transport) e.setTransport (true, 128.0, ppq);
        e.render (&L[(size_t) pos], &R[(size_t) pos], n);
        ppq += n * 128.0 / 60.0 / sr;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    float peak = 0; double rms = 0; int bad = 0;
    for (int i = 0; i < total; ++i)
    {
        if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) ++bad;
        peak = std::max ({ peak, std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i]) });
        rms += L[(size_t) i] * L[(size_t) i];
    }
    const double cpu = std::chrono::duration<double> (t1 - t0).count() / seconds * 100.0;
    std::printf ("%-18s peak %6.3f  rms %6.4f  nonfinite %d  cpu %.2f%% of realtime\n",
                 name.c_str(), peak, std::sqrt (rms / total), bad, cpu);
    writeWav ("out_" + name + ".wav", L, R, sr);
}

// estimate fundamental of a steady segment by autocorrelation
static double estimatePitch (int note, std::function<void (p6::Params&)> setup)
{
    const int sr = 48000;
    p6::Engine e; setup (e.p); e.prepare (sr);
    e.noteOn (note);
    std::vector<float> L (sr), R (sr);
    e.render (L.data(), R.data(), sr);
    const int start = sr / 2, len = 4096, maxLag = 4000;
    // YIN difference function, first dip below threshold
    std::vector<double> d ((size_t) maxLag, 0.0), cm ((size_t) maxLag, 1.0);
    double run = 0;
    for (int lag = 1; lag < maxLag; ++lag)
    {
        double s = 0;
        for (int i = 0; i < len; ++i) { const double x = L[(size_t) (start + i)] - L[(size_t) (start + i + lag)]; s += x * x; }
        d[(size_t) lag] = s; run += s; cm[(size_t) lag] = s * lag / run;
    }
    int lag = 2;
    while (lag < maxLag - 1 && cm[(size_t) lag] > 0.1) ++lag;
    while (lag < maxLag - 1 && cm[(size_t) lag + 1] < cm[(size_t) lag]) ++lag;
    const double a = cm[(size_t) lag - 1], b = cm[(size_t) lag], c = cm[(size_t) lag + 1];
    const double refined = lag + 0.5 * (a - c) / (a - 2 * b + c);
    return sr / refined;
}

int main()
{
    using P = p6::Params;
    auto clean = [] (P& p) { p.analog = 0; p.detune = 0; p.cutoff = 1.0f; p.fxMode = 0; };

    for (int note : { 45, 57, 69, 81 })
    {
        const double f = estimatePitch (note, clean), expect = p6::pitchToHz ((float) note);
        std::printf ("pitch note %d: %.2f Hz (expected %.2f, %+.1f cents)\n", note, f, expect, 1200 * std::log2 (f / expect));
    }

    auto chordEvents = [] (double off) {
        return std::vector<Ev> { { 0.0, [] (p6::Engine& e) { e.noteOn (48); e.noteOn (55); e.noteOn (60); e.noteOn (63); } },
                                 { off, [] (p6::Engine& e) { e.noteOff (48); e.noteOff (55); e.noteOff (60); e.noteOff (63); } } };
    };

    run ("saw_chord", 4, [] (P& p) { p.cutoff = 0.6f; p.egInt = 0.3f; }, chordEvents (2.5));
    run ("sub_bass", 3, [] (P& p) { p.octave = 0; p.waveform = 1; p.subOsc = 2; p.cutoff = 0.35f; p.resonance = 0.4f; p.egInt = 0.5f;
                                      p.decay = 0.4f; p.sustain = 0.2f; },
         { { 0.0, [] (p6::Engine& e) { e.noteOn (36); } }, { 0.4, [] (p6::Engine& e) { e.noteOff (36); } },
           { 0.5, [] (p6::Engine& e) { e.noteOn (36); } }, { 0.9, [] (p6::Engine& e) { e.noteOff (36); } },
           { 1.0, [] (p6::Engine& e) { e.noteOn (39); } }, { 1.8, [] (p6::Engine& e) { e.noteOff (39); } } });
    run ("res_selfosc", 3, [] (P& p) { p.cutoff = 0.5f; p.resonance = 1.0f; p.egInt = 0.6f; p.decay = 0.6f; p.sustain = 0.0f; }, chordEvents (2.0));
    run ("pwm_ensemble", 5, [] (P& p) { p.waveform = 2; p.pwPwm = 0.8f; p.pwmSpeed = 0.35f; p.fxMode = 3; p.fxSpeed = 0.7f;
                                          p.attack = 0.55f; p.release = 0.6f; p.cutoff = 0.65f; }, chordEvents (3.5));
    run ("unison_lead", 3, [] (P& p) { p.voiceMode = 1; p.detune = 0.6f; p.cutoff = 0.7f; p.resonance = 0.3f; p.mgLevel = 0.4f; p.mgDelay = 0.6f; p.mgFreq = 0.4f; },
         { { 0.0, [] (p6::Engine& e) { e.noteOn (64); } }, { 1.0, [] (p6::Engine& e) { e.noteOn (67); } },
           { 1.5, [] (p6::Engine& e) { e.noteOff (67); } }, { 2.2, [] (p6::Engine& e) { e.noteOff (64); } } });
    run ("chord_memory", 3, [] (P& p) { p.voiceMode = 2; p.fxMode = 1; },
         { { 0.0, [] (p6::Engine& e) { e.noteOn (57); } }, { 0.6, [] (p6::Engine& e) { e.noteOff (57); } },
           { 0.8, [] (p6::Engine& e) { e.noteOn (53); } }, { 1.4, [] (p6::Engine& e) { e.noteOff (53); } } });
    run ("arp_free", 4, [] (P& p) { p.arpOn = true; p.arpSpeed = 0.6f; p.arpRange = 1; p.arpMode = 2; p.resonance = 0.6f; p.cutoff = 0.45f; p.egInt = 0.4f;
                                      p.decay = 0.3f; p.sustain = 0.1f; p.fxMode = 2; }, chordEvents (3.0));
    run ("arp_sync_latch", 4, [] (P& p) { p.arpOn = true; p.arpSync = true; p.arpDiv = 3; p.arpLatch = true; p.cutoff = 0.5f; p.egInt = 0.4f;
                                            p.decay = 0.3f; p.sustain = 0.1f; }, chordEvents (0.5), 48000, true);
    run ("six_voice_stress", 4, [] (P& p) { p.fxMode = 3; p.resonance = 0.7f; p.mgLevel = 0.5f; p.mgMode = 1; },
         { { 0.0, [] (p6::Engine& e) { for (int n : { 36, 48, 55, 60, 63, 67, 70, 74 }) e.noteOn (n); } },
           { 3.0, [] (p6::Engine& e) { e.allNotesOff(); } } });
    return 0;
}
