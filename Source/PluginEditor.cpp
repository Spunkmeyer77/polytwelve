#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
const juce::Colour kCream     { 0xffeee6c8 };
const juce::Colour kCreamDim  { 0xffc8c4b0 };
const juce::Colour kAmber     { 0xffffb03c };
const juce::Colour kRed       { 0xffff3c28 };
const juce::Colour kMenuBg    { 0xff1d2a44 };
const juce::Colour kMenuLine  { 0xff3a5480 };

juce::Image loadImage (const juce::String& file)
{
    int size = 0;
    const auto id = file.replaceCharacter ('.', '_').replaceCharacter ('-', '_');
    if (auto* data = BinaryData::getNamedResource (id.toRawUTF8(), size))
        return juce::ImageCache::getFromMemory (data, size);
    jassertfalse;
    return {};
}

juce::Typeface::Ptr loadFace (const char* id)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource (id, size))
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    return {};
}

const ui::Item* findItem (const char* id)
{
    for (auto& it : ui::items)
        if (juce::String (it.id) == id) return &it;
    return nullptr;
}

juce::Rectangle<float> rectOf (const ui::Item& it) { return { it.x, it.y, it.w, it.h }; }
juce::Rectangle<int> boundsOf (const ui::Item& it) { return rectOf (it).getSmallestIntegerContainer(); }

const juce::String kStar = juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x85"));
} // namespace

// ---------------------------------------------------------------------------------------------
PanelLookAndFeel::PanelLookAndFeel()
{
    cap = loadImage ("knob_cap.png");
    setColour (juce::PopupMenu::backgroundColourId, kMenuBg);
    setColour (juce::PopupMenu::textColourId, kCream);
    setColour (juce::PopupMenu::headerTextColourId, kAmber);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kMenuLine);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf0101418));
    setColour (juce::BubbleComponent::outlineColourId, kMenuLine);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xf0101418));
    setColour (juce::TooltipWindow::textColourId, kCream);
    setColour (juce::TooltipWindow::outlineColourId, kMenuLine);
    setColour (juce::AlertWindow::backgroundColourId, kMenuBg);
    setColour (juce::AlertWindow::textColourId, kCream);
    setColour (juce::AlertWindow::outlineColourId, kMenuLine);
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff101820));
    setColour (juce::TextEditor::textColourId, kCream);
    setColour (juce::TextEditor::outlineColourId, kMenuLine);
    setColour (juce::TextButton::buttonColourId, kMenuLine);
    setColour (juce::TextButton::textColourOffId, kCream);
    setColour (juce::Label::textColourId, kAmber);
}

void PanelLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    // The skirt and scale are printed on the panel; only the cap turns.
    const float d = (float) juce::jmin (w, h);
    const float cx = (float) x + (float) w * 0.5f + ui::capOffX * d;
    const float cy = (float) y + (float) h * 0.5f + ui::capOffY * d;
    const float cw = ui::capScale * d;
    const float angle = start + pos * (end - start);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    const auto t = juce::AffineTransform::translation (-(float) cap.getWidth() * 0.5f, -(float) cap.getHeight() * 0.5f)
                       .rotated (angle)
                       .scaled (cw / (float) cap.getWidth())
                       .translated (cx, cy);
    g.drawImageTransformed (cap, t);
}

juce::Font PanelLookAndFeel::getPopupMenuFont()            { return juce::Font (juce::FontOptions (14.0f, juce::Font::bold)); }
juce::Font PanelLookAndFeel::getSliderPopupFont (juce::Slider&) { return juce::Font (juce::FontOptions (14.0f, juce::Font::bold)); }

// ---------------------------------------------------------------------------------------------
ImgButton::ImgButton (const juce::String& id)
{
    off = loadImage ("btn_" + id + "_0.png");
    on  = loadImage ("btn_" + id + "_1.png");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ImgButton::paint (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (shown ? on : off, getLocalBounds().toFloat().translated (0.0f, down ? 1.0f : 0.0f), juce::RectanglePlacement::stretchToFit);
}

void ImgButton::mouseUp (const juce::MouseEvent& e)
{
    down = false;
    repaint();
    if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
}

bool ImgButton::hitTest (int x, int y)
{
    // the pill plus the LED above it, not the transparent margin
    return getLocalBounds().reduced (6, 2).contains (x, y);
}

// ---------------------------------------------------------------------------------------------
SlideSwitch::SlideSwitch (juce::RangedAudioParameter& p, int positions, float switchWidth)
    : param (p), attachment (p, [this] (float v) { pos = juce::jlimit (0, n - 1, juce::roundToInt (v)); repaint(); }),
      n (positions), swW (switchWidth)
{
    for (int i = 0; i < n; ++i)
        images[i] = loadImage ("sw" + juce::String (n) + "_" + juce::String (i) + ".png");
    attachment.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void SlideSwitch::paint (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (images[pos], juce::Rectangle<float> (0.0f, 0.0f, swW, (float) getHeight()), juce::RectanglePlacement::stretchToFit);
}

void SlideSwitch::setPos (int p)
{
    p = juce::jlimit (0, n - 1, p);
    if (p != pos) attachment.setValueAsCompleteGesture ((float) p);
}

void SlideSwitch::mouseDown (const juce::MouseEvent& e)
{
    setPos ((int) std::floor ((float) e.y / (float) getHeight() * (float) n));
}

void SlideSwitch::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (w.deltaY != 0.0f) setPos (pos + ((w.deltaY * (w.isReversed ? -1.0f : 1.0f)) > 0 ? -1 : 1));
}

// ---------------------------------------------------------------------------------------------
void PerfWheel::set (float v)
{
    v = juce::jlimit (spring ? -1.0f : 0.0f, 1.0f, v);
    if (v == value) return;
    value = v;
    if (onChange) onChange (value);
    repaint();
}

void PerfWheel::mouseDown (const juce::MouseEvent&) { dragging = true; startValue = value; }

void PerfWheel::mouseDrag (const juce::MouseEvent& e)
{
    const float travel = (float) getHeight() * 0.7f;
    set (startValue - (float) e.getDistanceFromDragStartY() / travel * (spring ? 2.0f : 1.0f));
}

void PerfWheel::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
    if (spring) set (0.0f);
}

void PerfWheel::paint (juce::Graphics& g)
{
    // the rubber wheel in the picture sits in the left part of its slot
    const auto slot = getLocalBounds().toFloat();
    const auto wheel = juce::Rectangle<float> (slot.getX() + slot.getWidth() * 0.12f, slot.getY() + slot.getHeight() * 0.12f,
                                               slot.getWidth() * 0.62f, slot.getHeight() * 0.76f);
    const float travel = wheel.getHeight() - 8.0f;
    const float y = spring ? wheel.getCentreY() - value * travel * 0.5f : wheel.getBottom() - 4.0f - value * travel;

    // the wheel looks rounded, so the mark narrows and fades near the ends
    const float edge = std::abs (y - wheel.getCentreY()) / (wheel.getHeight() * 0.5f);
    const float alpha = juce::jlimit (0.25f, 1.0f, 1.0f - edge * edge * 0.75f);
    g.setColour (juce::Colours::black.withAlpha (0.45f * alpha));
    g.fillRoundedRectangle (wheel.getX() + 2.0f, y - 1.0f, wheel.getWidth() - 4.0f, 4.0f, 1.5f);
    g.setColour (kCream.withAlpha (0.95f * alpha));
    g.fillRoundedRectangle (wheel.getX() + 2.0f, y - 2.0f, wheel.getWidth() - 4.0f, 3.0f, 1.5f);

    if (spring) // centre detent marks beside the wheel
    {
        g.setColour (kCream.withAlpha (0.6f));
        g.fillRect (slot.getRight() - 6.0f, wheel.getCentreY() - 1.0f, 5.0f, 2.0f);
    }
}

// ---------------------------------------------------------------------------------------------
void StepArea::mouseDrag (const juce::MouseEvent& e)
{
    const float y = (float) e.getDistanceFromDragStartY() - (float) e.getDistanceFromDragStartX();
    if (std::abs (y) > 3.0f) moved = true;
    const float d = (lastY - y) / 12.0f;
    if (! moved) return;
    acc += d;
    lastY = y;
    while (acc >= 1.0f)  { acc -= 1.0f; if (step) step (1); }
    while (acc <= -1.0f) { acc += 1.0f; if (step) step (-1); }
}

// ---------------------------------------------------------------------------------------------
AboutOverlay::AboutOverlay() { setInterceptsMouseClicks (true, false); }

juce::Rectangle<float> AboutOverlay::box() const
{
    return getLocalBounds().toFloat().withSizeKeepingCentre (600.0f, 420.0f);
}

void AboutOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.6f));
    const auto b = box();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (b.translated (0.0f, 8.0f), 10.0f);
    juce::ColourGradient grad (juce::Colour (0xff2f4c6e), b.getX(), b.getY(), juce::Colour (0xff22334f), b.getX(), b.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (juce::Colour (0xff141e30));
    g.drawRoundedRectangle (b, 10.0f, 2.0f);

    const float x = b.getX() + 44.0f, w = b.getWidth() - 88.0f;
    auto text = [&] (const juce::String& s, float y, float h, float size, juce::Colour c, bool bold = true)
    {
        g.setColour (c);
        g.setFont (juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)));
        g.drawText (s, juce::Rectangle<float> (x, y, w, h), juce::Justification::centredLeft);
    };
    text ("TICCO ROSS", b.getY() + 32.0f, 20.0f, 15.0f, kAmber);
    g.setColour (kCream);
    g.setFont (juce::Font (juce::FontOptions (54.0f, juce::Font::bold)).withHorizontalScale (0.8f));
    g.drawText ("POLYTWELVE", juce::Rectangle<float> (x - 2.0f, b.getY() + 52.0f, w, 58.0f), juce::Justification::centredLeft);
    text (juce::String ("Version ") + JucePlugin_VersionString, b.getY() + 110.0f, 18.0f, 14.0f, kCreamDim, false);
    g.setColour (kAmber.withAlpha (0.5f));
    g.fillRect (x, b.getY() + 142.0f, w, 1.0f);
    const char* lines[] = {
        "Programmable 12-voice analog-style synthesizer.",
        "AU / VST3 / LV2 and standalone, for macOS, Windows and Linux.",
        "",
        "Chord memory, arpeggiator, Chorus / Phaser / Ensemble,",
        "preset banks with favourites.",
        "",
        "Designed by Transistor Labs, 2026.",
    };
    float y = b.getY() + 156.0f;
    for (auto* l : lines) { text (l, y, 20.0f, 14.5f, kCream, false); y += 21.0f; }

    mailR = { x, b.getBottom() - 92.0f, w, 24.0f };
    shopR = { x, b.getBottom() - 64.0f, w, 24.0f };
    text ("ticcoross@gmail.com", mailR.getY(), 24.0f, 17.0f, kAmber);
    text ("ticcoross.gumroad.com", shopR.getY(), 24.0f, 17.0f, kAmber);
    g.setColour (kCreamDim.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (11.5f)));
    g.drawText ("Click anywhere to close", b.reduced (20.0f, 14.0f), juce::Justification::bottomRight);
}

void AboutOverlay::mouseUp (const juce::MouseEvent& e)
{
    const auto p = e.position;
    if (mailR.withWidth (220.0f).contains (p))
        juce::URL ("mailto:ticcoross@gmail.com?subject=Polytwelve%20" + juce::String (JucePlugin_VersionString)).launchInDefaultBrowser();
    else if (shopR.withWidth (240.0f).contains (p))
        juce::URL ("https://ticcoross.gumroad.com/").launchInDefaultBrowser();
    else
        setVisible (false);
}

// ---------------------------------------------------------------------------------------------
namespace
{
// Ableton convention: MIDI 60 = C3
juce::String noteName (int midi)
{
    static const char* names[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return juce::String (names[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 2);
}

juce::String pitchClass (int pc)
{
    static const char* names[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return names[((pc % 12) + 12) % 12];
}

// Name a chord from its notes; tries every note as the root and adds "/bass" for inversions
juce::String chordName (const int* intervals, int n, int bass)
{
    struct Shape { std::vector<int> pcs; const char* suffix; };
    static const std::vector<Shape> shapes {
        { { 0, 4, 7 }, "" },        { { 0, 3, 7 }, "m" },        { { 0, 3, 6 }, "dim" },     { { 0, 4, 8 }, "aug" },
        { { 0, 2, 7 }, "sus2" },    { { 0, 5, 7 }, "sus4" },     { { 0, 7 }, "5" },
        { { 0, 4, 7, 10 }, "7" },   { { 0, 4, 7, 11 }, "maj7" }, { { 0, 3, 7, 10 }, "m7" },  { { 0, 3, 6, 10 }, "m7b5" },
        { { 0, 3, 6, 9 }, "dim7" }, { { 0, 4, 7, 9 }, "6" },     { { 0, 3, 7, 9 }, "m6" },   { { 0, 3, 7, 11 }, "mMaj7" },
        { { 0, 5, 7, 10 }, "7sus4" },
        { { 0, 2, 4, 7 }, "add9" }, { { 0, 2, 3, 7 }, "m(add9)" },
        { { 0, 2, 4, 7, 10 }, "9" }, { { 0, 2, 4, 7, 11 }, "maj9" }, { { 0, 2, 3, 7, 10 }, "m9" },
        { { 0, 3, 5, 7, 10 }, "m11" }, { { 0, 2, 3, 5, 7, 10 }, "m11" }, { { 0, 4, 7, 9, 2 }, "6/9" },
    };

    std::vector<int> pcs;
    for (int i = 0; i < n; ++i) { const int pc = (intervals[i] % 12 + 12) % 12; if (std::find (pcs.begin(), pcs.end(), pc) == pcs.end()) pcs.push_back (pc); }
    std::sort (pcs.begin(), pcs.end());
    if (pcs.size() == 1) return noteName (bass) + (n > 1 ? " oct" : "");

    for (int rootPc : pcs)
    {
        std::vector<int> rel;
        for (int pc : pcs) rel.push_back (((pc - rootPc) % 12 + 12) % 12);
        std::sort (rel.begin(), rel.end());
        for (const auto& s : shapes)
        {
            auto want = s.pcs; std::sort (want.begin(), want.end());
            if (want == rel)
            {
                juce::String nm = pitchClass (bass + rootPc) + s.suffix;
                if (rootPc != 0) nm << "/" << pitchClass (bass);
                return nm;
            }
        }
    }
    return juce::String (n) + "-note chord";
}
} // namespace

// ---------------------------------------------------------------------------------------------
PolysixEditor::PolysixEditor (PolysixAudioProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    background = loadImage ("panel.jpg");
    monoFace = loadFace ("DejaVuSansMonoBold_ttf");
    addAndMakeVisible (canvas);

    auto& st = proc.apvts;
    for (auto& it : ui::items)
    {
        const juce::String id (it.id);
        switch (it.kind)
        {
            case ui::Kind::Knob:
            {
                auto sl = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
                sl->setRotaryParameters (juce::degreesToRadians (-135.0f), juce::degreesToRadians (135.0f), true);
                sl->setPopupDisplayEnabled (true, true, &canvas);
                sl->setMouseDragSensitivity (220);
                sl->setBounds (boundsOf (it));
                knobAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, id, *sl));
                if (auto* prm = st.getParameter (id))
                {
                    sl->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
                    sl->setTooltip (prm->getName (64));
                }
                canvas.addAndMakeVisible (*sl);
                knobs.push_back (std::move (sl));
                break;
            }
            case ui::Kind::Switch:
            {
                auto* prm = st.getParameter (id);
                jassert (prm != nullptr);
                auto sw = std::make_unique<SlideSwitch> (*prm, it.extra, it.w);
                sw->setBounds (juce::Rectangle<float> (it.x, it.y, it.hitW, it.h).getSmallestIntegerContainer());
                sw->setTooltip (prm->getName (64));
                canvas.addAndMakeVisible (*sw);
                others.push_back (std::move (sw));
                break;
            }
            case ui::Kind::Button:
                addButton (it);
                break;
            case ui::Kind::Wheel:
            {
                const bool isPitch = id == "pitchWheel";
                auto w = std::make_unique<PerfWheel> (isPitch);
                (isPitch ? pitchWheel : modWheel) = w.get();
                if (isPitch)
                {
                    w->setTooltip ("Pitch bend: drag up or down; springs back to the centre");
                    w->onChange = [this] (float v) { proc.setUiPitchBend (v); };
                }
                else
                {
                    w->setTooltip ("Modulation wheel: adds MG vibrato");
                    w->onChange = [this] (float v) { proc.setUiModWheel (v); };
                }
                w->setBounds (boundsOf (it));
                canvas.addAndMakeVisible (*w);
                others.push_back (std::move (w));
                break;
            }
            case ui::Kind::Display:
            {
                auto r = rectOf (it);
                auto area = std::make_unique<StepArea>();
                area->setBounds (r.getSmallestIntegerContainer());
                if (id == "preset")
                {
                    presetR = r;
                    area->click = [this] { showPresetMenu(); };
                    area->step = [this] (int d) { proc.getPresetManager().step (d, favouritesOnly); };
                    area->setTooltip ("Click to browse presets; scroll or drag to step through them");
                }
                else if (id == "chord")
                {
                    chordR = r;
                    area->setTooltip ("The stored chord: hold a chord and press MEMORY to store a new one");
                }
                else if (id == "arpDiv")
                {
                    arpDivR = r;
                    area->step = [this] (int d) { stepParam ("arpDiv", d); };
                    area->setTooltip ("Arpeggiator step length when SYNC is on: drag, scroll or use the arrows");
                }
                else if (id == "bendRange")
                {
                    bendR = r;
                    area->step = [this] (int d) { stepParam ("bendRange", d); };
                    area->setTooltip ("Pitch wheel range in semitones: drag, scroll or use the arrows");
                }
                area->setMouseCursor (juce::MouseCursor::PointingHandCursor);
                canvas.addAndMakeVisible (*area);
                others.push_back (std::move (area));
                break;
            }
            case ui::Kind::Logo:
            {
                auto area = std::make_unique<StepArea>();
                area->setBounds (boundsOf (it));
                area->click = [this] { about.setVisible (true); about.toFront (false); };
                area->setTooltip ("About Polytwelve");
                area->setMouseCursor (juce::MouseCursor::PointingHandCursor);
                canvas.addAndMakeVisible (*area);
                others.push_back (std::move (area));
                break;
            }
            case ui::Kind::Info:
                infoR = rectOf (it);
                break;
        }
    }

    canvas.addChildComponent (about);
    about.setBounds (0, 0, ui::designW, ui::designH);

    auto& pm = proc.getPresetManager();
    pm.addChangeListener (this);
    pm.refresh();

    refreshChord();
    refreshButtons();
    pitchWheel->setDisplayValue (proc.getPitchBendDisplay());
    modWheel->setDisplayValue (proc.getModWheelDisplay());

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) ui::designW / (double) ui::designH);
    setResizeLimits (720, 450, 2160, 1350);
    setSize (1152, 720);
    startTimerHz (20);
}

PolysixEditor::~PolysixEditor()
{
    stopTimer();
    proc.getPresetManager().removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void PolysixEditor::setValue (const juce::String& id, float v)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (v));
        prm->endChangeGesture();
    }
}

void PolysixEditor::stepParam (const char* id, int delta)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        const auto range = prm->getNormalisableRange();
        const float cur = range.convertFrom0to1 (prm->getValue());
        setValue (id, juce::jlimit (range.start, range.end, std::round (cur) + (float) delta));
        canvas.repaint();
    }
}

ImgButton& PolysixEditor::addButton (const ui::Item& it)
{
    const juce::String id (it.id);
    auto b = std::make_unique<ImgButton> (id);
    auto& btn = *b;
    btn.setBounds (boundsOf (it));

    auto toggle = [this, &btn] (const char* pid, const char* tip)
    {
        btn.isOn = [this, pid] { return value (pid) > 0.5f; };
        btn.onClick = [this, pid] { setValue (pid, value (pid) > 0.5f ? 0.0f : 1.0f); refreshButtons(); };
        btn.setTooltip (tip);
    };

    if (id == "hold")             toggle ("hold", "Hold: notes keep sounding after you let go (sustain pedal does the same)");
    else if (id == "glideLegato") toggle ("glideLegato", "Legato: glide only between overlapping notes");
    else if (id == "arpOn")       toggle ("arpOn", "Arpeggiator on / off");
    else if (id == "arpLatch")    toggle ("arpLatch", "Latch: the arpeggio keeps running after you let go");
    else if (id == "arpSync")     toggle ("arpSync", "Sync: follow the DAW tempo, at the SYNC RATE below");
    else if (id == "memory")
    {
        btn.isOn = [this] { return value ("voiceMode") > 1.5f && (flashTicks == 0 || (flashTicks % 2) == 1); };
        btn.onClick = [this]
        {
            proc.requestChordMemory();
            setValue ("voiceMode", 2.0f);
            flashTicks = 6;
        };
        btn.setTooltip ("Chord Memory: hold a chord and press to store it (switches to Chord mode)");
    }
    else if (id.startsWith ("fx"))
    {
        const int mode = id.getTrailingIntValue();
        btn.isOn = [this, mode] { return juce::roundToInt (value ("fxMode")) == mode; };
        btn.onClick = [this, mode] { setValue ("fxMode", (float) mode); refreshButtons(); };
        btn.setTooltip ("Effect: Off, Chorus, Phaser or Ensemble");
    }
    else if (id == "arpDivDown" || id == "arpDivUp")
        btn.onClick = [this, d = id.endsWith ("Up") ? 1 : -1] { stepParam ("arpDiv", d); };
    else if (id == "bendRangeDown" || id == "bendRangeUp")
        btn.onClick = [this, d = id.endsWith ("Up") ? 1 : -1] { stepParam ("bendRange", d); };
    else if (id == "presetPrev") { btn.onClick = [this] { proc.getPresetManager().step (-1, favouritesOnly); }; btn.setTooltip ("Previous preset"); }
    else if (id == "presetNext") { btn.onClick = [this] { proc.getPresetManager().step (+1, favouritesOnly); }; btn.setTooltip ("Next preset"); }
    else if (id == "presetSave") { btn.onClick = [this] { askSaveName(); }; btn.setTooltip ("Save the current sound as a user preset"); }
    else if (id == "presetBank") { btn.onClick = [this] { showPresetMenu(); }; btn.setTooltip ("Preset browser: Favourites, Factory and User"); }
    else if (id == "presetFav")
    {
        btn.isOn = [this] { auto& m = proc.getPresetManager(); return m.isFavourite (m.getCurrentKey()); };
        btn.onClick = [this] { auto& m = proc.getPresetManager(); m.toggleFavourite (m.getCurrentKey()); refreshButtons(); };
        btn.setTooltip ("Add / remove this preset from favourites");
    }

    canvas.addAndMakeVisible (btn);
    buttons.push_back (&btn);
    others.push_back (std::move (b));
    return btn;
}

void PolysixEditor::refreshButtons()
{
    for (auto* b : buttons) b->refresh();
}

void PolysixEditor::refreshChord()
{
    int iv[p6::kNumVoices]; int root = 60;
    const int n = proc.getChord (iv, root);
    juce::StringArray parts;
    for (int i = 0; i < n; ++i) parts.add (noteName (root + iv[i]));
    const auto name = chordName (iv, n, root);
    const auto notes = parts.joinIntoString (" ");
    if (name != chordNameText || notes != chordNotesText)
    {
        chordNameText = name; chordNotesText = notes;
        canvas.repaint (chordR.getSmallestIntegerContainer().expanded (4));
    }
}

void PolysixEditor::timerCallback()
{
    if (flashTicks > 0) --flashTicks;
    refreshButtons();
    pitchWheel->setDisplayValue (proc.getPitchBendDisplay());
    modWheel->setDisplayValue (proc.getModWheelDisplay());

    const int v = proc.getChordVersion();
    if (v != lastChordVersion) { lastChordVersion = v; refreshChord(); }

    // displays follow automation and preset changes
    const float now[4] = { value ("arpDiv"), value ("bendRange"), value ("arpSync"), value ("voiceMode") };
    bool changed = false;
    for (int i = 0; i < 4; ++i) if (now[i] != lastShown[i]) { lastShown[i] = now[i]; changed = true; }
    if (changed) canvas.repaint();
}

void PolysixEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffd6d6d4));
}

void PolysixEditor::resized()
{
    const float scale = (float) getWidth() / (float) ui::designW;
    canvas.setBounds (0, 0, ui::designW, ui::designH);
    canvas.setTransform (juce::AffineTransform::scale (scale));
}

void PanelCanvas::paint (juce::Graphics& g)
{
    auto& e = editor;
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (e.background, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);

    auto mono = [&] (float size) { return e.monoFace != nullptr ? juce::Font (juce::FontOptions (e.monoFace).withHeight (size))
                                                                 : juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::bold)); };
    auto glowText = [&] (const juce::String& s, juce::Rectangle<float> r, juce::Font f, juce::Colour c, juce::Justification j)
    {
        g.setFont (f);
        g.setColour (c.withAlpha (0.18f));
        for (auto d : { juce::Point<float> (-1.2f, 0), juce::Point<float> (1.2f, 0), juce::Point<float> (0, -1.2f), juce::Point<float> (0, 1.2f) })
            g.drawText (s, r.translated (d.x, d.y), j, false);
        g.setColour (c);
        g.drawText (s, r, j, false);
    };

    // preset display
    {
        auto& pm = e.proc.getPresetManager();
        const auto key = pm.getCurrentKey();
        const int idx = pm.indexOfKey (key);
        juce::String num;
        if (idx >= 0)
        {
            int n = 0;
            for (int i = 0; i <= idx; ++i) if (pm.getEntries()[(size_t) i].factory == pm.getEntries()[(size_t) idx].factory) ++n;
            num = juce::String (n).paddedLeft ('0', 2) + " ";
        }
        const juce::String txt = (pm.currentIsUser() ? "USER " : "FACTORY ") + num + pm.getCurrentName().toUpperCase();
        auto f = mono (21.0f);
        g.saveState();
        g.reduceClipRegion (e.presetR.reduced (6.0f, 4.0f).getSmallestIntegerContainer());
        glowText (txt, e.presetR.reduced (12.0f, 2.0f), f, kAmber, juce::Justification::centredLeft);
        if (e.favouritesOnly)
            glowText (kStar + " ONLY", e.presetR.reduced (10.0f, 2.0f), mono (12.0f), kAmber.withAlpha (0.8f), juce::Justification::centredRight);
        g.restoreState();
    }

    // stored chord: bright in Chord mode, dim otherwise
    {
        const bool chordMode = e.value ("voiceMode") > 1.5f;
        const auto c = kAmber.withAlpha (chordMode ? 1.0f : 0.4f);
        auto r = e.chordR.reduced (8.0f, 5.0f);
        glowText (e.chordNameText, r.removeFromTop (r.getHeight() * 0.58f), mono (22.0f), c, juce::Justification::centredLeft);
        g.setColour (c);
        g.setFont (mono (11.0f));
        g.drawFittedText (e.chordNotesText, r.getSmallestIntegerContainer(), juce::Justification::centredLeft, 1, 0.6f);
    }

    // arpeggiator sync rate (dim while SYNC is off) and bend range
    {
        juce::String div = "1/16";
        if (auto* prm = dynamic_cast<juce::AudioParameterChoice*> (e.proc.apvts.getParameter ("arpDiv")))
            div = prm->getCurrentChoiceName();
        glowText (div, e.arpDivR, mono (21.0f), kRed.withAlpha (e.value ("arpSync") > 0.5f ? 1.0f : 0.35f), juce::Justification::centred);
        glowText ("BEND " + juce::String (juce::roundToInt (e.value ("bendRange"))), e.bendR, mono (19.0f), kRed, juce::Justification::centred);
    }

    // credit line between the preset strip and the logo
    {
        g.setColour (kCreamDim);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText ("PROGRAMMABLE 12 VOICE SYNTHESIZER", e.infoR.removeFromTop (17.0f), juce::Justification::centredRight);
        g.setColour (kCreamDim.withAlpha (0.8f));
        g.drawText (juce::String (juce::CharPointer_UTF8 ("TICCO ROSS  \xc2\xb7  v")) + JucePlugin_VersionString,
                    e.infoR, juce::Justification::centredRight);
    }
}

// ---- presets ---------------------------------------------------------------------------------
void PolysixEditor::showPresetMenu()
{
    auto& pm = proc.getPresetManager();
    pm.refresh();
    const auto& entries = pm.getEntries();
    const auto current = pm.getCurrentKey();

    juce::PopupMenu menu;
    static constexpr int favBase = 10000;
    auto label = [&] (const PresetManager::Entry& en) { return (pm.isFavourite (en.key) ? kStar + " " : juce::String()) + en.name; };

    if (pm.hasFavourites())
    {
        menu.addSectionHeader ("Favourites");
        for (size_t i = 0; i < entries.size(); ++i)
            if (pm.isFavourite (entries[i].key))
                menu.addItem (favBase + (int) i, entries[i].name, true, entries[i].key == current);
        menu.addSeparator();
    }
    menu.addSectionHeader ("Factory");
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].factory) menu.addItem ((int) i + 1, label (entries[i]), true, entries[i].key == current);
    menu.addSectionHeader ("User");
    bool anyUser = false;
    for (size_t i = 0; i < entries.size(); ++i)
        if (! entries[i].factory) { anyUser = true; menu.addItem ((int) i + 1, label (entries[i]), true, entries[i].key == current); }
    if (! anyUser) menu.addItem (-1, "(no saved presets yet)", false);

    menu.addSeparator();
    menu.addItem (90001, "Save As...");
    menu.addItem (90004, "Delete \"" + pm.getCurrentName() + "\"...", pm.currentIsUser() && pm.indexOfKey (current) >= 0);
    menu.addItem (90005, "Step Through Favourites Only", true, favouritesOnly);
    menu.addSeparator();
    menu.addItem (90002, "Open Preset File...");
    menu.addItem (90003, "Show Preset Folder");

    const auto anchor = findItem ("presetBank");
    juce::Rectangle<int> target = anchor != nullptr ? canvas.localAreaToGlobal (boundsOf (*anchor)) : getScreenBounds();
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (target).withParentComponent (nullptr),
        [this] (int r)
        {
            auto& m = proc.getPresetManager();
            if (r == 90001)      askSaveName();
            else if (r == 90002) openPresetFile();
            else if (r == 90003) { m.getUserDir().createDirectory(); m.getUserDir().revealToUser(); }
            else if (r == 90004) askDelete();
            else if (r == 90005) { favouritesOnly = ! favouritesOnly; canvas.repaint(); }
            else if (r >= favBase) m.load (r - favBase);
            else if (r > 0)        m.load (r - 1);
            refreshButtons();
        });
}

void PolysixEditor::askSaveName()
{
    auto& pm = proc.getPresetManager();
    auto* w = new juce::AlertWindow ("Save Preset", "Name for this preset:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", pm.currentIsUser() ? pm.getCurrentName() : juce::String());
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PolysixEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result != 1 || safe == nullptr) return;
        const auto name = w->getTextEditorContents ("name").trim();
        if (name.isNotEmpty()) safe->saveWithName (name);
    }), true);
}

void PolysixEditor::saveWithName (const juce::String& name)
{
    auto& pm = proc.getPresetManager();
    if (! pm.userPresetExists (name)) { pm.save (name); return; }

    juce::Component::SafePointer<PolysixEditor> safe (this);
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withTitle ("Overwrite Preset")
                                      .withMessage ("\"" + name + "\" already exists. Replace it?")
                                      .withButton ("Replace")
                                      .withButton ("Cancel")
                                      .withAssociatedComponent (this),
                                  [safe, name] (int r) { if (r == 1 && safe != nullptr) safe->proc.getPresetManager().save (name); });
}

void PolysixEditor::askDelete()
{
    auto& pm = proc.getPresetManager();
    if (! pm.currentIsUser()) return;
    juce::Component::SafePointer<PolysixEditor> safe (this);
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withTitle ("Delete Preset")
                                      .withMessage ("Delete \"" + pm.getCurrentName() + "\"? This can't be undone.")
                                      .withButton ("Delete")
                                      .withButton ("Cancel")
                                      .withAssociatedComponent (this),
                                  [safe] (int r) { if (r == 1 && safe != nullptr) safe->proc.getPresetManager().deleteCurrent(); });
}

void PolysixEditor::openPresetFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Open Polytwelve preset", proc.getPresetManager().getUserDir(),
                                                   juce::String ("*") + PresetManager::extension);
    juce::Component::SafePointer<PolysixEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (safe != nullptr && f.existsAsFile()) safe->proc.getPresetManager().loadFile (f);
                          });
}
