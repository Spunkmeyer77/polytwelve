#pragma once

#include "PluginProcessor.h"
#include "PanelLayout.h"

// The panel is drawn at a fixed design size (ui::designW x ui::designH) inside PanelCanvas,
// which is scaled to the editor's size. Graphics come from Resources/ui (tools/ui/build_ui.py).

class PanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PanelLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getSliderPopupFont (juce::Slider&) override;
    juce::Image cap;
};

// Rubber button with its LED, from two baked images (off / on).
class ImgButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ImgButton (const juce::String& id);
    std::function<void()> onClick;
    std::function<bool()> isOn;
    void refresh() { const bool s = isOn ? isOn() : false; if (s != shown) { shown = s; repaint(); } }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override;
    bool hitTest (int x, int y) override;
private:
    juce::Image off, on;
    bool shown = false, down = false;
};

// Slide switch for a choice parameter (2 or 3 positions); its position labels are part of the hit area.
class SlideSwitch : public juce::Component, public juce::SettableTooltipClient
{
public:
    SlideSwitch (juce::RangedAudioParameter&, int positions, float switchWidth);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent& e) override { mouseDown (e); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    void setPos (int p);
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    int n, pos = 0;
    float swW;
    juce::Image images[3];
};

// Pitch / mod wheel: the wheel itself is in the background picture; this draws its position mark.
class PerfWheel : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit PerfWheel (bool springBack) : spring (springBack) {}
    std::function<void (float)> onChange;
    void setDisplayValue (float v) { if (! dragging && std::abs (v - value) > 1.0e-4f) { value = v; repaint(); } }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    void set (float v);
    bool spring, dragging = false;
    float value = 0.0f, startValue = 0.0f;
};

// Steps an int/choice parameter by dragging or scrolling over a display.
class StepArea : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void (int)> step;
    std::function<void()> click;
    void mouseDown (const juce::MouseEvent&) override { acc = 0.0f; moved = false; }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (! moved && click) click(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    { if (step && w.deltaY != 0.0f) step ((w.deltaY * (w.isReversed ? -1.0f : 1.0f)) > 0 ? 1 : -1); }
private:
    float acc = 0.0f, lastY = 0.0f;
    bool moved = false;
};

class AboutOverlay : public juce::Component
{
public:
    AboutOverlay();
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::Rectangle<float> box() const;
    juce::Rectangle<float> mailR, shopR;
};

class PolysixEditor;

class PanelCanvas : public juce::Component
{
public:
    explicit PanelCanvas (PolysixEditor& e) : editor (e) {}
    void paint (juce::Graphics&) override;
private:
    PolysixEditor& editor;
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
    friend class PanelCanvas;
    PolysixAudioProcessor& proc;
    PanelLookAndFeel lnf;
    PanelCanvas canvas { *this };
    juce::Image background;
    juce::Typeface::Ptr monoFace;
    float lastShown[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAtts;
    std::vector<std::unique_ptr<juce::Component>> others;
    std::vector<ImgButton*> buttons;
    PerfWheel* pitchWheel = nullptr;
    PerfWheel* modWheel = nullptr;
    AboutOverlay about;
    juce::TooltipWindow tooltips { this, 700 };

    juce::Rectangle<float> presetR, chordR, arpDivR, bendR, infoR;
    juce::String chordNameText, chordNotesText;
    bool favouritesOnly = false;
    std::unique_ptr<juce::FileChooser> chooser;
    int lastChordVersion = -1, flashTicks = 0;

    float value (const juce::String& id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void setValue (const juce::String& id, float v);
    void stepParam (const char* id, int delta);
    ImgButton& addButton (const ui::Item&);

    void changeListenerCallback (juce::ChangeBroadcaster*) override { canvas.repaint(); refreshButtons(); }
    void timerCallback() override;
    void refreshButtons();
    void refreshChord();
    void showPresetMenu();
    void askSaveName();
    void saveWithName (const juce::String& name);
    void askDelete();
    void openPresetFile();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolysixEditor)
};
