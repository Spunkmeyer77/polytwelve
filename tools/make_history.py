#!/usr/bin/env python3
"""Builds docs/Polytwelve-Update-History.pdf. Add an entry to HISTORY at every release, with the manual update."""
import pathlib, re
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.platypus import KeepTogether, Paragraph, SimpleDocTemplate, Spacer

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "Polytwelve-Update-History.pdf"
VERSION = re.search(r"project\(Polytwelve VERSION ([0-9.]+)\)", (ROOT / "CMakeLists.txt").read_text()).group(1)

NAVY = colors.HexColor("#2f4c6e")
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
body = ParagraphStyle("body", parent=ss["Normal"], fontName="DejaVu", fontSize=10, leading=14.5, textColor=INK, spaceAfter=4)
sub = ParagraphStyle("sub", parent=body, leftIndent=34, firstLineIndent=-10, spaceAfter=2)
item = ParagraphStyle("item", parent=body, leftIndent=20, firstLineIndent=-20)
h2 = ParagraphStyle("h2", parent=body, fontName="DejaVuB", fontSize=13, leading=16, spaceBefore=12, spaceAfter=4, textColor=AMBER)
title = ParagraphStyle("title", parent=body, fontName="DejaVuB", fontSize=26, leading=30, alignment=TA_CENTER, textColor=NAVY)
subtitle = ParagraphStyle("subtitle", parent=body, fontSize=11, leading=15, alignment=TA_CENTER, textColor=GREY)

# (version heading, [(text, [sub-points])]) - numbered across the whole history. No dates.
HISTORY = [
    ("Up to 0.5.1", [
        ("Polytwelve, a 12-voice synthesizer built from the Polysix 2 Reaktor ensemble, as AU, VST3 and standalone for "
         "macOS, then VST3 for Windows and VST3/LV2 for Linux, built automatically on GitHub.", []),
        ("Preset manager with factory and user presets, favourites, previous/next and preset files.", []),
        ("Chord Memory with a chord-name display, Unison mode, glide with legato, and the arpeggiator with host sync.", []),
        ("Chorus, Phaser and Ensemble effects, pitch and mod wheels, analog drift and detune.", []),
        ("PDF user manual in every build.", []),
    ]),
    ("0.6.0: New panel", [
        ("A completely new, photo-realistic hardware panel: navy metal with section boxes, cream knobs, black rubber buttons "
         "with red LEDs, slide switches, pitch and mod wheels, an amber preset display and red LED displays.", []),
        ("Layout and controls:", [
            "Sections in signal order: VCO, VCF, VCA, EG, MG on top; WHEELS, VOICE, ARPEGGIATOR, EFFECTS, MASTER below.",
            "OCTAVE, WAVEFORM, SUB OSC, VCA MODE, MG MODE, VOICE MODE and the arpeggiator RANGE and MODE are now slide switches.",
            "EFFECTS has a lit button for each effect.",
            "SYNC RATE and BEND RANGE are LED displays, set with arrows, dragging or the mouse wheel.",
            "The stored chord is shown next to VOICE MODE.",
        ]),
        ("Preset strip with &lt; &gt; SAVE BANK ★. BANK opens the browser, which now also has Delete and "
         "Step Through Favourites Only.", []),
        ("The window can be resized and keeps its shape.", []),
        ("New About window (click the logo) with version and contact details.", []),
        ("The manual is rewritten for the new panel, and this update history ships with every build.", []),
    ]),
    ("0.7.0: Polysix model", [
        ("New <b>MODEL</b> switch in MASTER: <b>CLASSIC</b> (the engine so far, unchanged) or <b>POLYSIX</b>, a new engine "
         "built from the Korg Polysix service manual. Presets keep the current MODEL; projects remember it.", []),
        ("The POLYSIX model:", [
            "SSM2044 filter model, as on the Polysix voice board, with a clean self-oscillating sine at full resonance.",
            "Cutoff, keyboard tracking (up to 150%), MG speed and delay, and PWM speed ranges from the factory specs.",
            "Effects modelled on the bucket-brigade effect board, mono like the original; PHASE is now the original's short "
            "swept delay with feedback.",
        ]),
        ("The manual has a new chapter on the two engines.", []),
    ]),
]

def build():
    def on_page(c, doc):
        c.setFont("DejaVu", 7.5)
        c.setFillColor(GREY)
        c.drawString(18 * mm, 10 * mm, f"Polytwelve {VERSION} Update History · Ticco Ross")
        c.drawRightString(A4[0] - 18 * mm, 10 * mm, str(doc.page))

    story = [Paragraph("Polytwelve Update History", title), Spacer(1, 4),
             Paragraph(f"Ticco Ross · current version {VERSION}", subtitle), Spacer(1, 10)]
    n = 0
    for heading, items in HISTORY:
        block = [Paragraph(heading, h2)]
        for text, subs in items:
            n += 1
            block.append(Paragraph(f"<b>{n}.</b>&nbsp;&nbsp;{text}", item))
            block += [Paragraph("•&nbsp;&nbsp;" + s, sub) for s in subs]
        story.append(KeepTogether(block))
    SimpleDocTemplate(str(OUT), pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm, topMargin=18 * mm, bottomMargin=18 * mm,
                      title="Polytwelve Update History", author="Ticco Ross").build(story, onFirstPage=on_page, onLaterPages=on_page)
    print("wrote", OUT)

if __name__ == "__main__":
    build()
