#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class PolysixLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PolysixLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

// White switch with LED (Mono/Poly style); momentary, LED shows an external state
class LedButton : public juce::Button
{
public:
    LedButton() : juce::Button ("led") {}
    void setLit (bool b) { if (b != lit) { lit = b; repaint(); } }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
private:
    bool lit = false;
};

// Amber read-out showing the stored chord
class ChordDisplay : public juce::Component
{
public:
    void set (const juce::String& nameText, const juce::String& notesText, bool active);
    void paint (juce::Graphics&) override;
private:
    juce::String name, notes;
    bool active = false;
};

class PolysixEditor : public juce::AudioProcessorEditor,
                      private juce::ChangeListener,
                      private juce::Timer
{
public:
    explicit PolysixEditor (PolysixAudioProcessor&);
    ~PolysixEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class Kind { Knob, Choice, Toggle, Custom };
    struct Ctrl
    {
        Kind kind; juce::String paramId, label;
        std::unique_ptr<juce::Component> comp;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   sAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   bAtt;
        juce::Rectangle<int> cell;
        int span = 1;
    };
    struct Section { juce::String title; int row; std::vector<std::unique_ptr<Ctrl>> ctrls; juce::Rectangle<int> bounds; };

    PolysixAudioProcessor& proc;
    PolysixLookAndFeel lnf;
    std::vector<Section> sections;
    // preset bar
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, nameBtn, favBtn, saveBtn { "SAVE" }, delBtn { "DEL" }, favOnlyBtn;
    bool favouritesOnly = false;
    std::unique_ptr<juce::FileChooser> chooser;

    void changeListenerCallback (juce::ChangeBroadcaster*) override { updatePresetBar(); }
    void updatePresetBar();
    void showPresetMenu();
    void askSaveName();
    void saveWithName (const juce::String& name);
    void askDelete();
    void openPresetFile();

    Section& addSection (const juce::String& title, int row);
    void add (Section&, Kind, const juce::String& id, const juce::String& label);
    void addCustom (Section&, std::unique_ptr<juce::Component>, const juce::String& label, int span);

    // chord memory
    LedButton*    memoryBtn = nullptr;
    ChordDisplay* chordDisplay = nullptr;
    int lastChordVersion = -1, flashTicks = 0;
    void timerCallback() override;
    void refreshChordDisplay();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolysixEditor)
};
