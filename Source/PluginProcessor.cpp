#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;

juce::String fmtTime (float sec)
{
    return sec < 1.0f ? juce::String (sec * 1000.0f, sec < 0.01f ? 1 : 0) + " ms"
                      : juce::String (sec, 2) + " s";
}

// 0..1 knob shown as 0.0 .. 10.0, like the panel
std::unique_ptr<APF> knob (const char* id, const char* name, float def)
{
    return std::make_unique<APF> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.0f, 1.0f), def,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v * 10.0f, 1); }));
}

std::unique_ptr<APF> custom (const char* id, const char* name, float lo, float hi, float def,
                             std::function<juce::String (float, int)> fmt, float step = 0.0f)
{
    return std::make_unique<APF> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (lo, hi, step), def,
                                  juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (fmt)));
}

std::unique_ptr<APC> choice (const char* id, const char* name, juce::StringArray items, int def)
{
    return std::make_unique<APC> (juce::ParameterID { id, 1 }, name, items, def);
}

std::unique_ptr<APB> toggle (const juce::String& id, const juce::String& name, bool def)
{
    return std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def);
}

// ---- factory programs ------------------------------------------------------------------------
struct Program { const char* name; std::vector<std::pair<const char*, float>> values; };

const std::vector<Program>& programs()
{
    static const std::vector<Program> list {
        { "Init", {} },
        { "Strings", { { "waveform", 0 }, { "cutoff", 0.55f }, { "egInt", 0.15f }, { "attack", 0.6f }, { "decay", 0.6f },
                       { "sustain", 0.9f }, { "release", 0.62f }, { "fxMode", 3 }, { "fxSpeed", 0.6f }, { "analog", 0.35f } } },
        { "Brass", { { "waveform", 0 }, { "cutoff", 0.35f }, { "resonance", 0.15f }, { "egInt", 0.45f }, { "kbdTrack", 0.6f },
                     { "attack", 0.45f }, { "decay", 0.55f }, { "sustain", 0.7f }, { "release", 0.45f } } },
        { "Bass", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 0.3f }, { "subOsc", 1 }, { "cutoff", 0.3f }, { "resonance", 0.35f },
                    { "egInt", 0.5f }, { "attack", 0 }, { "decay", 0.45f }, { "sustain", 0.25f }, { "release", 0.3f } } },
        { "Square Lead", { { "waveform", 1 }, { "pwPwm", 0 }, { "voiceMode", 1 }, { "detune", 0.45f }, { "cutoff", 0.6f },
                           { "resonance", 0.25f }, { "egInt", 0.2f }, { "mgDelay", 0.6f }, { "mgLevel", 0.35f }, { "mgFreq", 0.42f }, { "glide", 0.35f }, { "glideLegato", 1 },
                           { "fxMode", 1 }, { "fxSpeed", 0.3f } } },
        { "Rezzy Arp", { { "waveform", 0 }, { "cutoff", 0.42f }, { "resonance", 0.75f }, { "egInt", 0.5f }, { "decay", 0.35f },
                         { "sustain", 0 }, { "release", 0.3f }, { "arpOn", 1 }, { "arpSync", 1 }, { "arpDiv", 3 }, { "arpRange", 1 },
                         { "arpMode", 2 }, { "fxMode", 2 }, { "fxSpeed", 0.3f } } },
        { "Filter Sweep", { { "waveform", 2 }, { "pwPwm", 0.7f }, { "pwmSpeed", 0.3f }, { "cutoff", 0.32f }, { "resonance", 0.6f },
                            { "mgMode", 1 }, { "mgLevel", 0.55f }, { "mgFreq", 0.18f }, { "attack", 0.5f }, { "release", 0.6f },
                            { "fxMode", 3 }, { "fxSpeed", 0.5f } } },
        // ---- Korg Polysix factory setting charts (both sets, A-1 to D-8) ----
        { "Brass 2", { { "octave", 0 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 0.2f }, { "egInt", 0.24f }, { "kbdTrack", 0.7f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.2f }, { "mgLevel", 0.2f }, { "mgMode", 0 }, { "attack", 0.4f }, { "decay", 0.5f }, { "sustain", 0.7f }, { "release", 1 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Strings 2", { { "octave", 0 }, { "waveform", 2 }, { "pwPwm", 0.6f }, { "pwmSpeed", 0.35f }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 0.1f }, { "egInt", 0 }, { "kbdTrack", 0.9f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.55f }, { "mgDelay", 0.2f }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.5f }, { "decay", 1 }, { "sustain", 1 }, { "release", 0.55f }, { "fxMode", 3 }, { "fxSpeed", 0.5f }, { "voiceMode", 0 } } },
        { "Electric Piano 1", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 0.35f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0 }, { "resonance", 0.3f }, { "egInt", 0.4f }, { "kbdTrack", 0.35f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.55f }, { "mgDelay", 0 }, { "mgLevel", 0.4f }, { "mgMode", 2 }, { "attack", 0 }, { "decay", 1 }, { "sustain", 0 }, { "release", 1 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Clavi", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 0.75f }, { "pwmSpeed", 0.15f }, { "subOsc", 0 }, { "cutoff", 0.3f }, { "resonance", 0.24f }, { "egInt", 1 }, { "kbdTrack", 0 }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.2f }, { "sustain", 0.75f }, { "release", 0 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Acoustic Piano", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 0.67f }, { "pwmSpeed", 0.4f }, { "subOsc", 0 }, { "cutoff", 0.38f }, { "resonance", 0.2f }, { "egInt", 0 }, { "kbdTrack", 0.5f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.15f }, { "decay", 0.7f }, { "sustain", 0 }, { "release", 0.5f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Organ 1", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0.3f }, { "pwmSpeed", 0.1f }, { "subOsc", 0 }, { "cutoff", 0.22f }, { "resonance", 0.45f }, { "egInt", 0.52f }, { "kbdTrack", 0.56f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.65f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0 }, { "sustain", 0.45f }, { "release", 0.8f }, { "fxMode", 3 }, { "fxSpeed", 0.5f }, { "voiceMode", 0 } } },
        { "Moon Sound", { { "octave", 1 }, { "waveform", 2 }, { "pwPwm", 0.35f }, { "pwmSpeed", 0.25f }, { "subOsc", 0 }, { "cutoff", 0 }, { "resonance", 0.4f }, { "egInt", 1 }, { "kbdTrack", 0 }, { "vcaMode", 0 }, { "atten", -4 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 1 }, { "sustain", 0.5f }, { "release", 1 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Vibes", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0 }, { "pwmSpeed", 0.1f }, { "subOsc", 0 }, { "cutoff", 0.1f }, { "resonance", 0.1f }, { "egInt", 0.3f }, { "kbdTrack", 0.65f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0.6f }, { "mgMode", 2 }, { "attack", 0.1f }, { "decay", 0.65f }, { "sustain", 0.7f }, { "release", 0.8f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Synthe Organ", { { "octave", 2 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 1 }, { "resonance", 0.1f }, { "egInt", 0 }, { "kbdTrack", 1 }, { "vcaMode", 0 }, { "atten", -4 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 1 }, { "attack", 0 }, { "decay", 1 }, { "sustain", 1 }, { "release", 0 }, { "fxMode", 2 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "High Strings", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.45f }, { "resonance", 0.2f }, { "egInt", 0 }, { "kbdTrack", 0.52f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.5f }, { "decay", 0.7f }, { "sustain", 0.7f }, { "release", 0.6f }, { "fxMode", 3 }, { "fxSpeed", 1 }, { "voiceMode", 0 } } },
        { "Fat Solo Sound", { { "octave", 0 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0 }, { "resonance", 0.15f }, { "egInt", 1 }, { "kbdTrack", 0.2f }, { "vcaMode", 0 }, { "atten", 2 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.4f }, { "sustain", 0.9f }, { "release", 0.8f }, { "fxMode", 2 }, { "fxSpeed", 0.2f }, { "voiceMode", 0 } } },
        { "Saxophone", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 0.72f }, { "pwmSpeed", 0.2f }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 0.1f }, { "egInt", 1 }, { "kbdTrack", 0.6f }, { "vcaMode", 0 }, { "atten", 4 }, { "mgFreq", 0.6f }, { "mgDelay", 0.2f }, { "mgLevel", 0.4f }, { "mgMode", 0 }, { "attack", 0.45f }, { "decay", 0.2f }, { "sustain", 0.85f }, { "release", 0.5f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Flute", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0.5f }, { "pwmSpeed", 0.2f }, { "subOsc", 0 }, { "cutoff", 0.2f }, { "resonance", 0.65f }, { "egInt", 0 }, { "kbdTrack", 0.68f }, { "vcaMode", 0 }, { "atten", 10 }, { "mgFreq", 0.6f }, { "mgDelay", 0.3f }, { "mgLevel", 0.55f }, { "mgMode", 0 }, { "attack", 0.35f }, { "decay", 0.65f }, { "sustain", 0.65f }, { "release", 0.3f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Trombone", { { "octave", 0 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0 }, { "resonance", 0.12f }, { "egInt", 0.52f }, { "kbdTrack", 0.5f }, { "vcaMode", 0 }, { "atten", 10 }, { "mgFreq", 1 }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.35f }, { "decay", 0.6f }, { "sustain", 0.85f }, { "release", 0.6f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Guitar", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 0.4f }, { "pwmSpeed", 0.15f }, { "subOsc", 0 }, { "cutoff", 0.13f }, { "resonance", 0.2f }, { "egInt", 0.64f }, { "kbdTrack", 0 }, { "vcaMode", 0 }, { "atten", 10 }, { "mgFreq", 0.65f }, { "mgDelay", 0.3f }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.5f }, { "sustain", 0.8f }, { "release", 0.45f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Honky Tonk Piano", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0.65f }, { "pwmSpeed", 0.2f }, { "subOsc", 0 }, { "cutoff", 1 }, { "resonance", 0.8f }, { "egInt", 1 }, { "kbdTrack", 1 }, { "vcaMode", 0 }, { "atten", 5 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.7f }, { "sustain", 0 }, { "release", 0.6f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Lead Sound", { { "octave", 1 }, { "waveform", 2 }, { "pwPwm", 0.75f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0 }, { "resonance", 0.5f }, { "egInt", 1 }, { "kbdTrack", 1 }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.65f }, { "mgDelay", 0.2f }, { "mgLevel", 0.4f }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 1 }, { "sustain", 0 }, { "release", 1 }, { "fxMode", 2 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Fat Bass", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 1 }, { "cutoff", 0.16f }, { "resonance", 0.5f }, { "egInt", 0.6f }, { "kbdTrack", 0 }, { "vcaMode", 0 }, { "atten", 10 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.35f }, { "sustain", 0.75f }, { "release", 0 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Reggae Sound", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.36f }, { "resonance", 0.22f }, { "egInt", 1 }, { "kbdTrack", 0.2f }, { "vcaMode", 0 }, { "atten", 10 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.25f }, { "sustain", 0.45f }, { "release", 1 }, { "fxMode", 2 }, { "fxSpeed", 0.2f }, { "voiceMode", 0 } } },
        { "Banjo", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 0.7f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.6f }, { "resonance", 0.2f }, { "egInt", -0.28f }, { "kbdTrack", 1 }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.65f }, { "mgDelay", 0.2f }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.55f }, { "sustain", 0 }, { "release", 0.4f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Electric Piano 2", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 0 }, { "pwmSpeed", 0.75f }, { "subOsc", 1 }, { "cutoff", 0.15f }, { "resonance", 0.2f }, { "egInt", 0.6f }, { "kbdTrack", 0 }, { "vcaMode", 0 }, { "atten", 10 }, { "mgFreq", 0.55f }, { "mgDelay", 0 }, { "mgLevel", 0.4f }, { "mgMode", 2 }, { "attack", 0 }, { "decay", 0.84f }, { "sustain", 0.2f }, { "release", 0.8f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Filtered Clavi", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 0.7f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.3f }, { "resonance", 0.45f }, { "egInt", -1 }, { "kbdTrack", 0.35f }, { "vcaMode", 0 }, { "atten", 10 }, { "mgFreq", 0.65f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.3f }, { "sustain", 0 }, { "release", 0.75f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "New Wave Organ", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0.1f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.9f }, { "resonance", 0.24f }, { "egInt", 0 }, { "kbdTrack", 0.35f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0.25f }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 1 }, { "sustain", 1 }, { "release", 1 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Jazz Guitar", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 0.4f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0 }, { "resonance", 0.2f }, { "egInt", 0.46f }, { "kbdTrack", 0.45f }, { "vcaMode", 0 }, { "atten", 10 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.8f }, { "sustain", 0.65f }, { "release", 0.9f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Unison Solo Low", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.2f }, { "resonance", 0.3f }, { "egInt", 0.4f }, { "kbdTrack", 0 }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.75f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 1 }, { "sustain", 1 }, { "release", 1 }, { "fxMode", 2 }, { "fxSpeed", 0 }, { "voiceMode", 1 } } },
        { "Unison Solo High", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 0.75f }, { "pwmSpeed", 0.3f }, { "subOsc", 0 }, { "cutoff", 0.9f }, { "resonance", 0 }, { "egInt", -0.5f }, { "kbdTrack", 0.7f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.75f }, { "sustain", 1 }, { "release", 0 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 1 } } },
        { "Organ 2", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 0.3f }, { "egInt", 0 }, { "kbdTrack", 0.5f }, { "vcaMode", 1 }, { "atten", 2 }, { "mgFreq", 0.55f }, { "mgDelay", 0 }, { "mgLevel", 0.25f }, { "mgMode", 0 }, { "attack", 0.2f }, { "decay", 0 }, { "sustain", 0 }, { "release", 0.45f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Chord Chime", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.17f }, { "resonance", 0.2f }, { "egInt", 0.6f }, { "kbdTrack", 0 }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 1 }, { "sustain", 1 }, { "release", 1 }, { "fxMode", 1 }, { "fxSpeed", 0 }, { "voiceMode", 2 } } },
        { "Sound Fall", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 1 }, { "pwmSpeed", 0.6f }, { "subOsc", 0 }, { "cutoff", 0.25f }, { "resonance", 1 }, { "egInt", 1 }, { "kbdTrack", 0.35f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.75f }, { "mgDelay", 0 }, { "mgLevel", 0.2f }, { "mgMode", 1 }, { "attack", 0 }, { "decay", 1 }, { "sustain", 0.2f }, { "release", 1 }, { "fxMode", 1 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Thin Sound", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0.75f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.7f }, { "resonance", 0.3f }, { "egInt", -1 }, { "kbdTrack", 0.9f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.65f }, { "mgDelay", 0.2f }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.7f }, { "sustain", 0 }, { "release", 0.9f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Touch Sound", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0.6f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.55f }, { "resonance", 0.5f }, { "egInt", -1 }, { "kbdTrack", 1 }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.65f }, { "mgDelay", 0.2f }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.55f }, { "decay", 0.65f }, { "sustain", 0 }, { "release", 1 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Kim Carnes", { { "octave", 0 }, { "waveform", 2 }, { "pwPwm", 0.5f }, { "pwmSpeed", 0.25f }, { "subOsc", 0 }, { "cutoff", 0.6f }, { "resonance", 0.2f }, { "egInt", 1 }, { "kbdTrack", 1 }, { "vcaMode", 0 }, { "atten", 10 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.7f }, { "sustain", 0.65f }, { "release", 0.65f }, { "fxMode", 2 }, { "fxSpeed", 0.2f }, { "voiceMode", 0 } } },
        { "Brass Ensemble", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.3f }, { "resonance", 0.2f }, { "egInt", 0.4f }, { "kbdTrack", 0.5f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.1f }, { "mgLevel", 0.15f }, { "mgMode", 0 }, { "attack", 0.4f }, { "decay", 0.7f }, { "sustain", 0.4f }, { "release", 0.35f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Mellow Brass", { { "octave", 0 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 0.3f }, { "egInt", 0.2f }, { "kbdTrack", 0.5f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.4f }, { "decay", 0.7f }, { "sustain", 0.4f }, { "release", 0.7f }, { "fxMode", 1 }, { "fxSpeed", 0.35f }, { "voiceMode", 0 } } },
        { "Oboe", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.5f }, { "resonance", 0.2f }, { "egInt", 0 }, { "kbdTrack", 0.4f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.1f }, { "mgLevel", 0.2f }, { "mgMode", 0 }, { "attack", 0.4f }, { "decay", 0.7f }, { "sustain", 0.4f }, { "release", 0.3f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Flute 2", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0.4f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.35f }, { "resonance", 0.3f }, { "egInt", 0 }, { "kbdTrack", 0.5f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.15f }, { "mgLevel", 0.8f }, { "mgMode", 2 }, { "attack", 0.45f }, { "decay", 0.4f }, { "sustain", 0.4f }, { "release", 0.2f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "String Orchestra", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.6f }, { "resonance", 0 }, { "egInt", 0 }, { "kbdTrack", 0.4f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.7f }, { "decay", 0 }, { "sustain", 1 }, { "release", 0.7f }, { "fxMode", 3 }, { "fxSpeed", 0.4f }, { "voiceMode", 0 } } },
        { "Solo Violin", { { "octave", 2 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.5f }, { "resonance", 0 }, { "egInt", 0 }, { "kbdTrack", 0.5f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.1f }, { "mgLevel", 0.1f }, { "mgMode", 0 }, { "attack", 0.6f }, { "decay", 0.7f }, { "sustain", 0.4f }, { "release", 0.6f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Bowing Strings", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.6f }, { "resonance", 0 }, { "egInt", 0 }, { "kbdTrack", 0.4f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.6f }, { "decay", 0.6f }, { "sustain", 0.4f }, { "release", 0.7f }, { "fxMode", 3 }, { "fxSpeed", 0.4f }, { "voiceMode", 0 } } },
        { "Koto", { { "octave", 1 }, { "waveform", 2 }, { "pwPwm", 0.5f }, { "pwmSpeed", 0.35f }, { "subOsc", 0 }, { "cutoff", 0.5f }, { "resonance", 0.2f }, { "egInt", 0.4f }, { "kbdTrack", 0.4f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0.2f }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.6f }, { "sustain", 0 }, { "release", 0.7f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Electric Piano", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 0.5f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.35f }, { "resonance", 0.4f }, { "egInt", 0 }, { "kbdTrack", 0.4f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0.3f }, { "mgMode", 2 }, { "attack", 0 }, { "decay", 0.7f }, { "sustain", 0 }, { "release", 0.7f }, { "fxMode", 1 }, { "fxSpeed", 0.4f }, { "voiceMode", 0 } } },
        { "Organ Slow", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 2 }, { "cutoff", 0.4f }, { "resonance", 0.3f }, { "egInt", 0.2f }, { "kbdTrack", 0.45f }, { "vcaMode", 1 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0 }, { "sustain", 0 }, { "release", 0 }, { "fxMode", 2 }, { "fxSpeed", 0.2f }, { "voiceMode", 0 } } },
        { "Organ Fast", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 2 }, { "cutoff", 0.4f }, { "resonance", 0.3f }, { "egInt", 0.2f }, { "kbdTrack", 0.45f }, { "vcaMode", 1 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0 }, { "sustain", 0 }, { "release", 0 }, { "fxMode", 3 }, { "fxSpeed", 0.4f }, { "voiceMode", 0 } } },
        { "Harpsichord", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0.6f }, { "pwmSpeed", 0 }, { "subOsc", 1 }, { "cutoff", 0.6f }, { "resonance", 0.4f }, { "egInt", 0.6f }, { "kbdTrack", 0.7f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.6f }, { "sustain", 0.4f }, { "release", 0.65f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Clavi 2", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 0.8f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.35f }, { "resonance", 0.2f }, { "egInt", 0.6f }, { "kbdTrack", 0.4f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.7f }, { "sustain", 0 }, { "release", 0 }, { "fxMode", 2 }, { "fxSpeed", 0.2f }, { "voiceMode", 0 } } },
        { "Female Chorus", { { "octave", 2 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.5f }, { "resonance", 0.8f }, { "egInt", 0 }, { "kbdTrack", 0.2f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.1f }, { "mgLevel", 0.1f }, { "mgMode", 0 }, { "attack", 0.6f }, { "decay", 0.5f }, { "sustain", 0.8f }, { "release", 0.1f }, { "fxMode", 3 }, { "fxSpeed", 0.7f }, { "voiceMode", 0 } } },
        { "Male Chorus", { { "octave", 0 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.5f }, { "resonance", 0.8f }, { "egInt", 0 }, { "kbdTrack", 0.2f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.1f }, { "mgLevel", 0.1f }, { "mgMode", 0 }, { "attack", 0.6f }, { "decay", 0.5f }, { "sustain", 0.8f }, { "release", 0.1f }, { "fxMode", 3 }, { "fxSpeed", 0.7f }, { "voiceMode", 0 } } },
        { "Steam Pipe Organ", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 1 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.35f }, { "resonance", 1 }, { "egInt", 0 }, { "kbdTrack", 0.55f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.2f }, { "decay", 0 }, { "sustain", 1 }, { "release", 0.2f }, { "fxMode", 3 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Synthe 1", { { "octave", 1 }, { "waveform", 2 }, { "pwPwm", 0.5f }, { "pwmSpeed", 0.1f }, { "subOsc", 0 }, { "cutoff", 0 }, { "resonance", 0.65f }, { "egInt", 0.8f }, { "kbdTrack", 0.5f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.7f }, { "sustain", 0.5f }, { "release", 0 }, { "fxMode", 1 }, { "fxSpeed", 0.3f }, { "voiceMode", 0 } } },
        { "Synthe 2", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.5f }, { "resonance", 0.7f }, { "egInt", -0.4f }, { "kbdTrack", 0.5f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.7f }, { "mgLevel", 0.35f }, { "mgMode", 0 }, { "attack", 0.7f }, { "decay", 0.8f }, { "sustain", 0 }, { "release", 0.7f }, { "fxMode", 2 }, { "fxSpeed", 0.3f }, { "voiceMode", 0 } } },
        { "Synthe 3", { { "octave", 1 }, { "waveform", 2 }, { "pwPwm", 0.1f }, { "pwmSpeed", 0.3f }, { "subOsc", 0 }, { "cutoff", 0.6f }, { "resonance", 0.65f }, { "egInt", -0.2f }, { "kbdTrack", 0.4f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.3f }, { "mgDelay", 0 }, { "mgLevel", 0.3f }, { "mgMode", 1 }, { "attack", 0.1f }, { "decay", 0.7f }, { "sustain", 0.4f }, { "release", 0.6f }, { "fxMode", 2 }, { "fxSpeed", 0.6f }, { "voiceMode", 0 } } },
        { "Slow Wah", { { "octave", 1 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.35f }, { "resonance", 0.7f }, { "egInt", 0.4f }, { "kbdTrack", 0.2f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0.1f }, { "mgLevel", 0.1f }, { "mgMode", 1 }, { "attack", 0.8f }, { "decay", 0.8f }, { "sustain", 0.8f }, { "release", 0.8f }, { "fxMode", 2 }, { "fxSpeed", 0.1f }, { "voiceMode", 0 } } },
        { "Synthe Bass", { { "octave", 0 }, { "waveform", 0 }, { "pwPwm", 0 }, { "pwmSpeed", 0 }, { "subOsc", 1 }, { "cutoff", 0.3f }, { "resonance", 0.55f }, { "egInt", 0.4f }, { "kbdTrack", 0.3f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.6f }, { "sustain", 0.6f }, { "release", 0.6f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Synthe 4", { { "octave", 1 }, { "waveform", 2 }, { "pwPwm", 0.7f }, { "pwmSpeed", 0.2f }, { "subOsc", 0 }, { "cutoff", 0.35f }, { "resonance", 1 }, { "egInt", 0 }, { "kbdTrack", 1 }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0.3f }, { "decay", 0 }, { "sustain", 1 }, { "release", 0.6f }, { "fxMode", 1 }, { "fxSpeed", 0.2f }, { "voiceMode", 0 } } },
        { "Oriental Sound", { { "octave", 1 }, { "waveform", 2 }, { "pwPwm", 0.5f }, { "pwmSpeed", 0.2f }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 0.2f }, { "egInt", 0.4f }, { "kbdTrack", 0.4f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.7f }, { "mgDelay", 0 }, { "mgLevel", 0.3f }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.6f }, { "sustain", 0.4f }, { "release", 0.7f }, { "fxMode", 1 }, { "fxSpeed", 0.4f }, { "voiceMode", 0 } } },
        { "Synthe Harp", { { "octave", 2 }, { "waveform", 1 }, { "pwPwm", 0.4f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 0.3f }, { "egInt", 0.6f }, { "kbdTrack", 0.4f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.6f }, { "sustain", 0.5f }, { "release", 0.8f }, { "fxMode", 3 }, { "fxSpeed", 0.3f }, { "voiceMode", 0 } } },
        { "Whistle", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 1 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.3f }, { "resonance", 1 }, { "egInt", 0.1f }, { "kbdTrack", 0.6f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.1f }, { "mgLevel", 0.1f }, { "mgMode", 1 }, { "attack", 0.5f }, { "decay", 0 }, { "sustain", 1 }, { "release", 0 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Sound Effect 1", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 1 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.2f }, { "resonance", 1 }, { "egInt", 0 }, { "kbdTrack", 1 }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 1 }, { "mgDelay", 0 }, { "mgLevel", 0.3f }, { "mgMode", 1 }, { "attack", 0 }, { "decay", 0 }, { "sustain", 1 }, { "release", 0.6f }, { "fxMode", 2 }, { "fxSpeed", 0.4f }, { "voiceMode", 0 } } },
        { "Gambang", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 0.2f }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 1 }, { "egInt", 0 }, { "kbdTrack", 0.6f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.4f }, { "mgDelay", 0 }, { "mgLevel", 0.15f }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.4f }, { "sustain", 0.55f }, { "release", 0.8f }, { "fxMode", 1 }, { "fxSpeed", 0.4f }, { "voiceMode", 0 } } },
        { "Sound Effect 2", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 1 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.3f }, { "resonance", 1 }, { "egInt", 0.4f }, { "kbdTrack", 0.6f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.75f }, { "mgDelay", 0 }, { "mgLevel", 0.25f }, { "mgMode", 1 }, { "attack", 0 }, { "decay", 0.7f }, { "sustain", 0.7f }, { "release", 0.8f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Sound Effect 3", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 1 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 1 }, { "egInt", -0.2f }, { "kbdTrack", 0.6f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.6f }, { "mgDelay", 0.2f }, { "mgLevel", 0.1f }, { "mgMode", 1 }, { "attack", 0.2f }, { "decay", 0.5f }, { "sustain", 0.6f }, { "release", 0.8f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Cave Echo", { { "octave", 0 }, { "waveform", 1 }, { "pwPwm", 1 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.4f }, { "resonance", 1 }, { "egInt", 0 }, { "kbdTrack", 0.5f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0.2f }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.5f }, { "sustain", 0.4f }, { "release", 0.8f }, { "fxMode", 3 }, { "fxSpeed", 1 }, { "voiceMode", 0 } } },
        { "Sound Effect 4", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 1 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.45f }, { "resonance", 1 }, { "egInt", -0.6f }, { "kbdTrack", 0.7f }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.8f }, { "sustain", 0.4f }, { "release", 1 }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
        { "Sound Effect 5", { { "octave", 1 }, { "waveform", 1 }, { "pwPwm", 1 }, { "pwmSpeed", 0 }, { "subOsc", 0 }, { "cutoff", 0.2f }, { "resonance", 1 }, { "egInt", 0.3f }, { "kbdTrack", 1 }, { "vcaMode", 0 }, { "atten", 0 }, { "mgFreq", 0.5f }, { "mgDelay", 0 }, { "mgLevel", 0 }, { "mgMode", 0 }, { "attack", 0 }, { "decay", 0.3f }, { "sustain", 0.4f }, { "release", 0.2f }, { "fxMode", 0 }, { "fxSpeed", 0 }, { "voiceMode", 0 } } },
    };
    return list;
}
} // namespace

// ---------------------------------------------------------------------------------------------
juce::AudioProcessorValueTreeState::ParameterLayout PolysixAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    const p6::Params d;

    // Master
    l.add (knob ("volume", "Volume", d.volume));
    l.add (custom ("tune", "Tune", -1.0f, 1.0f, d.tune, [] (float v, int) { return juce::String (v * 100.0f, 0) + " ct"; }));
    // VCO
    l.add (choice ("octave", "Octave", { "16'", "8'", "4'" }, d.octave));
    l.add (choice ("waveform", "Waveform", { "Saw", "Pulse", "PWM" }, d.waveform));
    l.add (knob ("pwPwm", "PW/PWM", d.pwPwm));
    l.add (custom ("pwmSpeed", "PWM Speed", 0.0f, 1.0f, d.pwmSpeed,
                   [] (float v, int) { return juce::String (0.1f * std::exp2 (v * 7.0f), 2) + " Hz"; }));
    l.add (choice ("subOsc", "Sub Osc", { "Off", "1 Oct", "2 Oct" }, d.subOsc));
    // VCF
    l.add (custom ("cutoff", "Cutoff", 0.0f, 1.0f, d.cutoff, [] (float v, int)
    {
        const float hz = p6::pitchToHz (21.0f + v * 110.0f);
        return hz < 1000.0f ? juce::String (hz, 0) + " Hz" : juce::String (hz / 1000.0f, 2) + " kHz";
    }));
    l.add (knob ("resonance", "Resonance", d.resonance));
    l.add (custom ("egInt", "EG Intensity", -1.0f, 1.0f, d.egInt, [] (float v, int) { return juce::String (v * 10.0f, 1); }));
    l.add (knob ("kbdTrack", "Kbd Track", d.kbdTrack));
    // VCA
    l.add (choice ("vcaMode", "VCA Mode", { "EG", "Gate" }, d.vcaMode));
    l.add (custom ("atten", "Attenuator", -10.0f, 10.0f, d.attenDb, [] (float v, int) { return juce::String (v, 1) + " dB"; }));
    // EG
    auto egTime = [] (float v, int) { return fmtTime (p6::logTimeToSec (v * 80.0f)); };
    l.add (custom ("attack", "Attack", 0.0f, 1.0f, d.attack, egTime));
    l.add (custom ("decay", "Decay", 0.0f, 1.0f, d.decay, egTime));
    l.add (knob ("sustain", "Sustain", d.sustain));
    l.add (custom ("release", "Release", 0.0f, 1.0f, d.release, egTime));
    // MG
    l.add (custom ("mgFreq", "MG Frequency", 0.0f, 1.0f, d.mgFreq, [] (float v, int) { return juce::String (p6::mgKnobToHz (v), 2) + " Hz"; }));
    l.add (custom ("mgDelay", "MG Delay", 0.0f, 1.0f, d.mgDelay, [] (float v, int) { return fmtTime (p6::logTimeToSec (v * 70.0f)); }));
    l.add (knob ("mgLevel", "MG Level", d.mgLevel));
    l.add (choice ("mgMode", "MG Mode", { "VCO", "VCF", "VCA" }, d.mgMode));
    // Voice
    l.add (choice ("voiceMode", "Voice Mode", { "Poly", "Unison", "Chord" }, d.voiceMode));
    l.add (toggle ("hold", "Hold", d.hold));
    l.add (knob ("analog", "Analog", d.analog));
    l.add (knob ("detune", "Detune", d.detune));
    l.add (custom ("bendRange", "Bend Range", 0.0f, 12.0f, d.bendRange, [] (float v, int) { return juce::String ((int) v) + " st"; }, 1.0f));
    l.add (custom ("glide", "Glide", 0.0f, 1.0f, d.glide, [] (float v, int)
    {
        return v <= 0.0f ? juce::String ("Off") : fmtTime (0.005f * std::pow (600.0f, v));
    }));
    l.add (toggle ("glideLegato", "Glide Legato", d.glideLegato));
    // Arpeggiator
    l.add (toggle ("arpOn", "Arpeggio", d.arpOn));
    l.add (custom ("arpSpeed", "Arp Speed", 0.0f, 1.0f, d.arpSpeed,
                   [] (float v, int) { return juce::String (0.5f * std::exp2 (v * 5.6f), 1) + " /s"; }));
    l.add (choice ("arpRange", "Arp Range", { "1 Oct", "2 Oct", "Full" }, d.arpRange));
    l.add (choice ("arpMode", "Arp Mode", { "Up", "Down", "Up/Down" }, d.arpMode));
    l.add (toggle ("arpLatch", "Arp Latch", d.arpLatch));
    l.add (toggle ("arpSync", "Arp Sync", d.arpSync));
    l.add (choice ("arpDiv", "Arp Division", { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, d.arpDiv));
    // Effects
    l.add (choice ("fxMode", "Effect", { "Off", "Chorus", "Phaser", "Ensemble" }, d.fxMode));
    l.add (knob ("fxSpeed", "Speed/Intensity", d.fxSpeed));
    return l;
}

PolysixAudioProcessor::PolysixAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PolysixPort", createLayout())
{
    auto r = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    raw = { r ("volume"), r ("tune"), r ("octave"), r ("waveform"), r ("pwPwm"), r ("pwmSpeed"), r ("subOsc"),
            r ("cutoff"), r ("resonance"), r ("egInt"), r ("kbdTrack"), r ("vcaMode"), r ("atten"),
            r ("attack"), r ("decay"), r ("sustain"), r ("release"),
            r ("mgFreq"), r ("mgDelay"), r ("mgLevel"), r ("mgMode"),
            r ("voiceMode"), r ("hold"), r ("analog"), r ("detune"), r ("bendRange"), r ("glide"), r ("glideLegato"),
            r ("arpOn"), r ("arpSpeed"), r ("arpRange"), r ("arpMode"), r ("arpLatch"), r ("arpSync"), r ("arpDiv"),
            r ("fxMode"), r ("fxSpeed") };

    presets = std::make_unique<PresetManager> (*this);
}

bool PolysixAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PolysixAudioProcessor::prepareToPlay (double sampleRate, int)
{
    pullParams();
    engine.prepare (sampleRate);
}

void PolysixAudioProcessor::pullParams()
{
    auto f = [] (std::atomic<float>* a) { return a->load (std::memory_order_relaxed); };
    auto i = [&] (std::atomic<float>* a) { return (int) std::lround (f (a)); };
    auto b = [&] (std::atomic<float>* a) { return f (a) > 0.5f; };
    auto& p = engine.p;
    p.volume = f (raw.volume); p.tune = f (raw.tune);
    p.octave = i (raw.octave); p.waveform = i (raw.waveform); p.pwPwm = f (raw.pwPwm); p.pwmSpeed = f (raw.pwmSpeed); p.subOsc = i (raw.subOsc);
    p.cutoff = f (raw.cutoff); p.resonance = f (raw.resonance); p.egInt = f (raw.egInt); p.kbdTrack = f (raw.kbdTrack);
    p.vcaMode = i (raw.vcaMode); p.attenDb = f (raw.atten);
    p.attack = f (raw.attack); p.decay = f (raw.decay); p.sustain = f (raw.sustain); p.release = f (raw.release);
    p.mgFreq = f (raw.mgFreq); p.mgDelay = f (raw.mgDelay); p.mgLevel = f (raw.mgLevel); p.mgMode = i (raw.mgMode);
    p.voiceMode = i (raw.voiceMode); p.hold = b (raw.hold); p.analog = f (raw.analog); p.detune = f (raw.detune); p.bendRange = f (raw.bendRange);
    p.glide = f (raw.glide); p.glideLegato = b (raw.glideLegato);
    p.arpOn = b (raw.arpOn); p.arpSpeed = f (raw.arpSpeed); p.arpRange = i (raw.arpRange); p.arpMode = i (raw.arpMode);
    p.arpLatch = b (raw.arpLatch); p.arpSync = b (raw.arpSync); p.arpDiv = i (raw.arpDiv);
    p.fxMode = i (raw.fxMode); p.fxSpeed = f (raw.fxSpeed);
    { int bits = 0; for (int c = 0; c < p6::kCalibCount; ++c) if (calibDefault (c)) bits |= 1 << c; p.calib = bits; } // fixed: all on except Effects and Noise Floor
}

void PolysixAudioProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                     engine.noteOn (m.getNoteNumber());
    else if (m.isNoteOff())               engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())            engine.pitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isSustainPedalOn())        engine.setSustainPedal (true);
    else if (m.isSustainPedalOff())       engine.setSustainPedal (false);
    else if (m.isController() && m.getControllerNumber() == 1)
                                          engine.modWheel ((float) m.getControllerValue() / 127.0f);
    else if (m.isAllNotesOff() || m.isAllSoundOff())
                                          engine.allNotesOff();
}

void PolysixAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    pullParams();

    bool playing = false; double bpm = 120.0, ppq = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            bpm     = pos->getBpm().orFallback (120.0);
            ppq     = pos->getPpqPosition().orFallback (0.0);
        }
    engine.setTransport (playing, bpm, ppq);

    // on-screen wheels
    if (uiBendDirty.exchange (false)) engine.pitchBend (uiBend.load());
    if (uiModDirty.exchange (false))  engine.modWheel (uiMod.load());

    buffer.clear();
    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (buffer.getNumChannels() > 1 ? 1 : 0);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int s = juce::jlimit (0, n, meta.samplePosition);
        if (s > pos) { engine.render (L + pos, R + pos, s - pos); pos = s; }
        handleMidi (meta.getMessage());
    }
    if (pos < n) engine.render (L + pos, R + pos, n - pos);

    shownBend.store (engine.getPitchBend());
    shownMod.store (engine.getModWheel());
}

// ---- programs ----------------------------------------------------------------------------------
int PolysixAudioProcessor::getNumPrograms() { return (int) programs().size(); }

const juce::String PolysixAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, getNumPrograms()) ? programs()[(size_t) index].name : "";
}

void PolysixAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms())) return;
    currentProgram = index;
    const auto& prog = programs()[(size_t) index];
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            float value = rp->convertFrom0to1 (rp->getDefaultValue());
            for (const auto& [id, v] : prog.values)
                if (rp->paramID == id) value = v;
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
        }
    if (presets != nullptr) presets->setCurrentKey ("F:" + juce::String (prog.name));
}

// ---- state ----------------------------------------------------------------------------------
juce::ValueTree PolysixAudioProcessor::captureState()
{
    auto state = apvts.copyState();
    int iv[p6::kNumVoices]; int root = 60;
    const int n = engine.getChord (iv, root);
    juce::StringArray chord;
    for (int i = 0; i < n; ++i) chord.add (juce::String (iv[i]));
    state.setProperty ("chordMemory", chord.joinIntoString (","), nullptr);
    state.setProperty ("chordRoot", root, nullptr);
    return state;
}

void PolysixAudioProcessor::restoreState (const juce::ValueTree& source, bool keepModel)
{
    if (! source.hasType (apvts.state.getType())) return;
    auto tree = source.createCopy();
    // presets don't change the calibration switches; sessions do
    auto keep = [&] (const juce::String& id, std::atomic<float>* now, float def)
    {
        const float current = now->load();
        auto child = tree.getChildWithProperty ("id", id);
        if (keepModel || ! child.isValid())
        {
            if (! child.isValid()) { child = juce::ValueTree ("PARAM"); child.setProperty ("id", id, nullptr); tree.appendChild (child, nullptr); }
            child.setProperty ("value", keepModel ? current : def, nullptr);
        }
    };
    const auto chord = juce::StringArray::fromTokens (tree.getProperty ("chordMemory", "0,3,7").toString(), ",", "");
    std::array<int, p6::kNumVoices> mem {};
    int count = 0;
    for (const auto& s : chord)
        if (count < p6::kNumVoices && s.trim().isNotEmpty()) mem[(size_t) count++] = s.getIntValue();
    if (count > 0) engine.setChord (mem.data(), count, (int) tree.getProperty ("chordRoot", 60));
    tree.removeProperty ("program", nullptr);
    tree.removeProperty ("presetKey", nullptr);
    apvts.replaceState (tree);
}

void PolysixAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = captureState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("presetKey", presets->getCurrentKey(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void PolysixAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            const auto key = tree.getProperty ("presetKey", "F:Init").toString();
            restoreState (tree);
            presets->setCurrentKey (key);
        }
}

juce::AudioProcessorEditor* PolysixAudioProcessor::createEditor() { return new PolysixEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PolysixAudioProcessor(); }
