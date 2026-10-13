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

private:
    p6::Engine engine;
    int currentProgram = 0;
    std::unique_ptr<PresetManager> presets;

    struct Raw
    {
        std::atomic<float> *volume, *tune, *octave, *waveform, *pwPwm, *pwmSpeed, *subOsc,
                           *cutoff, *resonance, *egInt, *kbdTrack, *vcaMode, *atten,
                           *attack, *decay, *sustain, *release,
                           *mgFreq, *mgDelay, *mgLevel, *mgMode,
                           *voiceMode, *hold, *analog, *detune, *bendRange,
                           *arpOn, *arpSpeed, *arpRange, *arpMode, *arpLatch, *arpSync, *arpDiv,
                           *fxMode, *fxSpeed;
    } raw {};

    void pullParams();
    void handleMidi (const juce::MidiMessage& m);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolysixAudioProcessor)
};
