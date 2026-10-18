#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/PolysixEngine.h"
#include "PresetManager.h"

class PolysixAudioProcessor : public juce::AudioProcessor
{
public:
    PolysixAudioProcessor();
    ~PolysixAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // full sound state (parameters + chord memory) for presets and host sessions
    juce::ValueTree captureState();
    void restoreState (const juce::ValueTree& tree);

    PresetManager& getPresetManager() { return *presets; }

    // chord memory (MEMORY button + display)
    void requestChordMemory()                       { engine.requestChordCapture(); }
    int  getChord (int* intervals, int& root) const { return engine.getChord (intervals, root); }
    int  getChordVersion() const                    { return engine.getChordVersion(); }

    // on-screen wheels: UI -> engine (applied at the next block), engine -> UI (display)
    void setUiPitchBend (float v) { uiBend.store (v); uiBendDirty.store (true); }
    void setUiModWheel (float v)  { uiMod.store (v);  uiModDirty.store (true); }
    float getPitchBendDisplay() const { return shownBend.load(); }
    float getModWheelDisplay() const  { return shownMod.load(); }

private:
    p6::Engine engine;
    int currentProgram = 0;
    std::unique_ptr<PresetManager> presets;
    std::atomic<float> uiBend { 0.0f }, uiMod { 0.0f }, shownBend { 0.0f }, shownMod { 0.0f };
    std::atomic<bool>  uiBendDirty { false }, uiModDirty { false };

    struct Raw
    {
        std::atomic<float> *volume, *tune, *octave, *waveform, *pwPwm, *pwmSpeed, *subOsc,
                           *cutoff, *resonance, *egInt, *kbdTrack, *vcaMode, *atten,
                           *attack, *decay, *sustain, *release,
                           *mgFreq, *mgDelay, *mgLevel, *mgMode,
                           *voiceMode, *hold, *analog, *detune, *bendRange, *glide, *glideLegato,
                           *arpOn, *arpSpeed, *arpRange, *arpMode, *arpLatch, *arpSync, *arpDiv,
                           *fxMode, *fxSpeed;
    } raw {};

    void pullParams();
    void handleMidi (const juce::MidiMessage& m);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolysixAudioProcessor)
};
