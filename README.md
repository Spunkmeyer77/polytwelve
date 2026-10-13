# Polytwelve

Programmable 12-voice synth (AU / VST3 / Standalone) by Ticco Ross, 2026.

Its voice structure is based on the "Polysix" Reaktor ensemble by Sandy Small (2008). That ensemble
credits Mona/Polly for design ideas, and Broken Chords and AuReality for its arpeggiator.

## Build on your Mac (macOS 12+)

1. **Compiler:** run `xcode-select --install` in Terminal. Command Line Tools are enough,
   but Xcode 13/14 works too.
2. **CMake 3.22 or newer:** download the macOS .dmg from https://cmake.org/download/,
   drag it to Applications, then run:
   `sudo "/Applications/CMake.app/Contents/bin/cmake-gui" --install`
3. **Build.** Run these in Terminal from inside this folder:

   ```sh
   cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
   cmake --build build --config Release -j8
   ```

   The first configure downloads JUCE 8.0.9 from GitHub, which takes a minute.
   After the build finishes, the plugin is copied automatically to:
   - `~/Library/Audio/Plug-Ins/Components/Polytwelve.component` (AU)
   - `~/Library/Audio/Plug-Ins/VST3/Polytwelve.vst3`
4. **Validate the AU (optional):** `auval -v aumu Ptwv Tcro`
5. **Ableton Live:** go to Settings → Plug-Ins, turn on "Use Audio Units v2", and rescan.
   The synth appears under Plug-Ins → Audio Units → Ticco Ross → Polytwelve.

The standalone app is in `build/Polytwelve_artefacts/Release/Standalone/`.

## Presets

The bar at the top right of the plugin window:

- **< / >** step through presets. Turn on **★ ONLY** to step through favourites only.
- **Click the preset name** to open the browser: Favourites, Factory, User, plus
  *Save As…*, *Open Preset File…* and *Show Preset Folder*.
- **☆ / ★** adds or removes the current preset from favourites.
- **SAVE** stores the current sound, including chord memory, as a user preset. Saving with an
  existing name asks before replacing it.
- **DEL** deletes the current user preset. Factory presets can't be deleted.

User presets are `.p6preset` files in `~/Library/Audio/Presets/Ticco Ross/Polytwelve/`, so you can
back them up or share them. Favourites are stored in `favourites.txt` in the same folder.
Your Live set also remembers which preset was loaded. The first time it runs, Polytwelve copies over
any presets saved with the earlier "Polysix Port" build.

## How it maps to the ensemble

| Reaktor macro | Here |
|---|---|
| VCO: Multi-Wave Osc, Waveform compares, PWM macro | Band-limited saw / pulse / PWM. Pulse width 50–95 %; PWM is a triangle LFO (0.1–12.8 Hz) |
| VCO: Freq. Divider sub (÷2 / ÷4), added via Relay | Flip-flop divider locked to the main cycle, band-limited |
| VCO: Octave, MG × 12 | 16'/8'/4', MG→VCO up to ±12 st (Level knob is squared for finer control) |
| VCF: Pro-52 Filter + cutoff sum | 4-pole zero-delay ladder LP with tanh feedback. Cutoff = 21 + knob·110 semitones + (P−60)·KbdTrack + EG·Int·60 + MG·48 |
| EG: ADSR with D/R random offsets | Exponential ADSR. Times use Reaktor's log units (0 = 1 ms, 40 = 100 ms, 60 = 1 s); knob 0–10 = 0–80 units |
| VCA: Mixer ±10 dB, EG/Gate relay, slew-limited gate, −MG tremolo | Same |
| MG: Ctr.Shp breakpoints, Slow Random rate wobble, AR-Env delay, Distributor | Same curve (0.01 / 1 / 8 / 15 / 20 Hz), per-voice MG, delayed fade-in, VCO/VCF/VCA routing. Mod wheel always adds up to 2 st vibrato |
| Voice: V/F/D/R Tune, Detune, Analog | Fixed per-voice offsets plus slow drift. Analog scales pitch (±0.12 st + drift), cutoff (±2 st), decay/release; Detune adds up to ±25 cents per voice |
| Voice: Poly / Unison / Chord Memory, Hold | Round-robin poly; Unison stacks all 12 voices; **Chord Memory works here**; Hold latches notes (CC64 sustain does the same) |
| Arpeggiator | Up / Down / Up-Down, 1 Oct / 2 Oct / Full (4 oct), Latch, 50 % gate. Free speed or synced to host tempo |
| Effects: Chorus / KleinStein phaser / Ensemble | Rebuilt from scratch with the same single Speed/Intensity knob; output is stereo |

**Chord Memory:** hold a chord and press **MEMORY** (next to the Voice Mode selector). This stores
the chord and switches to Chord mode, and the LED lights. Each key then plays that chord, transposed.
Pressing MEMORY with no keys held just switches Chord mode on with the stored chord. The **CHORD
MEMORY** read-out shows the stored notes (Ableton naming, middle C = C3) and the chord's name.
The chord is saved with presets and with your Live set. The default chord is a C minor triad.

## Known differences / to verify against the ensemble

- The knob ranges for Cutoff, EG Intensity, Kbd Track, the envelope times, Analog and Detune
  were chosen musically, not read from the ensemble.
- The VCF MG depth reads "606" in the screenshot; that's implausible, so it's set to 48 semitones.
- The ensemble adds a sine at the cutoff frequency (selected from the VCO's PW output); it's
  left out because this ladder filter self-oscillates on its own.
- The PWM, Chorus, KleinStein and Ensemble macro internals weren't visible, so these are new designs.
- The on-screen keyboard is left out; it uses MIDI only.

## Engine test (no JUCE)

```sh
g++ -O2 -std=c++17 -ISource tools/render_test.cpp -o render_test && ./render_test
```

## Making an installer (.pkg)

After a Release build, run:

```sh
./tools/make_pkg.sh
```

This creates `dist/Polytwelve-<version>.pkg`, which installs the AU, VST3 and standalone app
system-wide (`/Library/Audio/Plug-Ins/...` and `/Applications`). Each one can be deselected under Customize.

**Unsigned installers (free):** on other Macs, Gatekeeper blocks the first open. To get past it, go to
System Settings → Privacy & Security → "Open Anyway".

**Signed and notarized** (needs the Apple Developer Program, 99 USD/year), so it installs with no warnings:

1. In Xcode → Settings → Accounts → Manage Certificates, create a **Developer ID Application**
   and a **Developer ID Installer** certificate.
2. Make an app-specific password at account.apple.com, then store it once:
   `xcrun notarytool store-credentials polytwelve-notary --apple-id you@example.com --team-id TEAMID`
3. Build the signed installer:
   ```sh
   DEV_ID_APP="Developer ID Application: Your Name (TEAMID)" \
   DEV_ID_INSTALLER="Developer ID Installer: Your Name (TEAMID)" \
   NOTARY_PROFILE="polytwelve-notary" ./tools/make_pkg.sh
   ```
   `security find-identity -v` lists your exact certificate names.

**Uninstall:** delete `/Library/Audio/Plug-Ins/Components/Polytwelve.component`,
`/Library/Audio/Plug-Ins/VST3/Polytwelve.vst3` and `/Applications/Polytwelve.app`.

## Automatic builds (GitHub Actions)

Each push to `main` builds Polytwelve on GitHub's macOS, Windows and Linux machines
(`.github/workflows/build.yml`). The finished files appear under the run's **Artifacts**
in the repository's **Actions** tab:

| Platform | Files |
|---|---|
| macOS | `Polytwelve-<v>-macOS.pkg` (AU + VST3 + app), plus AU and VST3 zips |
| Windows | `Polytwelve-<v>-Windows-Setup.exe` (VST3 + app), plus a VST3 zip |
| Linux | `Polytwelve-<v>-Linux-x86_64.tar.gz` (VST3 + LV2 + app, with `install.sh`) |

**Publishing a release:** raise the version in `CMakeLists.txt`, commit, then tag it, e.g.
`git tag v0.4.0 && git push --tags`. GitHub builds everything and attaches all the installers
to a new release on the repository's Releases page.

The installers are unsigned. On first open, macOS shows a Gatekeeper warning (fix: Privacy &
Security → Open Anyway) and Windows shows SmartScreen (fix: More info → Run anyway).

Preset folders: macOS `~/Library/Audio/Presets/Ticco Ross/Polytwelve`,
Windows `Documents\Ticco Ross\Polytwelve\Presets`, Linux `~/.local/share/Ticco Ross/Polytwelve/Presets`.
