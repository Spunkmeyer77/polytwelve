#!/usr/bin/env python3
"""Builds docs/Polytwelve-Manual.pdf (reportlab). Run from the repo root after changing the panel or
controls; refresh docs/panel.jpg first from the test app's editor_2x.png snapshot."""
import pathlib, re
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.graphics.shapes import Drawing, Rect, String, Line, Polygon
from reportlab.platypus import (Image, KeepTogether, PageBreak, Paragraph, SimpleDocTemplate, Spacer, Table,
                                TableStyle, ListFlowable, ListItem)

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "Polytwelve-Manual.pdf"
VERSION = re.search(r"project\(Polytwelve VERSION ([0-9.]+)\)", (ROOT / "CMakeLists.txt").read_text()).group(1)

NAVY = colors.HexColor("#2f4c6e")
DARK = colors.HexColor("#1d2a44")
AMBER = colors.HexColor("#c8741c")
INK = colors.HexColor("#1d1d1f")
GREY = colors.HexColor("#5a5650")

from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.fonts import addMapping
pdfmetrics.registerFont(TTFont("DejaVu", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("DejaVuB", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"))
pdfmetrics.registerFont(TTFont("DejaVuI", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Oblique.ttf"))
addMapping("DejaVu", 0, 0, "DejaVu"); addMapping("DejaVu", 1, 0, "DejaVuB"); addMapping("DejaVu", 0, 1, "DejaVuI"); addMapping("DejaVu", 1, 1, "DejaVuB")
ss = getSampleStyleSheet()
body = ParagraphStyle("body", parent=ss["Normal"], fontName="DejaVu", fontSize=10, leading=14.5, textColor=INK, spaceAfter=5)
small = ParagraphStyle("small", parent=body, fontSize=8.5, leading=11.5, textColor=GREY)
h1 = ParagraphStyle("h1", parent=body, fontName="DejaVuB", fontSize=18, leading=22, spaceBefore=4, spaceAfter=8, textColor=NAVY)
h2 = ParagraphStyle("h2", parent=body, fontName="DejaVuB", fontSize=12.5, leading=16, spaceBefore=10, spaceAfter=4, textColor=AMBER)
title = ParagraphStyle("title", parent=body, fontName="DejaVuB", fontSize=34, leading=40, alignment=TA_CENTER, textColor=NAVY)
subtitle = ParagraphStyle("subtitle", parent=body, fontSize=12, leading=16, alignment=TA_CENTER, textColor=GREY)
cell = ParagraphStyle("cell", parent=body, fontSize=9, leading=12, spaceAfter=0)
cellb = ParagraphStyle("cellb", parent=cell, fontName="DejaVuB")
warn = ParagraphStyle("warn", parent=body, backColor=colors.HexColor("#fdf1e2"), borderColor=AMBER, borderWidth=0.8,
                      borderPadding=7, spaceBefore=6, spaceAfter=12)

def table(rows, widths, header=True):
    data = [[Paragraph(str(c), cellb if (header and r == 0) or c_i == 0 else cell) for c_i, c in enumerate(row)]
            for r, row in enumerate(rows)]
    t = Table(data, colWidths=[w * mm for w in widths], repeatRows=1 if header else 0)
    st = [("VALIGN", (0, 0), (-1, -1), "TOP"), ("GRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#c9c4b8")),
          ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]
    if header:
        st += [("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#e9e4d6"))]
    t.setStyle(TableStyle(st))
    return t

def bullets(items):
    return ListFlowable([ListItem(Paragraph(i, body), leftIndent=12, value="circle") for i in items],
                        bulletType="bullet", start="\u2022", leftIndent=12, bulletFontSize=8, bulletFontName="DejaVu")

def steps(items):
    return ListFlowable([ListItem(Paragraph(i, body), leftIndent=14) for i in items], bulletType="1", leftIndent=14, bulletFontName="DejaVuB")

def flow_diagram():
    d = Drawing(170 * mm, 30 * mm)
    boxes = ["VCO + SUB", "VCF", "VCA", "\u00d7 12 voices", "EFFECTS", "OUT"]
    x = 0
    w, h, gap = 24 * mm, 11 * mm, 5.2 * mm
    for i, b in enumerate(boxes):
        fill = DARK if b != "\u00d7 12 voices" else colors.white
        d.add(Rect(x, 13 * mm, w, h, rx=2, ry=2, fillColor=fill, strokeColor=NAVY, strokeWidth=1))
        d.add(String(x + w / 2, 13 * mm + h / 2 - 3, b, textAnchor="middle", fontName="DejaVuB", fontSize=8.5,
                     fillColor=colors.white if fill == DARK else NAVY))
        if i < len(boxes) - 1:
            d.add(Line(x + w, 13 * mm + h / 2, x + w + gap - 1.5, 13 * mm + h / 2, strokeColor=NAVY, strokeWidth=1.2))
            ax = x + w + gap
            d.add(Polygon([ax, 13 * mm + h / 2, ax - 4, 13 * mm + h / 2 + 2.5, ax - 4, 13 * mm + h / 2 - 2.5], fillColor=NAVY, strokeColor=NAVY))
        x += w + gap
    for b, bx in (("EG", 1), ("MG", 2)):
        cx = (w + gap) * bx + w / 2 - (w + gap) / 2
        d.add(Rect(cx - 10 * mm, 0, 20 * mm, 8 * mm, rx=2, ry=2, fillColor=colors.white, strokeColor=AMBER, strokeWidth=1, strokeDashArray=[2, 2] if b == "MG" else None))
        d.add(String(cx, 2.6 * mm, b, textAnchor="middle", fontName="DejaVuB", fontSize=8.5, fillColor=AMBER))
    return d

def build():
    def on_page(c, doc):
        c.setFont("DejaVu", 7.5)
        c.setFillColor(GREY)
        c.drawString(18 * mm, 10 * mm, f"Polytwelve {VERSION} User Manual \u00b7 Transistor Labs")
        c.drawRightString(A4[0] - 18 * mm, 10 * mm, str(doc.page))

    s = []
    s += [Spacer(1, 30 * mm), Paragraph("POLYTWELVE", title), Spacer(1, 4),
          Paragraph(f"Programmable 12-voice synthesizer \u00b7 User Manual \u00b7 version {VERSION}", subtitle),
          Spacer(1, 10 * mm), Image(str(ROOT / "docs" / "panel.jpg"), width=170 * mm, height=170 * mm * 900 / 1440),
          Spacer(1, 10 * mm), Paragraph("Transistor Labs \u00b7 ticcoross@gmail.com \u00b7 ticcoross.gumroad.com", subtitle), PageBreak()]

    s += [Paragraph("1. Introduction", h1), Paragraph(
        f"Polytwelve is a programmable 12-voice analog-style synthesizer by Transistor Labs (2026), version {VERSION}. Each voice "
        "has one oscillator with a sub-oscillator, a resonant 24 dB/oct low-pass filter, an envelope, an amplifier and its own "
        "modulation generator (MG). The twelve voices are summed and pass through a stereo effects section (Chorus, Phaser or "
        "Ensemble).", body), flow_diagram(),
        Paragraph("Signal flow: one voice of twelve, then the shared effects. The EG shapes the filter and the amplifier; the "
                  "MG (dashed) reaches one destination at a time, chosen by its MODE switch.", small),
        Paragraph("It plays three ways: Poly (twelve notes at once), Unison (all twelve voices on one note) and Chord (one key "
                  "plays a stored chord). A built-in arpeggiator, a preset browser with favourites and per-voice analog drift "
                  "complete it.", body),
        Paragraph("Formats", h2),
        table([["System", "Formats", "Installer"],
               ["macOS 10.13 or later (Apple Silicon and Intel)", "Audio Unit, VST3, standalone app", f"Polytwelve-{VERSION}-macOS.pkg"],
               ["Windows 10 or later (64-bit)", "VST3, standalone app", f"Polytwelve-{VERSION}-Windows-Setup.exe"],
               ["Linux x86_64", "VST3, LV2, standalone app", f"Polytwelve-{VERSION}-Linux-x86_64.tar.gz"]], [55, 50, 65])]

    s += [Paragraph("2. Installing", h1),
          Paragraph("<b>macOS security warning.</b> The macOS installer is not notarised by Apple, so the first time you open it "
                    "macOS says it \u201ccannot be opened because Apple cannot check it for malicious software\u201d (or that it is "
                    "from an unidentified developer). This is expected. To install it: click <b>Done</b> (not Move to Trash), open "
                    "<b>System Settings \u2192 Privacy &amp; Security</b>, scroll down to the message about Polytwelve and click "
                    "<b>Open Anyway</b>, then confirm with your password. On older macOS you can instead right-click (or "
                    "Control-click) the .pkg and choose <b>Open</b>.", warn),
          bullets(["<b>macOS:</b> double-click the .pkg and follow the steps. Click Customize to skip the AU, VST3 or app.",
                   "<b>Windows:</b> run the Setup .exe. If SmartScreen appears, click <b>More info \u2192 Run anyway</b>. The VST3 "
                   "goes to <i>C:\\Program Files\\Common Files\\VST3</i>.",
                   "<b>Linux:</b> unpack the archive and run <i>./install.sh</i>. It installs for your user to <i>~/.vst3</i>, "
                   "<i>~/.lv2</i> and <i>~/.local/bin</i>, with no root needed."]),
          Paragraph("Afterwards, rescan plug-ins in your DAW. In Ableton Live on a Mac: Settings \u2192 Plug-Ins, turn on Use "
                    "Audio Units v2, hold Option and click Rescan. Polytwelve appears under Transistor Labs.", body)]

    s += [PageBreak(), Paragraph("3. The panel", h1),
          Paragraph("The panel follows the sound from left to right: the preset strip and logo along the top, the sound "
                    "sections in the middle row, and the playing sections in the bottom row. The window can be resized by "
                    "dragging its bottom-right corner; it keeps its shape.", body),
          table([["Row", "Sections, left to right"],
                 ["Top", "PRESET strip (display, &lt; &gt; SAVE BANK \u2605) \u00b7 POLYTWELVE logo (click it for About)"],
                 ["Middle", "VCO \u00b7 VCF \u00b7 VCA \u00b7 EG \u00b7 MG"],
                 ["Bottom", "WHEELS \u00b7 VOICE \u00b7 ARPEGGIATOR \u00b7 EFFECTS \u00b7 MASTER"]], [25, 145]),
          Paragraph("Working the controls", h2),
          bullets(["<b>Knobs:</b> drag up or down (or left and right). While you drag, a bubble shows the exact value (Hz, ms, "
                   "dB, semitones). Double-click returns a knob to its default.",
                   "<b>Slide switches:</b> click a position name, or click the switch and drag; the mouse wheel moves it too.",
                   "<b>Rubber buttons:</b> click to turn on or off. The red LED above shows it is on; the EFFECTS buttons also "
                   "light up orange.",
                   "<b>LED displays</b> (SYNC RATE, BEND RANGE): click the &lt; &gt; buttons beside them, drag over the display, "
                   "or scroll over it.",
                   "Every control can be automated from your DAW and is saved with your project and with presets."])]

    s += [Paragraph("4. VCO, VCF and VCA", h1), Paragraph("VCO (oscillator)", h2),
          Paragraph("Each voice has one oscillator plus a sub-oscillator. The waveforms are band-limited, so high notes stay clean.", body),
          table([["Control", "Range", "What it does"],
                 ["PW/PWM", "0\u201310", "In Pulse mode, the pulse width: 0 = square (50%), 10 = thin (95%). In PWM mode, how far the width sweeps. No effect on Saw."],
                 ["PWM SPEED", "0.1\u201312.8 Hz", "How fast the width sweeps in PWM mode."],
                 ["OCTAVE", "16' \u00b7 8' \u00b7 4'", "Shifts the oscillator down one octave, normal, or up one octave."],
                 ["WAVEFORM", "SAW \u00b7 PULSE \u00b7 PWM", "Saw: bright and full. Pulse: hollow; set its width with PW/PWM. PWM: a pulse whose width sweeps on its own, for a moving, chorused tone."],
                 ["SUB OSC", "OFF \u00b7 1 OCT \u00b7 2 OCT", "Adds a square wave one or two octaves below the main oscillator, at equal level."]], [28, 35, 107]),
          Paragraph("Tip: Pulse with the sub at 1 OCT and a low cutoff gives classic Polysix basses. PWM with Ensemble gives lush strings.", small),
          Paragraph("VCF (filter)", h2),
          Paragraph("A 4-pole (24 dB/oct) resonant low-pass filter with soft saturation in its feedback loop. Resonance stays in "
                    "tune across the range, and at high settings the filter self-oscillates into a clean sine.", body),
          table([["Control", "Range", "What it does"],
                 ["CUTOFF", "27.5 Hz \u2013 17 kHz", "The filter's brightness. Lower = darker."],
                 ["RESONANCE", "0\u201310", "Emphasis at the cutoff. Near 10 the filter sings on its own."],
                 ["EG INTENSITY", "\u22125 to +5 on the scale", "How far the envelope moves the cutoff: up to \u00b15 octaves. Negative values sweep it down instead of up."],
                 ["KBD TRACK", "0\u201310", "How much the cutoff follows the keys. At 10 it tracks the keyboard one-to-one, centred on C3."]], [28, 35, 107]),
          Paragraph("VCA (amplifier)", h2),
          table([["Control", "Range", "What it does"],
                 ["MODE", "EG \u00b7 GATE", "EG: volume follows the envelope. GATE: volume is fully on while a key is held and off when released, organ-style (the EG still drives the filter)."],
                 ["ATTENUATOR", "\u00b110 dB (\u22125 to +5 on the scale)", "Level of each voice before the effects. Lower it if high resonance or Unison gets too loud."]], [28, 35, 107])]

    s += [Paragraph("5. EG and MG", h1), Paragraph("EG (envelope)", h2),
          Paragraph("One ADSR per voice, shared by the filter (through EG INTENSITY) and the amplifier. Its curves are "
                    "exponential, like an analog envelope.", body),
          table([["Control", "Range", "What it does"],
                 ["ATTACK", "1 ms \u2013 10 s", "Time to rise to full level."],
                 ["DECAY", "1 ms \u2013 10 s", "Time to fall to the sustain level."],
                 ["SUSTAIN", "0\u201310", "Level held while the key is down."],
                 ["RELEASE", "1 ms \u2013 10 s", "Time to fade after the key is released."]], [28, 35, 107]),
          Paragraph("The time knobs are logarithmic: 2.5 \u2248 10 ms, 5 \u2248 100 ms, 7.5 \u2248 1 s, 10 = 10 s. With ANALOG turned "
                    "up, each voice's decay and release differ slightly.", small),
          Paragraph("MG (modulation generator)", h2),
          Paragraph("A triangle LFO, one per voice, that fades in after each note starts and modulates one destination. With "
                    "ANALOG turned up, each voice's MG runs at a slightly different rate.", body),
          table([["Control", "Range", "What it does"],
                 ["FREQUENCY", "0.01\u201320 Hz", "Speed."],
                 ["DELAY", "1 ms \u2013 3.2 s", "How long the modulation takes to fade in after a note starts. Restarts with every note."],
                 ["LEVEL", "0\u201310", "Depth. The curve is gentle at the bottom for fine vibrato."],
                 ["MODE", "VCO \u00b7 VCF \u00b7 VCA", "VCO: vibrato (up to \u00b112 semitones at full LEVEL). VCF: filter wobble (up to \u00b14 octaves). VCA: tremolo."]], [28, 35, 107]),
          Paragraph("The MOD wheel always adds vibrato from the same triangle, up to \u00b12 semitones, whatever the MODE, LEVEL or DELAY settings.", small)]

    s += [PageBreak(), Paragraph("6. Voice and Chord Memory", h1),
          table([["Control", "Range", "What it does"],
                 ["MODE", "POLY \u00b7 UNISON \u00b7 CHORD", "How keys are assigned to voices (below)."],
                 ["STORED CHORD", "display", "The chord in memory: its name on top (e.g. Am7, Cmaj7, C/E for an inversion) and its notes below, lowest first. Bright in Chord mode, dim otherwise."],
                 ["ANALOG", "0\u201310", "Per-voice variation: a small fixed tuning offset and slow pitch drift, plus differences in cutoff, decay, release and MG and PWM rates. 0 = perfectly stable."],
                 ["DETUNE", "0\u201310", "A fixed tuning offset per voice, up to \u00b125 cents. Most audible in Unison."],
                 ["GLIDE", "Off, 5 ms \u2013 3 s", "Portamento: each new note slides in pitch from the previous one, taking about this long. 0 = off."],
                 ["HOLD", "on / off", "Latches notes: they keep sounding after you let go. A sustain pedal does the same."],
                 ["LEGATO", "on / off", "Notes glide only if you press the new key before releasing the old one."],
                 ["MEMORY", "button", "Stores the chord you are holding and switches to Chord mode."]], [28, 35, 107]),
          Paragraph("Voice modes", h2),
          bullets(["<b>POLY:</b> up to twelve notes at once. Each new note takes the next free voice in turn, which lets release "
                   "tails ring. With all twelve busy, the oldest released voice is reused first, then the oldest held one.",
                   "<b>UNISON:</b> all twelve voices play one note, stacked with their DETUNE and ANALOG offsets, for a huge mono "
                   "sound. The last key pressed wins; releasing it returns to a key still held.",
                   "<b>CHORD:</b> every key plays the stored chord, transposed. Chords are polyphonic: when you play a new chord, "
                   "the one you let go keeps ringing through its RELEASE. While keys overlap, the chord follows the last key "
                   "(with GLIDE, if it is on), as in Unison."]),
          Paragraph("Storing a chord", h2),
          steps(["Hold the chord on your MIDI keyboard, e.g. A2, C3, E3, G3.",
                 "While holding it, press <b>MEMORY</b>. Its LED blinks.",
                 "Let go. MODE is now CHORD, the MEMORY LED stays lit and the STORED CHORD display shows Am7. Pressing D3 plays Dm7, F3 plays Fm7, and so on."]),
          bullets(["MEMORY with no keys held switches to Chord mode using the chord already stored.",
                   "Moving MODE to CHORD while holding two or more keys also stores them.",
                   "The chord is saved with presets and with your project. The default chord is a minor triad (C3 \u00b7 Eb3 \u00b7 G3).",
                   "Note names follow Ableton's convention (middle C = C3)."])]

    s += [Paragraph("7. Arpeggiator", h1),
          Paragraph("The arpeggiator plays the keys you hold one at a time, in pitch order, repeated over a range of octaves. Each "
                    "step sounds for half its length.", body),
          table([["Control", "Range", "What it does"],
                 ["ARP", "on / off", "Turns the arpeggiator on."],
                 ["LATCH", "on / off", "Keeps the pattern running after you let go. The next chord you play replaces it."],
                 ["SYNC", "on / off", "Follows your DAW's tempo at the SYNC RATE instead of the SPEED knob."],
                 ["SPEED", "0.5\u201324 steps per second", "Rate when SYNC is off."],
                 ["RANGE", "1 OCT \u00b7 2 OCT \u00b7 FULL", "How many octaves the pattern covers: one, two, or four."],
                 ["MODE", "UP \u00b7 DOWN \u00b7 UP/DN", "Direction. UP/DN doesn't repeat the top and bottom notes."],
                 ["SYNC RATE", "1/4 \u00b7 1/8 \u00b7 1/8T \u00b7 1/16 \u00b7 1/16T \u00b7 1/32", "Step length when SYNC is on. The display dims while SYNC is off."]], [28, 42, 100]),
          Paragraph("With SYNC on and the DAW playing, steps lock to the song's beat grid. With the transport stopped, it runs at "
                    "the project tempo. In Unison each step is the full twelve-voice stack, and in Chord each step plays the "
                    "stored chord.", body)]

    s += [Paragraph("8. Effects, Wheels and Master", h1), Paragraph("Effects", h2),
          Paragraph("One effect at a time processes all twelve voices together and makes the output stereo. Press OFF, CHORUS, "
                    "PHASER or ENSEMBLE; the selected button lights up. SPEED/INT sets its speed or intensity.", body),
          table([["Effect", "SPEED/INT controls", "Sound"],
                 ["OFF", "\u2014", "Dry, mono in the centre."],
                 ["CHORUS", "Speed, 0.1\u20134 Hz", "A modulated delay, opposite on left and right. Widens leads and pads."],
                 ["PHASER", "Speed, 0.05\u20136.4 Hz", "A six-stage phaser with feedback, its sweep offset between left and right."],
                 ["ENSEMBLE", "Intensity", "Three modulated delay lines with slow and fast movement, for the lush string-machine sound."]], [28, 35, 107]),
          Paragraph("Wheels", h2),
          table([["Control", "Range", "What it does"],
                 ["PITCH", "up / down", "Drag to bend all voices. Springs back to the centre when you let go."],
                 ["MOD", "0\u2013max", "Drag up to add MG vibrato, up to \u00b12 semitones. Stays where you leave it."]], [28, 35, 107]),
          Paragraph("The cream mark on each wheel shows its position. The wheels also move when your MIDI keyboard sends pitch "
                    "bend or mod wheel.", small),
          Paragraph("Master", h2),
          table([["Control", "Range", "What it does"],
                 ["VOLUME", "0\u201310 (default 7.5)", "Overall output level, after the effects."],
                 ["TUNE", "\u00b1100 cents (\u22125 to +5 on the scale)", "Fine-tunes all voices by up to one semitone either way."],
                 ["BEND RANGE", "0\u201312 semitones (default 2)", "How far the PITCH wheel and MIDI pitch bend reach, up and down."]], [28, 42, 100])]

    s += [Paragraph("Calibration (A/B)", h1),
          Paragraph("Polytwelve's engine is ported from the Polysix 2 Reaktor ensemble. Since 0.8.0 it can also be moved, one "
                    "part at a time, towards measurements of a real Korg Polysix (27 test recordings). Open the preset menu "
                    "(BANK) and choose <b>Calibration (A/B)</b>: each item switches one fix on or off, so you can compare it "
                    "and keep only the ones you like. All are off by default, which is the original sound. They belong to the "
                    "instrument, not to a sound: presets don't change them, and your project remembers them.", body),
          table([["Switch", "What it changes"],
                 ["Cutoff Curve", "The real CUTOFF knob law (open from 7 up, much darker at 5, almost shut at 4, closed at 2) "
                                  "and the real KBD TRACK pivot, low on the keyboard."],
                 ["Resonance Level", "Self-oscillation about 8 dB louder, as on the real unit."],
                 ["Pulse Width Range", "PW narrows until the pulse closes at 10; real pulse and PWM levels; PWM about 5 Hz at SPEED 5."],
                 ["Sub Level", "Sub oscillator about 3 dB quieter, as measured."],
                 ["MG Rate & Depth", "FREQUENCY about 0.6 Hz at 2.5, 3.2 Hz at 5, 16 Hz at 7.5; a third of the depth; DELAY "
                                     "reaches half depth after about 1.6 s at 5."],
                 ["Envelope Times", "Steeper time knobs: very short at the bottom, ATTACK 10 about 10 s, RELEASE 10 about 19 s."],
                 ["Effects", "Bucket-brigade effects (mono, like the original) with the measured speeds, depths and levels."],
                 ["Noise Floor", "The real unit's faint hiss and slow ripple (about \u221256 dB). \u201cAll On\u201d leaves it "
                                 "off: switch it on by itself."]], [34, 136])]

    s += [PageBreak(), Paragraph("9. Presets", h1),
          Paragraph("A preset stores every control plus the stored chord. The amber display shows the bank (FACTORY or USER), "
                    "the preset's number and its name.", body),
          table([["Control", "What it does"],
                 ["Display", "Click it to open the preset browser. Scroll or drag over it to step through presets."],
                 ["&lt; &gt;", "Previous / next preset."],
                 ["SAVE", "Saves the current sound as a user preset. It asks for a name, and asks before replacing an existing one."],
                 ["BANK", "Opens the preset browser: Favourites, Factory and User lists, plus Save As\u2026, Delete, "
                          "Step Through Favourites Only, Open Preset File\u2026 and Show Preset Folder."],
                 ["\u2605", "Adds or removes the current preset from favourites. It glows when the preset is a favourite."]], [28, 142]),
          Paragraph("With <b>Step Through Favourites Only</b> ticked, &lt; and &gt; step through favourites only, and the display shows "
                    "\u2605 ONLY. Your project remembers which preset was loaded. Factory presets also appear as programs in "
                    "hosts that show a program list.", body),
          Paragraph("Factory presets", h2),
          table([["Preset", "Character"],
                 ["Init", "Plain saw, the starting point"], ["Strings", "Slow saw pad through the Ensemble"],
                 ["Brass", "Swelling filter envelope"], ["Bass", "16' pulse with sub, resonant pluck"],
                 ["Square Lead", "Unison square with legato glide, delayed vibrato and chorus"],
                 ["Rezzy Arp", "Resonant saw on a synced 1/16 arpeggio, through the phaser"],
                 ["Filter Sweep", "PWM with the MG slowly sweeping the filter"],
                 ["Minor Chord", "Chord mode with a minor triad, plucky, with chorus"]], [35, 135]),
          Paragraph("Preset files", h2),
          Paragraph("User presets are .p6preset files, one per sound, which you can copy, back up or share. Favourites are listed "
                    "in favourites.txt in the same folder.", body),
          table([["System", "Preset folder"],
                 ["macOS", "~/Library/Audio/Presets/Transistor Labs/Polytwelve"],
                 ["Windows", "Documents\\Transistor Labs\\Polytwelve\\Presets"],
                 ["Linux", "~/.local/share/Transistor Labs/Polytwelve/Presets"]], [35, 135])]

    s += [Paragraph("10. MIDI implementation", h1),
          Paragraph("Polytwelve responds on all MIDI channels.", body),
          table([["Message", "Effect"],
                 ["Note on / off", "Plays notes. Velocity is ignored, as on the original Polysix."],
                 ["Pitch bend", "Bends all voices by up to the BEND RANGE; the on-screen PITCH wheel follows it."],
                 ["CC 1 (mod wheel)", "Adds MG vibrato, up to \u00b12 semitones; the on-screen MOD wheel follows it."],
                 ["CC 64 (sustain pedal)", "Works like HOLD."],
                 ["CC 120 / CC 123", "All sound off / all notes off."]], [40, 130]),
          Paragraph("All 37 controls are exposed to the host for automation and MIDI mapping. In Ableton Live, use the plug-in's "
                    "Configure button to add them to the device, then map them like any Live control.", body)]

    s += [Paragraph("11. Specifications, uninstalling and contact", h1),
          table([["Item", "Value"],
                 ["Version", f"{VERSION} (shown under the PROGRAMMABLE 12 VOICE SYNTHESIZER line next to the logo, and in About)"],
                 ["Polyphony", "12 voices"],
                 ["Per voice", "1 oscillator (saw, pulse, PWM) + sub-oscillator, 24 dB/oct resonant low-pass filter, ADSR, VCA, triangle MG, glide"],
                 ["Effects", "Chorus, Phaser, Ensemble (stereo)"],
                 ["Formats", "AU (macOS), VST3, LV2 (Linux), standalone app"],
                 ["Parameters", "37, all automatable"]], [35, 135]),
          Paragraph("Uninstalling", h2),
          bullets(["<b>macOS:</b> delete Polytwelve.component from /Library/Audio/Plug-Ins/Components, Polytwelve.vst3 from "
                   "/Library/Audio/Plug-Ins/VST3 and Polytwelve.app from Applications.",
                   "<b>Windows:</b> Settings \u2192 Apps \u2192 Polytwelve \u2192 Uninstall.",
                   "<b>Linux:</b> run ./uninstall.sh from the downloaded folder."]),
          Paragraph("Uninstalling leaves your presets in place.", small),
          Paragraph("Contact", h2),
          Paragraph("Polytwelve is designed by Transistor Labs, 2026. Questions, bug reports and suggestions: <b>ticcoross@gmail.com</b>. "
                    "More instruments: <b>ticcoross.gumroad.com</b>. Click the POLYTWELVE logo on the panel for the About window.", body)]

    SimpleDocTemplate(str(OUT), pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm, topMargin=16 * mm, bottomMargin=18 * mm,
                      title="Polytwelve User Manual", author="Transistor Labs").build(s, onFirstPage=on_page, onLaterPages=on_page)
    print("wrote", OUT)

if __name__ == "__main__":
    build()
