// PolysixEngine.h
// JUCE-free DSP engine for Polytwelve.
// Signal flow:
//   Voice -> VCO (+ divider sub osc) -> VCF (4-pole) -> VCA, per-voice EG and MG,
//   12 voices summed to mono -> Effects (Chorus / Phaser / Ensemble) -> Volume.
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

namespace p6
{

constexpr int   kNumVoices = 12;
constexpr float kPi        = 3.14159265358979323846f;
constexpr float kTwoPi     = 2.0f * kPi;

inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float pitchToHz (float p)                  { return 440.0f * std::exp2 ((p - 69.0f) / 12.0f); }

// Logarithmic time units used by the envelopes: 0 = 1 ms, 20 = 10 ms, 40 = 100 ms, 60 = 1 s
inline float logTimeToSec (float v)        { return 0.001f * std::pow (10.0f, v / 20.0f); }

// ---------------------------------------------------------------------------------------------
struct Rng
{
    uint32_t s;
    explicit Rng (uint32_t seed = 0x1234567u) : s (seed ? seed : 1u) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni()     { return (float) (next() >> 8) * (1.0f / 16777216.0f); } // [0,1)
    float bi()      { return uni() * 2.0f - 1.0f; }                           // [-1,1)
};

// Slow random: random points at rate F, smoothly interpolated
struct SlowRandom
{
    Rng rng;
    float phase = 0, a = 0, b = 0;
    void init (uint32_t seed) { rng = Rng (seed); a = rng.bi(); b = rng.bi(); phase = rng.uni(); }
    float tick (float hz, float invSr)
    {
        phase += hz * invSr;
        if (phase >= 1.0f) { phase -= std::floor (phase); a = b; b = rng.bi(); }
        const float t = phase * phase * (3.0f - 2.0f * phase);
        return a + (b - a) * t;
    }
};

inline float polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt;           return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

inline float triFromPhase (float ph) // phase [0,1) -> triangle [-1,1]
{
    return 1.0f - 4.0f * std::fabs (ph - 0.5f) ;
}

// ---------------------------------------------------------------------------------------------
// Parameters, in "panel" units (mostly 0..1 like the 0..10 knobs on the original)
struct Params
{
    // master
    float volume    = 0.75f;  // 0..1
    float tune      = 0.0f;   // -1..+1 semitone
    // VCO
    int   octave    = 1;      // 0=16' 1=8' 2=4'
    int   waveform  = 0;      // 0=Saw 1=Pulse 2=PWM
    float pwPwm     = 0.0f;   // 0..1
    float pwmSpeed  = 0.4f;   // 0..1
    int   subOsc    = 0;      // 0=Off 1=1 Oct 2=2 Oct
    // VCF
    float cutoff    = 0.75f;  // 0..1
    float resonance = 0.0f;   // 0..1
    float egInt     = 0.0f;   // -1..+1
    float kbdTrack  = 0.5f;   // 0..1
    // VCA
    int   vcaMode   = 0;      // 0=EG 1=Gate
    float attenDb   = 0.0f;   // -10..+10 dB
    // EG
    float attack = 0.0f, decay = 0.5f, sustain = 0.7f, release = 0.5f;  // 0..1
    // MG
    float mgFreq = 0.4f, mgDelay = 0.0f, mgLevel = 0.0f;                 // 0..1
    int   mgMode = 0;          // 0=VCO 1=VCF 2=VCA
    // Voice
    int   voiceMode = 0;       // 0=Poly 1=Unison 2=Chord memory
    bool  hold      = false;
    float analog    = 0.3f;    // 0..1
    float detune    = 0.2f;    // 0..1
    float bendRange = 2.0f;    // 0..12 semitones
    float glide     = 0.0f;    // 0 = off .. 1 = 3 s (portamento time)
    bool  glideLegato = false; // glide only between overlapping notes
    // Arpeggiator
    bool  arpOn = false, arpLatch = false, arpSync = false;
    float arpSpeed = 0.5f;     // 0..1 (free running)
    int   arpRange = 0;        // 0=1 Oct 1=2 Oct 2=Full
    int   arpMode  = 0;        // 0=Up 1=Down 2=Up/Down
    int   arpDiv   = 3;        // index into kArpDivBeats
    // Effects
    int   fxMode  = 0;         // 0=Off 1=Chorus 2=Phaser 3=Ensemble
    float fxSpeed = 0.4f;      // 0..1 speed (chorus/phaser) or intensity (ensemble)
};

constexpr std::array<double, 6> kArpDivBeats { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 }; // 1/4 1/8 1/8T 1/16 1/16T 1/32

// MG frequency knob curve, from the Ctr.Shp breakpoints in the ensemble: 0->0.01, 25->1, 50->8, 75->15, 100->20 Hz
inline float mgKnobToHz (float k)
{
    static constexpr float xs[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
    static constexpr float ys[] = { 0.01f, 1.0f, 8.0f, 15.0f, 20.0f };
    k = clampf (k, 0.0f, 1.0f);
    for (int i = 0; i < 4; ++i)
        if (k <= xs[i + 1])
            return ys[i] + (ys[i + 1] - ys[i]) * (k - xs[i]) / (xs[i + 1] - xs[i]);
    return 20.0f;
}

// ---------------------------------------------------------------------------------------------
// VCO: band-limited saw / pulse, with a flip-flop divider sub oscillator (f/2 or f/4) locked to the main cycle
struct Vco
{
    float phase = 0.0f;
    int   cycle = 0; // main-cycle counter, mod 4

    float process (float hz, float invSr, int waveform, float pw, int subMode)
    {
        const float dt = clampf (hz * invSr, 1.0e-6f, 0.45f);
        phase += dt;
        if (phase >= 1.0f) { phase -= 1.0f; cycle = (cycle + 1) & 3; }

        float out;
        if (waveform == 0)
        {
            out = 2.0f * phase - 1.0f - polyBlep (phase, dt);
        }
        else
        {
            pw = clampf (pw, 0.03f, 0.97f);
            float v = phase < pw ? 1.0f : -1.0f;
            v += polyBlep (phase, dt);
            float t2 = phase + (1.0f - pw); if (t2 >= 1.0f) t2 -= 1.0f;
            v -= polyBlep (t2, dt);
            out = v - (2.0f * pw - 1.0f); // AC coupled
        }

        if (subMode > 0)
        {
            const int   ratio = subMode == 1 ? 2 : 4;
            const float sdt   = dt / (float) ratio;
            const float sph   = ((float) (cycle % ratio) + phase) / (float) ratio;
            float s = sph < 0.5f ? 1.0f : -1.0f;
            s += polyBlep (sph, sdt);
            float t2 = sph + 0.5f; if (t2 >= 1.0f) t2 -= 1.0f;
            s -= polyBlep (t2, sdt);
            out += s;
        }
        return out;
    }
};

// ---------------------------------------------------------------------------------------------
// VCF: 4-pole zero-delay-feedback ladder low-pass with saturating feedback path.
struct Ladder
{
    float s[4] = { 0, 0, 0, 0 };

    void reset() { s[0] = s[1] = s[2] = s[3] = 0.0f; }

    float process (float x, float g, float k)
    {
        const float G   = g / (1.0f + g);
        const float inv = 1.0f / (1.0f + g);
        const float S   = (G * G * G * s[0] + G * G * s[1] + G * s[2] + s[3]) * inv;
        const float G4  = G * G * G * G;
        float u = (x * (1.0f + 0.3f * k) - k * S) / (1.0f + k * G4);
        u = std::tanh (u);
        float y = u;
        for (int i = 0; i < 4; ++i)
        {
            const float v   = (y - s[i]) * G;
            const float out = v + s[i];
            s[i] = out + v;
            y = out;
        }
        return y;
    }
};

// ---------------------------------------------------------------------------------------------
// Analog-style ADSR (exponential segments, retriggers from current level)
struct Adsr
{
    enum Stage { Idle, Attack, Decay, Release } stage = Idle;
    float v = 0.0f;

    void gateOn()  { stage = Attack; }
    void gateOff() { if (stage != Idle) stage = Release; }
    bool idle() const { return stage == Idle; }

    static float coef (float sec, float sr) { return 1.0f - std::exp (-1.0f / (std::max (sec, 0.0005f) * sr)); }

    float tick (float aSec, float dSec, float sus, float rSec, float sr)
    {
        switch (stage)
        {
            case Attack:
                v += (1.3f - v) * coef (aSec / 1.466f, sr); // reaches 1.0 after aSec
                if (v >= 1.0f) { v = 1.0f; stage = Decay; }
                break;
            case Decay:
                v += (sus - v) * coef (dSec / 3.0f, sr);
                break;
            case Release:
                v += (0.0f - v) * coef (rSec / 3.0f, sr);
                if (v < 1.0e-5f) { v = 0.0f; stage = Idle; }
                break;
            case Idle: default: break;
        }
        return v;
    }
};

// ---------------------------------------------------------------------------------------------
struct Voice
{
    int      note   = -1;
    bool     gate   = false;
    bool     active = false;
    uint64_t stamp  = 0;     // when last triggered / released (for stealing)

    Vco    vco;
    Ladder vcf;
    Adsr   eg;
    float  gateSlew = 0.0f;  // VCA gate mode (slew-limited gate)
    float  pitchNow = 60.0f; // glided note pitch (semitones)
    float  mgPhase = 0.0f, mgEnv = 0.0f, pwmPhase = 0.0f;

    // fixed per-voice "component tolerances" (V/F/D/R Tune + Detune in the ensemble)
    float offPitch = 0, offFilter = 0, offDecay = 0, offRelease = 0, offDetune = 0;
    SlowRandom drift, mgRateRnd, pwmRateRnd;

    void init (uint32_t seed)
    {
        Rng r (seed);
        offPitch = r.bi(); offFilter = r.bi(); offDecay = r.bi(); offRelease = r.bi(); offDetune = r.bi();
        mgPhase = r.uni(); pwmPhase = r.uni(); vco.phase = r.uni();
        drift.init (seed * 3u + 1u); mgRateRnd.init (seed * 5u + 7u); pwmRateRnd.init (seed * 11u + 3u);
    }

    // fromPitch < 0: jump straight to the note; otherwise glide from that pitch
    void start (int n, uint64_t t, float fromPitch = -1.0f)
    {
        if (! active) { vcf.reset(); mgEnv = 0.0f; gateSlew = 0.0f; }
        pitchNow = fromPitch >= 0.0f ? fromPitch : (float) n;
        note = n; gate = true; active = true; stamp = t;
        eg.gateOn();
    }

    void stop (uint64_t t) { if (gate) { gate = false; stamp = t; eg.gateOff(); } }
};

// ---------------------------------------------------------------------------------------------
// Simple fractional delay line
struct DelayLine
{
    std::vector<float> buf;
    int mask = 0, w = 0;

    void prepare (int minSize)
    {
        int n = 1; while (n < minSize) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read (float delaySamples) const // linear interpolation, delay >= 1
    {
        float rp = (float) w - delaySamples;
        while (rp < 0.0f) rp += (float) (mask + 1);
        const int   i0 = (int) rp;
        const float f  = rp - (float) i0;
        const float a  = buf[(size_t) (i0 & mask)], b = buf[(size_t) ((i0 + 1) & mask)];
        return a + (b - a) * f;
    }
};

// Effects section: mono in, stereo out
struct Effects
{
    float sr = 48000.0f;
    DelayLine dl;
    float lfo = 0.0f, lfoFast = 0.0f;
    float lpL = 0.0f, lpR = 0.0f, lpC = 0.0f;
    std::array<float, 6> apL {}, apR {};
    float fbL = 0.0f, fbR = 0.0f;
    int lastMode = -1;

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        dl.prepare ((int) (0.08f * sr) + 8);
        reset();
    }
    void reset()
    {
        dl.clear(); lpL = lpR = lpC = 0.0f; fbL = fbR = 0.0f;
        apL.fill (0.0f); apR.fill (0.0f);
    }

    static float allpass (float x, float a, float& z) // first-order allpass
    {
        const float y = a * x + z;
        z = x - a * y;
        return y;
    }

    void process (float in, float& L, float& R, int mode, float knob)
    {
        if (mode != lastMode) { reset(); lastMode = mode; }
        const float invSr = 1.0f / sr;
        const float ms    = sr * 0.001f;
        const float bbdLp = 1.0f - std::exp (-kTwoPi * 9000.0f * invSr); // BBD-ish wet roll-off

        switch (mode)
        {
            case 1: // Chorus: knob = speed
            {
                const float rate = 0.1f * std::exp2 (knob * 5.3f); // 0.1 .. ~4 Hz
                lfo += rate * invSr; if (lfo >= 1.0f) lfo -= 1.0f;
                const float t = triFromPhase (lfo);
                dl.push (in);
                const float wl = dl.read ((6.0f + 2.5f * t) * ms);
                const float wr = dl.read ((6.0f - 2.5f * t) * ms);
                lpL += (wl - lpL) * bbdLp; lpR += (wr - lpR) * bbdLp;
                L = 0.7f * in + 0.7f * lpL;
                R = 0.7f * in + 0.7f * lpR;
                break;
            }
            case 2: // Phaser: knob = speed
            {
                const float rate = 0.05f * std::exp2 (knob * 7.0f); // 0.05 .. 6.4 Hz
                lfo += rate * invSr; if (lfo >= 1.0f) lfo -= 1.0f;
                float ph2 = lfo + 0.25f; if (ph2 >= 1.0f) ph2 -= 1.0f;
                auto coefFor = [&] (float tri)
                {
                    const float fc = 250.0f * std::exp2 (2.2f * (tri + 1.0f)); // 250 Hz .. ~5 kHz
                    const float tn = std::tan (kPi * std::min (fc, 0.45f * sr) * invSr);
                    return (tn - 1.0f) / (tn + 1.0f);
                };
                const float aL = coefFor (triFromPhase (lfo)), aR = coefFor (triFromPhase (ph2));
                float xl = in + 0.45f * fbL, xr = in + 0.45f * fbR;
                for (int i = 0; i < 6; ++i) { xl = allpass (xl, aL, apL[(size_t) i]); xr = allpass (xr, aR, apR[(size_t) i]); }
                fbL = xl; fbR = xr;
                L = 0.6f * (in + xl);
                R = 0.6f * (in + xr);
                break;
            }
            case 3: // Ensemble: knob = intensity
            {
                lfo     += 0.6f * invSr; if (lfo >= 1.0f) lfo -= 1.0f;
                lfoFast += 6.0f * invSr; if (lfoFast >= 1.0f) lfoFast -= 1.0f;
                dl.push (in);
                const float depth = 0.15f + 0.85f * knob;
                float taps[3];
                for (int i = 0; i < 3; ++i)
                {
                    const float off = (float) i / 3.0f;
                    const float s   = std::sin (kTwoPi * (lfo + off));
                    const float f   = std::sin (kTwoPi * (lfoFast + off));
                    taps[i] = dl.read ((10.0f + depth * (2.0f * s + 0.35f * f)) * ms);
                }
                const float wl = taps[0] + 0.5f * taps[1];
                const float wr = taps[2] + 0.5f * taps[1];
                lpL += (wl - lpL) * bbdLp; lpR += (wr - lpR) * bbdLp;
                L = 0.55f * in + 0.5f * lpL;
                R = 0.55f * in + 0.5f * lpR;
                break;
            }
            default:
                L = R = in;
                break;
        }
    }
};

// ---------------------------------------------------------------------------------------------
class Engine
{
public:
    Params p;

    Engine() { const int minor[] = { 0, 3, 7 }; setChord (minor, 3, 60); }

    // ---- chord memory -----------------------------------------------------------------------
    // Intervals above the lowest captured note, plus that note (for display). Captured by
    // requestChordCapture() (MEMORY button) or when switching to Chord mode with >= 2 keys held.
    // Atomics: written on the audio thread, read by the UI.
    void setChord (const int* intervals, int count, int root)
    {
        count = std::clamp (count, 1, kNumVoices);
        for (int i = 0; i < kNumVoices; ++i) chordInt[(size_t) i].store (i < count ? intervals[i] : 0, std::memory_order_relaxed);
        chordRoot.store (std::clamp (root, 0, 127), std::memory_order_relaxed);
        chordSize.store (count, std::memory_order_release);
        chordVersion.fetch_add (1, std::memory_order_release);
    }
    int getChord (int* intervalsOut, int& rootOut) const
    {
        const int n = std::clamp (chordSize.load (std::memory_order_acquire), 1, kNumVoices);
        for (int i = 0; i < n; ++i) intervalsOut[i] = chordInt[(size_t) i].load (std::memory_order_relaxed);
        rootOut = chordRoot.load (std::memory_order_relaxed);
        return n;
    }
    int  getChordVersion() const { return chordVersion.load (std::memory_order_acquire); }
    void requestChordCapture()   { captureRequest.store (true, std::memory_order_release); }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate; invSr = 1.0f / sr;
        for (int i = 0; i < kNumVoices; ++i) voices[(size_t) i].init (0x9E3779B9u * (uint32_t) (i + 1));
        fx.prepare (sr);
        smCutoff = p.cutoff; smRes = p.resonance; smVol = p.volume; smAtten = p.attenDb;
        dcX = dcY = 0.0f;
        resetPerformance();
    }

    // Host transport for arp sync (call once per block before rendering)
    void setTransport (bool playing, double bpm, double ppqAtBlockStart)
    {
        hostPlaying = playing; hostBpm = bpm > 1.0 ? bpm : 120.0; hostPpq = ppqAtBlockStart; blockSample = 0;
    }

    // ---- MIDI input -------------------------------------------------------------------------
    void noteOn (int note)
    {
        note = std::clamp (note, 0, 127);
        physDown[(size_t) note] = true;
        if (! logical[(size_t) note]) { logical[(size_t) note] = true; logicalOn (note); }
        else if (! p.arpOn) { playOff (note); playOn (note); } // re-strike a held note
    }

    void noteOff (int note)
    {
        note = std::clamp (note, 0, 127);
        physDown[(size_t) note] = false;
        if (holdActive()) return;
        if (logical[(size_t) note]) { logical[(size_t) note] = false; logicalOff (note); }
    }

    float getPitchBend() const       { return bend; }
    float getModWheel() const        { return mw; }
    void setSustainPedal (bool down) { pedal = down; }
    void pitchBend (float v)         { bend = clampf (v, -1.0f, 1.0f); }
    void modWheel (float v)          { mw = clampf (v, 0.0f, 1.0f); }

    void allNotesOff()
    {
        physDown.fill (false); logical.fill (false);
        arpNotes.clear(); arpStale = false;
        resetPerformance();
    }

    // ---- rendering --------------------------------------------------------------------------
    void render (float* outL, float* outR, int numSamples)
    {
        if (captureRequest.exchange (false, std::memory_order_acq_rel)) captureChord (1);
        checkModeChanges();

        for (int i = 0; i < numSamples; ++i)
        {
            const float sm = 1.0f - std::exp (-1.0f / (0.005f * sr));
            smCutoff += (p.cutoff - smCutoff) * sm;
            smRes    += (p.resonance - smRes) * sm;
            smVol    += (p.volume - smVol) * sm;
            smAtten  += (p.attenDb - smAtten) * sm;

            if (p.arpOn) tickArp();
            ++blockSample;

            float mono = 0.0f;
            for (auto& v : voices)
                if (v.active) mono += renderVoice (v);

            if (p.voiceMode == 1) mono *= 0.36f;     // unison stacks all twelve voices
            else if (p.voiceMode == 2) mono *= 0.8f; // chord memory

            // DC blocker
            const float y = mono - dcX + 0.9995f * dcY; dcX = mono; dcY = y;

            float L, R;
            fx.process (y, L, R, p.fxMode, p.fxSpeed);
            const float g = 0.8f * smVol * smVol;
            outL[i] = L * g;
            outR[i] = R * g;
            ++now;
        }
    }

    int activeVoiceCount() const { int n = 0; for (auto& v : voices) n += v.active ? 1 : 0; return n; }

private:
    float sr = 48000.0f, invSr = 1.0f / 48000.0f;
    std::array<Voice, kNumVoices> voices;
    Effects fx;
    uint64_t now = 0;
    int nextVoice = 0;

    float bend = 0.0f, mw = 0.0f;
    bool  pedal = false;
    float lastPlayed = -1.0f;        // pitch of the most recent note (poly glide source)
    float smCutoff = 0.75f, smRes = 0.0f, smVol = 0.75f, smAtten = 0.0f;
    float dcX = 0.0f, dcY = 0.0f;

    std::array<bool, 128> physDown {}, logical {};
    std::vector<int> monoStack;      // for Unison / Chord (last-note priority)
    int monoNote = -1;

    // arp
    std::vector<int> arpNotes;       // notes feeding the arp (sorted)
    bool   arpStale = false;         // latch: all keys released, next key starts a new set
    int    arpIndex = -1, arpPlaying = -1;
    double arpPhase = 0.0; long long arpStep = -1;
    bool   arpGate = false;
    bool   hostPlaying = false; double hostBpm = 120.0, hostPpq = 0.0; int blockSample = 0;

    int lastVoiceMode = -1; bool lastArpOn = false, lastHold = false;

    std::array<std::atomic<int>, kNumVoices> chordInt {};
    std::atomic<int>  chordSize { 3 }, chordRoot { 60 }, chordVersion { 0 };
    std::atomic<bool> captureRequest { false };

    // store the currently held keys (lowest first) if at least minKeys are down
    void captureChord (int minKeys)
    {
        int held[kNumVoices]; int n = 0;
        for (int k = 0; k < 128 && n < kNumVoices; ++k) if (logical[(size_t) k]) held[n++] = k;
        if (n < minKeys || n == 0) return;
        int iv[kNumVoices];
        for (int i = 0; i < n; ++i) iv[i] = held[i] - held[0];
        setChord (iv, n, held[0]);
    }

    bool holdActive() const { return p.hold || pedal; }

    // ---- logical key layer (after Hold) -------------------------------------------------------
    void logicalOn (int note)
    {
        if (p.arpOn)
        {
            if (arpStale) { arpNotes.clear(); arpStale = false; }
            if (std::find (arpNotes.begin(), arpNotes.end(), note) == arpNotes.end())
            {
                const bool wasEmpty = arpNotes.empty();
                arpNotes.push_back (note);
                std::sort (arpNotes.begin(), arpNotes.end());
                if (wasEmpty) { arpIndex = -1; arpPhase = 0.999999; }
            }
        }
        else playOn (note);
    }

    void logicalOff (int note)
    {
        if (p.arpOn)
        {
            if (p.arpLatch)
            {
                bool any = false; for (bool b : logical) any |= b;
                if (! any) arpStale = true;
            }
            else arpNotes.erase (std::remove (arpNotes.begin(), arpNotes.end(), note), arpNotes.end());
        }
        else playOff (note);
    }

    // ---- play layer (voice modes) -----------------------------------------------------------
    bool anyGateHeld() const { for (auto& v : voices) if (v.gate) return true; return false; }

    // should a new note glide? (Legato mode: only when it overlaps a held note)
    bool glideAllowed (bool overlapping) const { return p.glide > 0.0f && (! p.glideLegato || overlapping); }

    void playOn (int note)
    {
        if (p.voiceMode == 0)
        {
            const bool doGlide = glideAllowed (anyGateHeld()) && lastPlayed >= 0.0f;
            Voice& v = voices[(size_t) allocVoice (note)];
            v.start (note, ++now, doGlide ? lastPlayed : -1.0f);
        }
        else
        {
            const bool overlapping = ! monoStack.empty();
            monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
            monoStack.push_back (note);
            triggerMono (note, overlapping);
        }
        lastPlayed = (float) note;
    }

    void playOff (int note)
    {
        if (p.voiceMode == 0)
        {
            for (auto& v : voices) if (v.gate && v.note == note) v.stop (++now);
        }
        else
        {
            monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
            if (note == monoNote)
            {
                if (! monoStack.empty()) { triggerMono (monoStack.back(), true); lastPlayed = (float) monoStack.back(); }
                else { for (auto& v : voices) v.stop (++now); monoNote = -1; }
            }
        }
    }

    void triggerMono (int note, bool overlapping)
    {
        monoNote = note;
        const bool doGlide = glideAllowed (overlapping);
        // each voice glides from where it is; an idle voice from the last note played
        auto from = [&] (const Voice& v, int offset) -> float
        {
            if (! doGlide) return -1.0f;
            if (v.active) return v.pitchNow;
            return lastPlayed >= 0.0f ? lastPlayed + (float) offset : -1.0f;
        };
        if (p.voiceMode == 1) // unison: all twelve voices on one note
        {
            for (auto& v : voices) v.start (note, ++now, from (v, 0));
        }
        else // chord memory
        {
            const size_t n = (size_t) std::clamp (chordSize.load (std::memory_order_relaxed), 1, kNumVoices);
            for (size_t i = 0; i < (size_t) kNumVoices; ++i)
            {
                const int iv = chordInt[i].load (std::memory_order_relaxed);
                if (i < n) voices[i].start (std::clamp (note + iv, 0, 127), ++now, from (voices[i], iv));
                else       voices[i].stop (++now);
            }
        }
    }

    int allocVoice (int note)
    {
        for (int i = 0; i < kNumVoices; ++i) // same note already sounding -> reuse
            if (voices[(size_t) i].active && voices[(size_t) i].note == note) return i;
        for (int k = 0; k < kNumVoices; ++k) // round robin over free voices
        {
            const int i = (nextVoice + k) % kNumVoices;
            if (! voices[(size_t) i].active) { nextVoice = (i + 1) % kNumVoices; return i; }
        }
        int best = -1; uint64_t bestT = UINT64_MAX; // oldest released voice
        for (int i = 0; i < kNumVoices; ++i)
            if (! voices[(size_t) i].gate && voices[(size_t) i].stamp < bestT) { bestT = voices[(size_t) i].stamp; best = i; }
        if (best < 0) // all gated: steal oldest
            for (int i = 0; i < kNumVoices; ++i)
                if (voices[(size_t) i].stamp < bestT) { bestT = voices[(size_t) i].stamp; best = i; }
        nextVoice = (best + 1) % kNumVoices;
        return best;
    }

    void releaseAllVoices() { for (auto& v : voices) v.stop (++now); monoStack.clear(); monoNote = -1; }

    void resetPerformance()
    {
        lastPlayed = -1.0f;
        releaseAllVoices();
        arpIndex = -1; arpPlaying = -1; arpGate = false; arpPhase = 0.0; arpStep = -1;
        lastVoiceMode = p.voiceMode; lastArpOn = p.arpOn; lastHold = holdActive();
    }

    void checkModeChanges()
    {
        // Hold released -> drop latched keys that are no longer physically down
        const bool h = holdActive();
        if (lastHold && ! h)
            for (int n = 0; n < 128; ++n)
                if (logical[(size_t) n] && ! physDown[(size_t) n]) { logical[(size_t) n] = false; logicalOff (n); }
        lastHold = h;

        if (p.voiceMode != lastVoiceMode || p.arpOn != lastArpOn)
        {
            // Entering chord mode with >= 2 keys held memorises the chord
            if (p.voiceMode == 2 && lastVoiceMode != 2) captureChord (2);
            releaseAllVoices();
            arpNotes.clear(); arpStale = false; arpIndex = -1; arpPlaying = -1; arpGate = false;
            lastVoiceMode = p.voiceMode; lastArpOn = p.arpOn;
            for (int n = 0; n < 128; ++n) if (logical[(size_t) n]) logicalOn (n);
        }
    }

    // ---- arpeggiator ------------------------------------------------------------------------
    void buildSequence (std::vector<int>& seq) const
    {
        seq.clear();
        if (arpNotes.empty()) return;
        const int octs = p.arpRange == 0 ? 1 : (p.arpRange == 1 ? 2 : 4);
        std::vector<int> up;
        for (int o = 0; o < octs; ++o)
            for (int n : arpNotes) if (n + 12 * o <= 127) up.push_back (n + 12 * o);
        if (p.arpMode == 0) seq = up;
        else if (p.arpMode == 1) seq.assign (up.rbegin(), up.rend());
        else
        {
            seq = up;
            for (int i = (int) up.size() - 2; i >= 1; --i) seq.push_back (up[(size_t) i]);
        }
    }

    void tickArp()
    {
        if (arpStale && ! p.arpLatch) { arpNotes.clear(); arpStale = false; }

        bool stepNow = false; double phase;
        if (p.arpSync && hostPlaying)
        {
            const double div = kArpDivBeats[(size_t) std::clamp (p.arpDiv, 0, (int) kArpDivBeats.size() - 1)];
            const double ppq = hostPpq + (double) blockSample * hostBpm / 60.0 / (double) sr;
            const double pos = ppq / div;
            const long long step = (long long) std::floor (pos);
            phase = pos - (double) step;
            if (step != arpStep) { arpStep = step; stepNow = true; }
        }
        else
        {
            double rate;
            if (p.arpSync) rate = hostBpm / 60.0 / kArpDivBeats[(size_t) std::clamp (p.arpDiv, 0, (int) kArpDivBeats.size() - 1)];
            else           rate = 0.5 * std::exp2 (p.arpSpeed * 5.6); // 0.5 .. ~24 steps/s
            arpPhase += rate / (double) sr;
            if (arpPhase >= 1.0) { arpPhase -= std::floor (arpPhase); stepNow = true; }
            phase = arpPhase;
        }

        if (stepNow)
        {
            if (arpPlaying >= 0) { playOff (arpPlaying); arpPlaying = -1; }
            std::vector<int> seq; buildSequence (seq);
            if (! seq.empty())
            {
                arpIndex = (arpIndex + 1) % (int) seq.size();
                arpPlaying = seq[(size_t) arpIndex];
                playOn (arpPlaying);
                arpGate = true;
            }
        }
        else if (arpGate && phase >= 0.5) // 50% gate length
        {
            if (arpPlaying >= 0) { playOff (arpPlaying); arpPlaying = -1; }
            arpGate = false;
        }
    }

    // ---- one voice, one sample --------------------------------------------------------------
    float renderVoice (Voice& v)
    {
        const float an = p.analog;

        // MG (per voice, like the ensemble): triangle with delayed fade-in, rate wobble from "Analog"
        const float mgHz = mgKnobToHz (p.mgFreq) * std::exp2 (an * 0.15f * v.mgRateRnd.tick (1.0f, invSr));
        v.mgPhase += mgHz * invSr; if (v.mgPhase >= 1.0f) v.mgPhase -= 1.0f;
        const float tri = triFromPhase (v.mgPhase);
        if (v.gate) v.mgEnv = std::min (1.0f, v.mgEnv + invSr / logTimeToSec (p.mgDelay * 70.0f));
        else        v.mgEnv = std::max (0.0f, v.mgEnv - invSr / 0.001f);
        const float mg = tri * v.mgEnv * p.mgLevel * p.mgLevel;

        // pitch
        const float drift = v.drift.tick (0.3f, invSr);
        // glide (portamento): exponential approach, reaching the note in about the glide time
        if (p.glide > 0.0f && v.pitchNow != (float) v.note)
        {
            const float sec = 0.005f * std::pow (600.0f, p.glide); // 5 ms .. 3 s
            v.pitchNow += ((float) v.note - v.pitchNow) * (1.0f - std::exp (-4.0f * invSr / sec));
            if (std::fabs ((float) v.note - v.pitchNow) < 0.001f) v.pitchNow = (float) v.note;
        }
        else v.pitchNow = (float) v.note;

        const float voicePitch = v.pitchNow + p.tune + bend * p.bendRange
                               + p.detune * 0.25f * v.offDetune
                               + an * (0.12f * v.offPitch + 0.08f * drift);
        float oscPitch = voicePitch + (float) ((p.octave - 1) * 12) + tri * mw * 2.0f;
        if (p.mgMode == 0) oscPitch += mg * 12.0f;

        // PW / PWM
        float pw = 0.5f;
        if (p.waveform == 1) pw = 0.5f + 0.45f * p.pwPwm;
        else if (p.waveform == 2)
        {
            const float hz = 0.1f * std::exp2 (p.pwmSpeed * 7.0f) * std::exp2 (an * 0.2f * v.pwmRateRnd.tick (0.7f, invSr));
            v.pwmPhase += hz * invSr; if (v.pwmPhase >= 1.0f) v.pwmPhase -= 1.0f;
            pw = 0.5f + 0.45f * p.pwPwm * triFromPhase (v.pwmPhase);
        }
        const float osc = 0.55f * v.vco.process (pitchToHz (oscPitch), invSr, p.waveform, pw, p.subOsc);

        // EG
        const float aSec = logTimeToSec (p.attack * 80.0f);
        const float dSec = logTimeToSec (p.decay * 80.0f + an * 3.0f * v.offDecay);
        const float rSec = logTimeToSec (p.release * 80.0f + an * 3.0f * v.offRelease);
        const float eg = v.eg.tick (aSec, dSec, p.sustain, rSec, sr);

        // VCF
        float cut = 21.0f + smCutoff * 110.0f
                  + (voicePitch - 60.0f) * p.kbdTrack
                  + eg * p.egInt * 60.0f
                  + an * 2.0f * v.offFilter;
        if (p.mgMode == 1) cut += mg * 48.0f;
        const float fc = std::min (pitchToHz (cut), 0.45f * sr);
        const float g  = std::tan (kPi * fc * invSr);
        const float filtered = v.vcf.process (osc, g, 4.2f * smRes);

        // VCA
        const float step = 1000.0f * invSr; // slew-limited gate, as in the ensemble
        v.gateSlew = v.gate ? std::min (1.0f, v.gateSlew + step) : std::max (0.0f, v.gateSlew - step);
        float amp = p.vcaMode == 0 ? eg : v.gateSlew;
        if (p.mgMode == 2) amp -= mg;
        amp = clampf (amp, 0.0f, 2.0f);
        const float out = filtered * amp * std::pow (10.0f, smAtten / 20.0f);

        // free the voice when silent
        if (! v.gate && (p.vcaMode == 0 ? v.eg.idle() : (v.gateSlew <= 0.0f && v.eg.idle())))
        { v.active = false; v.note = -1; }
        if (! v.gate && p.vcaMode == 1 && v.gateSlew <= 0.0f) { v.active = false; v.note = -1; v.eg.stage = Adsr::Idle; v.eg.v = 0.0f; }

        return out;
    }
};

} // namespace p6
