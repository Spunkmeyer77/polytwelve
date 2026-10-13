#include "PluginEditor.h"

namespace
{
const juce::Colour kPanel     { 0xff1c2347 };
const juce::Colour kPanelDark { 0xff141a37 };
const juce::Colour kLine      { 0xff3a4475 };
const juce::Colour kText      { 0xffe9ecf5 };
const juce::Colour kWood      { 0xff6b3a1e };
const juce::Colour kLed       { 0xffff3b2f };

constexpr int kCellW = 78, kCellH = 96, kSecPad = 8, kTitleH = 20, kRowGap = 12, kMargin = 34, kTopH = 44;
}

// ---------------------------------------------------------------------------------------------
PolysixLookAndFeel::PolysixLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, kPanelDark);
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::outlineColourId, kLine);
    setColour (juce::ComboBox::arrowColourId, kText);
    setColour (juce::PopupMenu::backgroundColourId, kPanelDark);
    setColour (juce::PopupMenu::textColourId, kText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kLine);
    setColour (juce::BubbleComponent::backgroundColourId, kPanelDark);
    setColour (juce::BubbleComponent::outlineColourId, kLine);
    setColour (juce::TooltipWindow::textColourId, kText);
    setColour (juce::TextButton::buttonColourId, kPanelDark);
    setColour (juce::TextButton::buttonOnColourId, kLine);
    setColour (juce::TextButton::textColourOffId, kText);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::AlertWindow::backgroundColourId, kPanelDark);
    setColour (juce::AlertWindow::textColourId, kText);
    setColour (juce::AlertWindow::outlineColourId, kLine);
    setColour (juce::TextEditor::backgroundColourId, kPanel);
    setColour (juce::TextEditor::textColourId, kText);
    setColour (juce::TextEditor::outlineColourId, kLine);
}

void PolysixLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    const auto area   = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float r     = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto centre = area.getCentre();

    // scale ticks
    g.setColour (kText.withAlpha (0.7f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        const auto p1 = centre.getPointOnCircumference (r + 1.0f, a);
        const auto p2 = centre.getPointOnCircumference (r + (i % 5 == 0 ? 5.0f : 3.5f), a);
        g.drawLine ({ p1, p2 }, 1.0f);
    }

    // white knob cap with a skirt
    const float kr = r * 0.82f;
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (centre.x - kr + 1.5f, centre.y - kr + 2.5f, kr * 2.0f, kr * 2.0f);
    juce::ColourGradient grad (juce::Colour (0xfffafafa), centre.x, centre.y - kr, juce::Colour (0xffbfc3cc), centre.x, centre.y + kr, false);
    g.setGradientFill (grad);
    g.fillEllipse (centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (juce::Colour (0xff8a8f9c));
    g.drawEllipse (centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

    // pointer
    const float a = start + pos * (end - start);
    g.setColour (juce::Colour (0xff15182a));
    g.drawLine ({ centre.getPointOnCircumference (kr * 0.25f, a), centre.getPointOnCircumference (kr * 0.92f, a) }, 2.4f);
}

void PolysixLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    // white switch with LED, like the Mono/Poly-style buttons in the ensemble
    auto area = b.getLocalBounds().toFloat();
    auto sw = juce::Rectangle<float> (0, 0, 30.0f, 22.0f).withCentre (area.getCentre());
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (sw.translated (1.0f, 2.0f), 3.0f);
    g.setColour (highlighted ? juce::Colour (0xffffffff) : juce::Colour (0xffe4e6ec));
    g.fillRoundedRectangle (sw, 3.0f);
    const auto led = juce::Rectangle<float> (0, 0, 8.0f, 8.0f).withCentre (sw.getCentre());
    g.setColour (b.getToggleState() ? kLed : juce::Colour (0xff5a2420));
    g.fillEllipse (led);
    if (b.getToggleState())
    {
        g.setColour (kLed.withAlpha (0.35f));
        g.fillEllipse (led.expanded (3.0f));
    }
}

void PolysixLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 10.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (kText.withAlpha (0.8f));
    g.fillPath (arrow);
}

juce::Font PolysixLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (11.5f, juce::Font::bold));
}

void PolysixLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (2, 1, box.getWidth() - 16, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setMinimumHorizontalScale (0.8f);
}

// ---------------------------------------------------------------------------------------------
PolysixEditor::PolysixEditor (PolysixAudioProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);

    auto& master = addSection ("MASTER", 0);
    add (master, Kind::Knob, "volume", "VOLUME");
    add (master, Kind::Knob, "tune", "TUNE");

    auto& vco = addSection ("VCO", 0);
    add (vco, Kind::Choice, "octave", "OCTAVE");
    add (vco, Kind::Choice, "waveform", "WAVEFORM");
    add (vco, Kind::Knob, "pwPwm", "PW/PWM");
    add (vco, Kind::Knob, "pwmSpeed", "PWM SPEED");
    add (vco, Kind::Choice, "subOsc", "SUB OSC");

    auto& vcf = addSection ("VCF", 0);
    add (vcf, Kind::Knob, "cutoff", "CUTOFF");
    add (vcf, Kind::Knob, "resonance", "RESONANCE");
    add (vcf, Kind::Knob, "egInt", "EG INTENSITY");
    add (vcf, Kind::Knob, "kbdTrack", "KBD TRACK");

    auto& vca = addSection ("VCA", 0);
    add (vca, Kind::Choice, "vcaMode", "MODE");
    add (vca, Kind::Knob, "atten", "ATTENUATOR");

    auto& eg = addSection ("EG", 1);
    add (eg, Kind::Knob, "attack", "ATTACK");
    add (eg, Kind::Knob, "decay", "DECAY");
    add (eg, Kind::Knob, "sustain", "SUSTAIN");
    add (eg, Kind::Knob, "release", "RELEASE");

    auto& mg = addSection ("MG", 1);
    add (mg, Kind::Knob, "mgFreq", "FREQUENCY");
    add (mg, Kind::Knob, "mgDelay", "DELAY");
    add (mg, Kind::Knob, "mgLevel", "LEVEL");
    add (mg, Kind::Choice, "mgMode", "MODE");

    auto& voice = addSection ("VOICE", 1);
    add (voice, Kind::Choice, "voiceMode", "MODE");
    {
        auto b = std::make_unique<LedButton>();
        memoryBtn = b.get();
        memoryBtn->setTooltip ("Chord Memory: hold a chord and press to store it (switches to Chord mode)");
        memoryBtn->onClick = [this]
        {
            proc.requestChordMemory();
            if (auto* vm = proc.apvts.getParameter ("voiceMode"))
                vm->setValueNotifyingHost (vm->convertTo0to1 (2.0f));
            flashTicks = 4;
        };
        addCustom (voice, std::move (b), "MEMORY", 1);
    }
    add (voice, Kind::Toggle, "hold", "HOLD");
    add (voice, Kind::Knob, "analog", "ANALOG");
    add (voice, Kind::Knob, "detune", "DETUNE");
    add (voice, Kind::Knob, "bendRange", "BEND");

    auto& arp = addSection ("ARPEGGIATOR", 2);
    add (arp, Kind::Toggle, "arpOn", "ARPEGGIO");
    add (arp, Kind::Knob, "arpSpeed", "SPEED");
    add (arp, Kind::Choice, "arpRange", "RANGE");
    add (arp, Kind::Choice, "arpMode", "MODE");
    add (arp, Kind::Toggle, "arpLatch", "LATCH");
    add (arp, Kind::Toggle, "arpSync", "SYNC");
    add (arp, Kind::Choice, "arpDiv", "SYNC RATE");

    auto& fx = addSection ("EFFECTS", 2);
    add (fx, Kind::Choice, "fxMode", "MODE");
    add (fx, Kind::Knob, "fxSpeed", "SPEED/INT");

    auto& chord = addSection ("CHORD MEMORY", 2);
    {
        auto d = std::make_unique<ChordDisplay>();
        chordDisplay = d.get();
        addCustom (chord, std::move (d), "STORED CHORD", 4);
    }

    // program selector
    auto& pm = proc.getPresetManager();
    prevBtn.onClick    = [this] { proc.getPresetManager().step (-1, favouritesOnly); };
    nextBtn.onClick    = [this] { proc.getPresetManager().step (+1, favouritesOnly); };
    nameBtn.onClick    = [this] { showPresetMenu(); };
    favBtn.onClick     = [this] { auto& m = proc.getPresetManager(); m.toggleFavourite (m.getCurrentKey()); };
    saveBtn.onClick    = [this] { askSaveName(); };
    delBtn.onClick     = [this] { askDelete(); };
    favOnlyBtn.setClickingTogglesState (true);
    favOnlyBtn.onClick = [this] { favouritesOnly = favOnlyBtn.getToggleState(); updatePresetBar(); };
    prevBtn.setTooltip ("Previous preset");
    nextBtn.setTooltip ("Next preset");
    favBtn.setTooltip ("Add / remove this preset from favourites");
    favOnlyBtn.setTooltip ("Step through favourites only");
    for (auto* b : { &prevBtn, &nextBtn, &nameBtn, &favBtn, &saveBtn, &delBtn, &favOnlyBtn }) addAndMakeVisible (b);
    pm.addChangeListener (this);
    pm.refresh();
    updatePresetBar();

    // size from the widest row
    int widest = 0;
    for (int row = 0; row < 3; ++row)
    {
        int w = 0;
        for (auto& s : sections)
            if (s.row == row)
            {
                int cells = 0; for (auto& c : s.ctrls) cells += c->span;
                w += cells * kCellW + 2 * kSecPad + 10;
            }
        widest = juce::jmax (widest, w);
    }
    setSize (widest - 10 + 2 * kMargin, kTopH + 3 * (kCellH + kTitleH + 2 * kSecPad) + 2 * kRowGap + 20);

    refreshChordDisplay();
    startTimerHz (15);
}

PolysixEditor::~PolysixEditor()
{
    stopTimer();
    proc.getPresetManager().removeChangeListener (this);
    setLookAndFeel (nullptr);
}

// ---- preset bar -------------------------------------------------------------------------------
namespace
{
const juce::String kStar      = juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x85"));
const juce::String kStarEmpty = juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x86"));
}

void PolysixEditor::updatePresetBar()
{
    auto& pm = proc.getPresetManager();
    const bool fav = pm.isFavourite (pm.getCurrentKey());
    nameBtn.setButtonText (pm.getCurrentName());
    favBtn.setButtonText (fav ? kStar : kStarEmpty);
    favBtn.setColour (juce::TextButton::textColourOffId, fav ? juce::Colour (0xffffc83d) : kText);
    favOnlyBtn.setButtonText (kStar + " ONLY");
    favOnlyBtn.setToggleState (favouritesOnly, juce::dontSendNotification);
    delBtn.setEnabled (pm.currentIsUser() && pm.indexOfKey (pm.getCurrentKey()) >= 0);
}

void PolysixEditor::showPresetMenu()
{
    auto& pm = proc.getPresetManager();
    pm.refresh();
    updatePresetBar();
    const auto& entries = pm.getEntries();
    const auto current = pm.getCurrentKey();

    juce::PopupMenu menu;
    constexpr int favBase = 10000;
    auto label = [&] (const PresetManager::Entry& e) { return (pm.isFavourite (e.key) ? kStar + " " : juce::String()) + e.name; };

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
    menu.addItem (90002, "Open Preset File...");
    menu.addItem (90003, "Show Preset Folder");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameBtn),
        [this] (int r)
        {
            auto& m = proc.getPresetManager();
            if (r == 90001)      askSaveName();
            else if (r == 90002) openPresetFile();
            else if (r == 90003) { m.getUserDir().createDirectory(); m.getUserDir().revealToUser(); }
            else if (r >= favBase) m.load (r - favBase);
            else if (r > 0)        m.load (r - 1);
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

PolysixEditor::Section& PolysixEditor::addSection (const juce::String& title, int row)
{
    sections.push_back ({ title, row, {}, {} });
    return sections.back();
}

void PolysixEditor::add (Section& s, Kind kind, const juce::String& id, const juce::String& label)
{
    auto c = std::make_unique<Ctrl>();
    c->kind = kind; c->paramId = id; c->label = label;
    auto& st = proc.apvts;

    if (kind == Kind::Knob)
    {
        auto sl = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        sl->setPopupDisplayEnabled (true, true, this);
        sl->setRotaryParameters (juce::degreesToRadians (-135.0f), juce::degreesToRadians (135.0f), true);
        c->sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, id, *sl);
        if (auto* prm = st.getParameter (id)) sl->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        c->comp = std::move (sl);
    }
    else if (kind == Kind::Choice)
    {
        auto cb = std::make_unique<juce::ComboBox>();
        if (auto* prm = dynamic_cast<juce::AudioParameterChoice*> (st.getParameter (id)))
            cb->addItemList (prm->choices, 1);
        c->cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, id, *cb);
        c->comp = std::move (cb);
    }
    else
    {
        auto tb = std::make_unique<juce::ToggleButton>();
        c->bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, id, *tb);
        c->comp = std::move (tb);
    }
    addAndMakeVisible (*c->comp);
    s.ctrls.push_back (std::move (c));
}

void PolysixEditor::addCustom (Section& s, std::unique_ptr<juce::Component> comp, const juce::String& label, int span)
{
    auto c = std::make_unique<Ctrl>();
    c->kind = Kind::Custom; c->label = label; c->span = span;
    c->comp = std::move (comp);
    addAndMakeVisible (*c->comp);
    s.ctrls.push_back (std::move (c));
}

void PolysixEditor::resized()
{
    int y = kTopH;
    for (int row = 0; row < 3; ++row)
    {
        int x = kMargin;
        const int secH = kCellH + kTitleH + 2 * kSecPad;
        for (auto& s : sections)
        {
            if (s.row != row) continue;
            int cells = 0; for (auto& c : s.ctrls) cells += c->span;
            const int w = cells * kCellW + 2 * kSecPad;
            s.bounds = { x, y, w, secH };
            int cx = x + kSecPad;
            for (auto& c : s.ctrls)
            {
                c->cell = { cx, y + kSecPad, kCellW * c->span, kCellH };
                auto area = c->cell.withTrimmedBottom (22);
                if (c->kind == Kind::Knob)        c->comp->setBounds (area.withSizeKeepingCentre (64, 64));
                else if (c->kind == Kind::Choice) c->comp->setBounds (area.withSizeKeepingCentre (kCellW - 4, 24));
                else if (c->span > 1)             c->comp->setBounds (area.reduced (6, 6));
                else                              c->comp->setBounds (area.withSizeKeepingCentre (40, 32));
                cx += kCellW * c->span;
            }
            x += w + 10;
        }
        y += secH + kRowGap;
    }
    // preset bar, right-aligned in the header
    auto bar = juce::Rectangle<int> (getWidth() - kMargin - 520, 10, 520, 24);
    favOnlyBtn.setBounds (bar.removeFromRight (64)); bar.removeFromRight (6);
    delBtn.setBounds (bar.removeFromRight (44));     bar.removeFromRight (4);
    saveBtn.setBounds (bar.removeFromRight (52));    bar.removeFromRight (10);
    favBtn.setBounds (bar.removeFromRight (30));     bar.removeFromRight (4);
    nextBtn.setBounds (bar.removeFromRight (26));    bar.removeFromRight (2);
    prevBtn.setBounds (bar.removeFromLeft (26));     bar.removeFromLeft (2);
    nameBtn.setBounds (bar);
}

void PolysixEditor::paint (juce::Graphics& g)
{
    g.fillAll (kPanel);

    // wooden end cheeks
    juce::ColourGradient wood (kWood.brighter (0.25f), 0, 0, kWood.darker (0.4f), 0, (float) getHeight(), false);
    g.setGradientFill (wood);
    g.fillRect (0, 0, 18, getHeight());
    g.fillRect (getWidth() - 18, 0, 18, getHeight());

    // header
    g.setColour (kText);
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    g.drawText ("POLYTWELVE", kMargin, 8, 400, 28, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.setColour (kText.withAlpha (0.6f));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("PROGRAMMABLE 12 VOICE SYNTH  \xc2\xb7  TICCO ROSS 2026")), kMargin + 168, 14, 380, 20,
                juce::Justification::centredLeft);

    for (auto& s : sections)
    {
        g.setColour (kPanelDark);
        g.fillRoundedRectangle (s.bounds.toFloat(), 5.0f);
        g.setColour (kLine);
        g.drawRoundedRectangle (s.bounds.toFloat().reduced (0.5f), 5.0f, 1.0f);

        g.setColour (kText);
        g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
        g.drawText (s.title, s.bounds.withTop (s.bounds.getBottom() - kTitleH - 4), juce::Justification::centred);

        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.setColour (kText.withAlpha (0.9f));
        for (auto& c : s.ctrls)
            g.drawText (c->label, c->cell.withTop (c->cell.getBottom() - 20), juce::Justification::centred);
    }
}

// ---- chord memory UI ----------------------------------------------------------------------------
void LedButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto area = getLocalBounds().toFloat();
    auto sw = juce::Rectangle<float> (0, 0, 30.0f, 22.0f).withCentre (area.getCentre()).translated (0.0f, down ? 1.0f : 0.0f);
    g.setColour (juce::Colours::black.withAlpha (down ? 0.2f : 0.35f));
    g.fillRoundedRectangle (sw.translated (1.0f, down ? 1.0f : 2.0f), 3.0f);
    g.setColour (highlighted ? juce::Colour (0xffffffff) : juce::Colour (0xffe4e6ec));
    g.fillRoundedRectangle (sw, 3.0f);
    const auto led = juce::Rectangle<float> (0, 0, 8.0f, 8.0f).withCentre (sw.getCentre());
    g.setColour (lit ? kLed : juce::Colour (0xff5a2420));
    g.fillEllipse (led);
    if (lit) { g.setColour (kLed.withAlpha (0.35f)); g.fillEllipse (led.expanded (3.0f)); }
}

void ChordDisplay::set (const juce::String& nameText, const juce::String& notesText, bool isActive)
{
    if (nameText == name && notesText == notes && isActive == active) return;
    name = nameText; notes = notesText; active = isActive;
    repaint();
}

void ChordDisplay::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0b0d18));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (kLine);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

    const auto amber = juce::Colour (0xffffb23e).withAlpha (active ? 1.0f : 0.45f);
    auto inner = getLocalBounds().reduced (10, 6);
    g.setColour (amber);
    g.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
    g.drawText (name, inner.removeFromTop (inner.getHeight() / 2 + 4), juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawFittedText (notes, inner, juce::Justification::centredLeft, 2, 0.8f);
}

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

    for (int rootPc : pcs) // pcs relative to the bass; try each as root
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

void PolysixEditor::refreshChordDisplay()
{
    int iv[p6::kNumVoices]; int root = 60;
    const int n = proc.getChord (iv, root);
    juce::StringArray parts;
    for (int i = 0; i < n; ++i) parts.add (noteName (root + iv[i]));
    const bool chordMode = proc.apvts.getRawParameterValue ("voiceMode")->load() > 1.5f;
    if (flashTicks == 0) memoryBtn->setLit (chordMode);
    chordDisplay->set (chordName (iv, n, root), parts.joinIntoString (juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  "))), chordMode);
}

void PolysixEditor::timerCallback()
{
    const bool chordMode = proc.apvts.getRawParameterValue ("voiceMode")->load() > 1.5f;
    if (flashTicks > 0) --flashTicks;
    memoryBtn->setLit (chordMode && (flashTicks == 0 || (flashTicks % 2) == 1));

    const int v = proc.getChordVersion();
    if (v != lastChordVersion) { lastChordVersion = v; refreshChordDisplay(); }
    else refreshChordDisplay(); // cheap: only repaints when text/state changed
}
