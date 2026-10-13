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

std::unique_ptr<APB> toggle (const char* id, const char* name, bool def)
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
                           { "resonance", 0.25f }, { "egInt", 0.2f }, { "mgDelay", 0.6f }, { "mgLevel", 0.35f }, { "mgFreq", 0.42f },
                           { "fxMode", 1 }, { "fxSpeed", 0.3f } } },
        { "Rezzy Arp", { { "waveform", 0 }, { "cutoff", 0.42f }, { "resonance", 0.75f }, { "egInt", 0.5f }, { "decay", 0.35f },
                         { "sustain", 0 }, { "release", 0.3f }, { "arpOn", 1 }, { "arpSync", 1 }, { "arpDiv", 3 }, { "arpRange", 1 },
                         { "arpMode", 2 }, { "fxMode", 2 }, { "fxSpeed", 0.3f } } },
        { "Filter Sweep", { { "waveform", 2 }, { "pwPwm", 0.7f }, { "pwmSpeed", 0.3f }, { "cutoff", 0.32f }, { "resonance", 0.6f },
                            { "mgMode", 1 }, { "mgLevel", 0.55f }, { "mgFreq", 0.18f }, { "attack", 0.5f }, { "release", 0.6f },
                            { "fxMode", 3 }, { "fxSpeed", 0.5f } } },
        { "Minor Chord", { { "voiceMode", 2 }, { "waveform", 0 }, { "cutoff", 0.45f }, { "resonance", 0.3f }, { "egInt", 0.4f },
                           { "decay", 0.45f }, { "sustain", 0.3f }, { "release", 0.4f }, { "fxMode", 1 }, { "fxSpeed", 0.35f } } },
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
    auto egTime = [] (float v, int) { return fmtTime (p6::reaktorLogTimeToSec (v * 80.0f)); };
    l.add (custom ("attack", "Attack", 0.0f, 1.0f, d.attack, egTime));
    l.add (custom ("decay", "Decay", 0.0f, 1.0f, d.decay, egTime));
    l.add (knob ("sustain", "Sustain", d.sustain));
    l.add (custom ("release", "Release", 0.0f, 1.0f, d.release, egTime));
    // MG
    l.add (custom ("mgFreq", "MG Frequency", 0.0f, 1.0f, d.mgFreq, [] (float v, int) { return juce::String (p6::mgKnobToHz (v), 2) + " Hz"; }));
    l.add (custom ("mgDelay", "MG Delay", 0.0f, 1.0f, d.mgDelay, [] (float v, int) { return fmtTime (p6::reaktorLogTimeToSec (v * 70.0f)); }));
    l.add (knob ("mgLevel", "MG Level", d.mgLevel));
    l.add (choice ("mgMode", "MG Mode", { "VCO", "VCF", "VCA" }, d.mgMode));
    // Voice
    l.add (choice ("voiceMode", "Voice Mode", { "Poly", "Unison", "Chord" }, d.voiceMode));
    l.add (toggle ("hold", "Hold", d.hold));
    l.add (knob ("analog", "Analog", d.analog));
    l.add (knob ("detune", "Detune", d.detune));
    l.add (custom ("bendRange", "Bend Range", 0.0f, 12.0f, d.bendRange, [] (float v, int) { return juce::String ((int) v) + " st"; }, 1.0f));
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
            r ("voiceMode"), r ("hold"), r ("analog"), r ("detune"), r ("bendRange"),
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
    p.arpOn = b (raw.arpOn); p.arpSpeed = f (raw.arpSpeed); p.arpRange = i (raw.arpRange); p.arpMode = i (raw.arpMode);
    p.arpLatch = b (raw.arpLatch); p.arpSync = b (raw.arpSync); p.arpDiv = i (raw.arpDiv);
    p.fxMode = i (raw.fxMode); p.fxSpeed = f (raw.fxSpeed);
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
    if (juce::String (prog.name) == "Minor Chord") { const int minor[] = { 0, 3, 7 }; engine.setChord (minor, 3, 60); }
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

void PolysixAudioProcessor::restoreState (const juce::ValueTree& source)
{
    if (! source.hasType (apvts.state.getType())) return;
    auto tree = source.createCopy();
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
