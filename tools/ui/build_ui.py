#!/usr/bin/env python3
"""Builds the Polytwelve panel graphics and Source/PanelLayout.h.

The photographic parts (knob, rubber button, LED, slide switch, wheel, screw) are cut from
the reference images in tools/ui/src; the panel, lettering, scales and layout are drawn here.
Output (design size 1440 x 900, rendered at 2x):
  Resources/ui/panel.jpg        static background: panel, sections, lettering, scales,
                                knob skirts, wheels, empty display windows
  Resources/ui/knob_cap.png     cream cap, pointer up (rotated by the editor)
  Resources/ui/btn_<id>_0/1.png each button off / on (text baked in)
  Resources/ui/sw3_<n>.png      3-position slide switch, block at position n (0 = top)
  Resources/ui/sw2_<n>.png      2-position slide switch
  Source/PanelLayout.h          where every control goes
Run from the repo root:  python3 tools/ui/build_ui.py   (needs Pillow, numpy; fonts Inter + DejaVu)
With --preview out.png it also renders a full mockup with every control drawn.
"""
import math, os, subprocess, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "tools", "ui", "src")
RES = os.path.join(ROOT, "Resources", "ui")
os.makedirs(RES, exist_ok=True)
PREVIEW = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None

PANEL_IMG = Image.open(os.path.join(SRC, "panel_ref.webp")).convert("RGB")
SHEET = Image.open(os.path.join(SRC, "parts.webp")).convert("RGB")

S = 2
W, H = 1440, 900

def font_path(name):
    return subprocess.check_output(["fc-match", "-f", "%{file}", name]).decode()
F_BOLD = font_path("Inter:style=Bold")
F_XB = font_path("Inter:style=ExtraBold")
F_SEMI = font_path("Inter:style=SemiBold")
F_LOGO = font_path("Inter Display:style=Black")
F_MONO = font_path("DejaVu Sans Mono:style=Bold")

CREAM = (238, 230, 200)
CREAM_DIM = (200, 196, 176)
BASE = (40, 55, 86)
SECTION = (47, 76, 110)

def f(path, size): return ImageFont.truetype(path, int(size * S))

# ------------------------------------------------------------------ parts
def circle_mask(size, cx, cy, r, feather=1.2):
    yy, xx = np.mgrid[0:size[1], 0:size[0]]
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    return np.clip((r - d) / feather + 0.5, 0, 1)

def cut_dark_shape(img, box, radius_frac=0.5, dark=95, pad=0):
    a = np.asarray(img.crop(box)).astype(float)
    ys, xs = np.nonzero(a.mean(2) < dark)
    x0, x1, y0, y1 = xs.min() - pad, xs.max() + pad, ys.min() - pad, ys.max() + pad
    part = img.crop((box[0] + x0, box[1] + y0, box[0] + x1 + 1, box[1] + y1 + 1))
    w, h = part.size
    m = Image.new("L", (w * 4, h * 4), 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w * 4 - 1, h * 4 - 1), min(w, h) * radius_frac * 4, fill=255)
    out = part.convert("RGBA"); out.putalpha(m.resize((w, h), Image.LANCZOS))
    return out

def cut(img, box, thresh=8, soft=30):
    a = np.asarray(img.crop(box)).astype(float)
    bg = np.median(np.concatenate([a[0], a[-1], a[:, 0], a[:, -1]]), axis=0)
    d = np.sqrt(((a - bg) ** 2).sum(2))
    al = np.clip((d - thresh) / soft, 0, 1)
    rgb = np.clip((a - bg * (1 - al[..., None])) / np.maximum(al, 1e-3)[..., None], 0, 255)
    return Image.fromarray(np.dstack([rgb, al * 255]).astype(np.uint8), "RGBA")

# knob: static skirt (rod painted out) + rotating cap
KB = (262, 210, 462, 412)
knob_raw = SHEET.crop(KB)
kcx, kcy, kr = 362 - KB[0], 311 - KB[1], 89
ccx, ccy, cr = 346.5 - KB[0], 301.5 - KB[1], 57.5
arr = np.asarray(knob_raw).astype(float)
rot = np.asarray(knob_raw.rotate(28, center=(kcx, kcy), resample=Image.BICUBIC)).astype(float)
yy, xx = np.mgrid[0:arr.shape[0], 0:arr.shape[1]]
rod = (np.abs(xx - (ccx + 2)) < 11) & (yy < ccy - cr + 6)
rodm = np.asarray(Image.fromarray((rod * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(2))).astype(float)[..., None] / 255
arr = arr * (1 - rodm) + rot * rodm
SKIRT = Image.fromarray(np.dstack([arr, circle_mask((arr.shape[1], arr.shape[0]), kcx, kcy, kr) * 255]).astype(np.uint8), "RGBA")
cap_full = Image.fromarray(np.dstack([np.asarray(knob_raw), circle_mask((arr.shape[1], arr.shape[0]), ccx, ccy, cr) * 255]).astype(np.uint8), "RGBA")
CAP = cap_full.crop((int(ccx - cr - 2), int(ccy - cr - 2), int(ccx + cr + 3), int(ccy + cr + 3)))
CAP_OFF = (ccx - kcx, ccy - kcy)   # cap centre relative to skirt centre (sheet px)
CAP_RATIO = CAP.width / (2 * kr)    # cap image width per skirt diameter

BTN = cut_dark_shape(SHEET, (728, 616, 880, 694), radius_frac=0.5, pad=-3)
def make_lit(btn):
    w, h = btn.size
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.sqrt(((xx - w / 2) / (w * 0.36)) ** 2 + ((yy - h / 2) / (h * 0.34)) ** 2)
    g = np.clip(1 - d, 0, 1) ** 1.3
    a = np.asarray(btn).astype(float)
    rgb = a[..., :3] + (np.array([255, 140, 60], float) - a[..., :3]) * (g[..., None] * 0.85)
    return Image.fromarray(np.dstack([rgb.clip(0, 255), a[..., 3]]).astype(np.uint8), "RGBA")
LIT = make_lit(BTN)
LED_ON = cut(SHEET, (778, 552, 832, 606), thresh=8, soft=30)
def dim_led(im):
    a = np.asarray(im).astype(float)
    a[..., :3] = a[..., :3] * np.array([0.38, 0.12, 0.12])
    a[..., 3] = a[..., 3] * (np.asarray(im.split()[3]) > 120)
    return Image.fromarray(a.astype(np.uint8), "RGBA").filter(ImageFilter.GaussianBlur(0.3))
LED_OFF = dim_led(LED_ON)
WHEEL = cut_dark_shape(SHEET, (330, 795, 445, 1130), radius_frac=0.14, dark=70)
sc = PANEL_IMG.crop((150, 202, 182, 234))
SCREW = Image.fromarray(np.dstack([np.asarray(sc), circle_mask(sc.size, 16, 16, 14.5) * 255]).astype(np.uint8), "RGBA")

# slide switch: separate the sliding block from the slot so it can be placed at any position
SW = cut(SHEET, (764, 248, 850, 394))
swa = np.asarray(SW).astype(float)
_raw = np.asarray(SHEET.crop((764, 248, 850, 394))).astype(float)
_ys, _xs = np.nonzero(_raw.mean(2) < 80)
_m = Image.new("L", (SW.width * 4, SW.height * 4), 0)
ImageDraw.Draw(_m).rounded_rectangle((_xs.min() * 4, _ys.min() * 4, _xs.max() * 4 + 3, _ys.max() * 4 + 3), 12, fill=255)
swa[..., :3] = _raw
swa[..., 3] = np.asarray(_m.resize(SW.size, Image.LANCZOS)).astype(float)
SLOT_Y0, SLOT_Y1, BLOCK_Y0, BLOCK_Y1, IN_X0, IN_X1 = 16, 136, 34, 68, 10, 68
slot = swa.copy()
fillrows = swa[104:136]
for y in range(BLOCK_Y0 - 2, BLOCK_Y1 + 2):
    slot[y, IN_X0:IN_X1] = fillrows[(y - BLOCK_Y0) % len(fillrows), IN_X0:IN_X1]
BLOCK = swa[BLOCK_Y0:BLOCK_Y1, IN_X0:IN_X1].copy()
def switch_image(npos, pos):
    out = slot.copy()
    travel = (SLOT_Y1 - 4) - (SLOT_Y0 + 4) - (BLOCK_Y1 - BLOCK_Y0)
    y = int(round(SLOT_Y0 + 4 + travel * (pos / (npos - 1))))
    # soft shadow under the block, then the block
    out[y + 2:y + 2 + BLOCK.shape[0], IN_X0:IN_X1, :3] *= 0.75
    out[y:y + BLOCK.shape[0], IN_X0:IN_X1] = BLOCK
    return Image.fromarray(out.clip(0, 255).astype(np.uint8), "RGBA")

# ------------------------------------------------------------------ canvas
img = Image.new("RGB", (W * S, H * S), (214, 214, 212))
draw = ImageDraw.Draw(img)
layout = []   # (kind, id, x, y, w, h, extra)

def texture(size, seed):
    rng = np.random.default_rng(seed)
    n = rng.normal(0, 1, (size[1] // 3 + 1, size[0] // 3 + 1))
    n = np.asarray(Image.fromarray(((n * 20) + 128).clip(0, 255).astype(np.uint8)).resize(size, Image.BICUBIC)).astype(float) - 128
    fine = rng.normal(0, 4, (size[1], size[0]))
    blot = np.asarray(Image.fromarray((rng.normal(128, 40, (size[1] // 60 + 2, size[0] // 60 + 2))).clip(0, 255).astype(np.uint8)).resize(size, Image.BICUBIC)).astype(float) - 128
    return n * 0.12 + fine * 0.6 + blot * 0.025

def fill_textured(box, colour, seed, radius=0):
    x0, y0, x1, y1 = [int(v * S) for v in box]
    w, h = x1 - x0, y1 - y0
    base = np.ones((h, w, 3)) * np.array(colour, float) + texture((w, h), seed)[..., None] * np.array([0.8, 0.9, 1.0])
    g = np.linspace(1.04, 0.95, h)[:, None] * np.linspace(1.02, 0.98, w)[None, :]
    tile = Image.fromarray((base * g[..., None]).clip(0, 255).astype(np.uint8))
    m = Image.new("L", (w, h), 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w - 1, h - 1), radius * S, fill=255)
    img.paste(tile, (x0, y0), m)

def text(xy, s, size, path=F_BOLD, colour=CREAM, anchor="mm", target=None):
    d = target or draw
    fnt = f(path, size)
    d.text((xy[0] * S + 1, xy[1] * S + 1.5), s, font=fnt, fill=(20, 28, 45), anchor=anchor)
    d.text((xy[0] * S, xy[1] * S), s, font=fnt, fill=colour, anchor=anchor)

def paste(part, cx, cy, width, shadow=True, target=None):
    t = target or img
    if t.mode == "RGBA":   # layer images: composite properly so glows and shadows add up
        w = int(width * S)
        h = int(part.height * w / part.width)
        p = part.resize((w, h), Image.LANCZOS)
        x, y = int(cx * S - w / 2), int(cy * S - h / 2)
        if shadow:
            m = Image.new("L", t.size, 0)
            m.paste(p.split()[3], (x + 2 * S, y + 4 * S))
            m = m.filter(ImageFilter.GaussianBlur(4 * S)).point(lambda v: v * 0.55)
            sl = Image.new("RGBA", t.size, (10, 16, 30, 0)); sl.putalpha(m)
            t.alpha_composite(sl)
        layer = Image.new("RGBA", t.size, (0, 0, 0, 0)); layer.paste(p, (x, y))
        t.alpha_composite(layer)
        return w / S, h / S
    w = int(width * S)
    h = int(part.height * w / part.width)
    p = part.resize((w, h), Image.LANCZOS)
    x, y = int(cx * S - w / 2), int(cy * S - h / 2)
    if shadow:
        a = p.split()[3].filter(ImageFilter.GaussianBlur(4 * S))
        t.paste((10, 16, 30), (x + 2 * S, y + 4 * S), a.point(lambda v: v * 0.55))
    t.paste(p, (x, y), p)
    return w / S, h / S

sh = Image.new("L", (W * S, H * S), 0)
ImageDraw.Draw(sh).rounded_rectangle((22 * S, 28 * S, 1418 * S, 884 * S), 12 * S, fill=150)
img.paste((60, 60, 70), (0, 0), sh.filter(ImageFilter.GaussianBlur(14 * S)))
fill_textured((18, 18, 1422, 878), BASE, 1, radius=8)
draw.rounded_rectangle((18 * S, 18 * S, 1422 * S, 878 * S), 8 * S, outline=(26, 36, 58), width=2 * S)

def section(x, y, w, h, title, seed):
    draw.rectangle(((x - 1) * S, (y - 1) * S, (x + w + 1) * S, (y + h + 1) * S), fill=(30, 42, 66))
    fill_textured((x, y, x + w, y + h), SECTION, seed)
    if title:
        text((x + w / 2, y + 22), title, 15, F_XB, CREAM)

# ------------------------------------------------------------------ controls
KNOB_D = 62
previews = []   # deferred drawing of moving parts for --preview

def knob(pid, cx, cy, label, value=0.5, bipolar=False, size=KNOB_D, numbers=True):
    r = size / 2
    ring = r + 9
    for i in range(11):
        a = math.radians(-135 + 27 * i)
        px, py = cx + math.sin(a) * ring, cy - math.cos(a) * ring
        draw.ellipse(((px - 1.7) * S, (py - 1.7) * S, (px + 1.7) * S, (py + 1.7) * S), fill=CREAM)
        if numbers and ((i in (0, 2, 5, 8, 10)) if bipolar else (i % 2 == 0)):
            lab = str(i - 5) if bipolar else str(i)
            if bipolar and i - 5 > 0: lab = "+" + lab
            text((cx + math.sin(a) * (ring + 10), cy - math.cos(a) * (ring + 10)), lab, 8.5, F_SEMI, CREAM_DIM)
    scl = size / (2 * kr)
    paste(SKIRT, cx, cy, SKIRT.width * scl)
    if label:
        text((cx, cy + r + 24), label, 11.5, F_XB)
    layout.append(("Knob", pid, cx - r, cy - r, size, size, 0))
    previews.append(("knob", cx, cy, size, value))

def switch(pid, cx, cy, label, positions, pos=0, h=52):
    n = len(positions)
    w = h * SW.width / SW.height
    if label:
        text((cx, cy - h / 2 - 14), label, 11, F_XB)
    widest = 0
    for i, s in enumerate(positions):
        y = cy - h / 2 + 8 + i * (h - 16) / max(1, n - 1)
        text((cx + w / 2 + 6, y), s, 9.5, F_BOLD, CREAM, anchor="lm")
        widest = max(widest, draw.textlength(s, font=f(F_BOLD, 9.5)) / S)
    # shadow of the switch body is static
    p = switch_image(n, 0).resize((int(w * S), int(h * S)), Image.LANCZOS)
    a = p.split()[3].filter(ImageFilter.GaussianBlur(4 * S))
    img.paste((10, 16, 30), (int((cx - w / 2) * S) + 2 * S, int((cy - h / 2) * S) + 4 * S), a.point(lambda v: v * 0.55))
    layout.append(("Switch", pid, cx - w / 2, cy - h / 2, w, h, n, w + 6 + widest))
    previews.append(("switch", cx, cy, w, h, n, pos))

BUTTONS = {}
def button(pid, cx, cy, label, width=62, led=True, style="led"):
    """style: 'led' (on = LED lit), 'lit' (on = pill glows + LED), 'star' (on = pill glows, no LED)."""
    bh = width * BTN.height / BTN.width
    top = cy - 32 if led else cy - bh / 2 - 6
    pad = 18 if style in ("lit", "star") else 8
    box = (cx - width / 2 - pad, top - (pad - 8), cx + width / 2 + pad, cy + bh / 2 + pad)
    BUTTONS[pid] = (box, cx, cy, label, width, led, style)
    layout.append(("Button", pid, box[0], box[1], box[2] - box[0], box[3] - box[1], 0))
    previews.append(("button", pid))

def button_image(pid, on):
    box, cx, cy, label, width, led, style = BUTTONS[pid]
    bw, bh = int((box[2] - box[0]) * S), int((box[3] - box[1]) * S)
    t = Image.new("RGBA", (bw, bh), (0, 0, 0, 0))
    td = ImageDraw.Draw(t)
    lx, ly = cx - box[0], cy - box[1]
    lit = on and style in ("lit", "star")
    if lit:
        gl = Image.new("L", (bw, bh), 0)
        ImageDraw.Draw(gl).ellipse((bw * 0.15, (ly - 14) * S, bw * 0.85, (ly + 14) * S), fill=140)
        gla = Image.new("RGBA", (bw, bh), (255, 120, 40, 0)); gla.putalpha(gl.filter(ImageFilter.GaussianBlur(7 * S)))
        t.alpha_composite(gla)
    paste(LIT if lit else BTN, lx, ly, width, target=t)
    fs = 10 if len(label) < 7 else 8.5
    if label == "★": fs = 13
    text((lx, ly), label, fs, F_XB if label != "★" else F_MONO, (250, 240, 215) if lit else CREAM, target=td)
    if led:
        paste(LED_ON if on else LED_OFF, lx, ly - 25, 11, shadow=False, target=t)
    return t

def display(pid, box, kind):
    x0, y0, x1, y1 = box
    draw.rounded_rectangle((x0 * S, y0 * S, x1 * S, y1 * S), 5 * S, fill=(14, 12, 12), outline=(5, 5, 8), width=2 * S)
    draw.rounded_rectangle(((x0 + 3) * S, (y0 + 3) * S, (x1 - 3) * S, (y0 + 8) * S), 3 * S, fill=(30, 28, 28))
    layout.append(("Display", pid, x0, y0, x1 - x0, y1 - y0, kind))

def wheel(pid, cx, cy, label, h=200):
    w, hh = paste(WHEEL, cx, cy, h * WHEEL.width / WHEEL.height)
    text((cx, cy + h / 2 + 18), label, 11.5, F_XB)
    layout.append(("Wheel", pid, cx - w / 2, cy - hh / 2, w, hh, 0))

def arrows(prefix, box):
    x0, y0, x1, y1 = box
    cy = (y0 + y1) / 2
    button(prefix + "Down", x0 - 17, cy, "<", width=30, led=False)
    button(prefix + "Up", x1 + 17, cy, ">", width=30, led=False)

# ------------------------------------------------------------------ layout
for (x, y) in [(36, 36), (720, 36), (1404, 36), (36, 470), (1404, 470), (36, 862), (720, 862), (1404, 862)]:
    paste(SCREW, x, y, 17, shadow=False)

draw.rounded_rectangle((60 * S, 30 * S, 820 * S, 84 * S), 6 * S, fill=(33, 46, 72), outline=(24, 33, 52), width=2 * S)
text((96, 57), "PRESET", 12.5, F_XB)
display("preset", (132, 38, 520, 76), 0)
for pid, lab, w, xs in [("presetPrev", "<", 38, 560), ("presetNext", ">", 38, 604), ("presetSave", "SAVE", 58, 660),
                        ("presetBank", "BANK", 58, 726), ("presetFav", "★", 38, 786)]:
    button(pid, xs, 57, lab, width=w, led=False, style="star" if pid == "presetFav" else "led")
logo = Image.new("L", (900 * S, 90 * S), 0)
ImageDraw.Draw(logo).text((4 * S, 45 * S), "POLYTWELVE", font=f(F_LOGO, 62), fill=255, anchor="lm",
                          stroke_width=int(1.2 * S), stroke_fill=255)
logo = logo.crop(logo.getbbox())
logo = logo.resize((int(logo.width * 0.74), int(logo.height * 1.06)), Image.LANCZOS)
lx, ly = int(1392 * S - logo.width), int(57 * S - logo.height / 2)
img.paste((15, 20, 35), (lx + 2, ly + 3), logo.filter(ImageFilter.GaussianBlur(1.5)))
img.paste(CREAM, (lx, ly), logo)
layout.append(("Logo", "logo", lx / S, ly / S, logo.width / S, logo.height / S, 0))
# version / credit line between the preset strip and the logo (drawn by the editor)
layout.append(("Info", "info", 836, 40, lx / S - 846, 34, 0))

Y1, H1 = 96, 380
x = 34
def nx(w):
    global x
    v = x; x += w + 6; return v

sx = nx(332); section(sx, Y1, 332, H1, "VCO", 11)
knob("pwPwm", sx + 90, Y1 + 100, "PW/PWM", 0.45)
knob("pwmSpeed", sx + 240, Y1 + 100, "PWM SPEED", 0.3)
for i, (pid, lab, pos) in enumerate([("octave", "OCTAVE", ["16'", "8'", "4'"]), ("waveform", "WAVEFORM", ["SAW", "PULSE", "PWM"]),
                                     ("subOsc", "SUB OSC", ["OFF", "1 OCT", "2 OCT"])]):
    switch(pid, sx + 46 + i * 104, Y1 + 285, lab, pos, 1 if i == 0 else 0)

sx = nx(300); section(sx, Y1, 300, H1, "VCF", 12)
knob("cutoff", sx + 80, Y1 + 100, "CUTOFF", 0.62)
knob("resonance", sx + 220, Y1 + 100, "RESONANCE", 0.35)
knob("egInt", sx + 80, Y1 + 268, "EG INTENSITY", 0.7, bipolar=True)
knob("kbdTrack", sx + 220, Y1 + 268, "KBD TRACK", 0.5)

sx = nx(150); section(sx, Y1, 150, H1, "VCA", 13)
switch("vcaMode", sx + 62, Y1 + 110, "MODE", ["EG", "GATE"], 0, h=46)
knob("atten", sx + 75, Y1 + 268, "ATTENUATOR", 0.5, bipolar=True)

sx = nx(300); section(sx, Y1, 300, H1, "EG", 14)
knob("attack", sx + 80, Y1 + 100, "ATTACK", 0.1)
knob("decay", sx + 220, Y1 + 100, "DECAY", 0.45)
knob("sustain", sx + 80, Y1 + 268, "SUSTAIN", 0.7)
knob("release", sx + 220, Y1 + 268, "RELEASE", 0.4)

sx = nx(1406 - x); section(sx, Y1, 1406 - sx, H1, "MG", 15)
mw = 1406 - sx
for i, (pid, lab, v) in enumerate([("mgFreq", "FREQUENCY", 0.55), ("mgDelay", "DELAY", 0.2), ("mgLevel", "LEVEL", 0.3)]):
    knob(pid, sx + mw * (i + 0.5) / 3, Y1 + 100, lab, v, size=52, numbers=False)
switch("mgMode", sx + mw / 2 - 18, Y1 + 280, "MODE", ["VCO", "VCF", "VCA"], 0)

Y2, H2 = 488, 378
x = 34
sx = nx(140); section(sx, Y2, 140, H2, "WHEELS", 21)
wheel("pitchWheel", sx + 40, Y2 + 180, "PITCH")
wheel("modWheel", sx + 100, Y2 + 180, "MOD")

sx = nx(300); section(sx, Y2, 300, H2, "VOICE", 22)
switch("voiceMode", sx + 52, Y2 + 100, "MODE", ["POLY", "UNISON", "CHORD"], 0)
display("chord", (sx + 150, Y2 + 64, sx + 284, Y2 + 124), 2)
text((sx + 217, Y2 + 140), "STORED CHORD", 10, F_XB)
for i, (pid, lab, v) in enumerate([("analog", "ANALOG", 0.3), ("detune", "DETUNE", 0.15), ("glide", "GLIDE", 0.0)]):
    knob(pid, sx + 58 + i * 92, Y2 + 215, lab, v, size=56, numbers=False)
for i, (pid, lab) in enumerate([("hold", "HOLD"), ("glideLegato", "LEGATO"), ("memory", "MEMORY")]):
    button(pid, sx + 58 + i * 92, Y2 + 338, lab)

sx = nx(384); section(sx, Y2, 384, H2, "ARPEGGIATOR", 23)
for i, (pid, lab) in enumerate([("arpOn", "ARP"), ("arpLatch", "LATCH"), ("arpSync", "SYNC")]):
    button(pid, sx + 48 + i * 72, Y2 + 92, lab, width=58)
knob("arpSpeed", sx + 130, Y2 + 225, "SPEED", 0.55)
switch("arpRange", sx + 258, Y2 + 92, "RANGE", ["1 OCT", "2 OCT", "FULL"], 0)
switch("arpMode", sx + 330, Y2 + 92, "MODE", ["UP", "DOWN", "UP/DN"], 0)
display("arpDiv", (sx + 252, Y2 + 212, sx + 336, Y2 + 246), 1)
arrows("arpDiv", (sx + 252, Y2 + 212, sx + 336, Y2 + 246))
text((sx + 294, Y2 + 268), "SYNC RATE", 10.5, F_XB)

sx = nx(270); section(sx, Y2, 270, H2, "EFFECTS", 24)
for i, lab in enumerate(["OFF", "CHORUS", "PHASER", "ENSEMBLE"]):
    button("fx" + str(i), sx + 75 + (i % 2) * 120, Y2 + 92 + (i // 2) * 66, lab, width=72, style="lit")
knob("fxSpeed", sx + 135, Y2 + 278, "SPEED/INT", 0.4)

sx = nx(1406 - x); section(sx, Y2, 1406 - sx, H2, "MASTER", 25)
mw = 1406 - sx
knob("volume", sx + mw * 0.28, Y2 + 98, "VOLUME", 0.75)
knob("tune", sx + mw * 0.72, Y2 + 98, "TUNE", 0.5, bipolar=True)
display("bendRange", (sx + mw / 2 - 52, Y2 + 196, sx + mw / 2 + 52, Y2 + 230), 1)
arrows("bendRange", (sx + mw / 2 - 52, Y2 + 196, sx + mw / 2 + 52, Y2 + 230))
text((sx + mw / 2, Y2 + 250), "BEND RANGE", 10.5, F_XB)

# ------------------------------------------------------------------ outputs
img.save(os.path.join(RES, "panel.jpg"), quality=90, optimize=True, subsampling=0)
cap = CAP.resize((int(CAP.width * 0.9), int(CAP.height * 0.9)), Image.LANCZOS)
cap.save(os.path.join(RES, "knob_cap.png"), optimize=True)
for pid in BUTTONS:
    for on in (0, 1):
        button_image(pid, on).save(os.path.join(RES, f"btn_{pid}_{on}.png"), optimize=True)
for n in (2, 3):
    for p in range(n):
        im = switch_image(n, p)
        im.save(os.path.join(RES, f"sw{n}_{p}.png"), optimize=True)

with open(os.path.join(ROOT, "Source", "PanelLayout.h"), "w") as out:
    out.write("// Generated by tools/ui/build_ui.py - do not edit by hand.\n")
    out.write("// Design coordinates (the panel is 1440 x 900; the images are 2x).\n#pragma once\n\nnamespace ui\n{\n")
    out.write(f"constexpr int designW = {W}, designH = {H};\n")
    out.write(f"// knob cap: image width per knob diameter, and its centre offset from the knob centre per diameter\n")
    out.write(f"constexpr float capScale = {CAP_RATIO:.5f}f, capOffX = {CAP_OFF[0] / (2 * kr):.5f}f, capOffY = {CAP_OFF[1] / (2 * kr):.5f}f;\n")
    out.write("enum class Kind { Knob, Switch, Button, Display, Wheel, Logo, Info };\n")
    out.write("struct Item { Kind kind; const char* id; float x, y, w, h; int extra; float hitW; };\n")
    out.write("inline constexpr Item items[] = {\n")
    for it in layout:
        kind, pid, x0, y0, w, h, extra = it[:7]
        hitw = it[7] if len(it) > 7 else w
        out.write(f'    {{ Kind::{kind}, "{pid}", {x0:.1f}f, {y0:.1f}f, {w:.1f}f, {h:.1f}f, {extra}, {hitw:.1f}f }},\n')
    out.write("};\n} // namespace ui\n")

if PREVIEW:
    pv = img.copy()
    for p in previews:
        if p[0] == "knob":
            _, cx, cy, size, value = p
            scl = size / (2 * kr)
            c = CAP.rotate(-(-135 + 270 * value), resample=Image.BICUBIC)
            paste(c, cx + CAP_OFF[0] * scl, cy + CAP_OFF[1] * scl, CAP.width * scl, shadow=False, target=pv)
        elif p[0] == "switch":
            _, cx, cy, w, h, n, pos = p
            paste(switch_image(n, pos), cx, cy, w, shadow=False, target=pv)
        elif p[0] == "button":
            pid = p[1]
            box = BUTTONS[pid][0]
            on = pid in ("fx1", "arpOn", "memory")
            b = button_image(pid, on)
            pv.paste(b, (int(box[0] * S), int(box[1] * S)), b)
    pv.save(PREVIEW)
print("wrote", RES, "and Source/PanelLayout.h")
