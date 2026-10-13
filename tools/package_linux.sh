#!/bin/bash
# Packs a Release build into dist/Polytwelve-<version>-Linux-x86_64.tar.gz (with an install script)
set -euo pipefail
cd "$(dirname "$0")/.."

VERSION=$(sed -nE 's/^project\(Polytwelve VERSION ([0-9.]+)\).*/\1/p' CMakeLists.txt)
ART="build/Polytwelve_artefacts/Release"
NAME="Polytwelve-$VERSION-Linux-$(uname -m)"
STAGE="dist/$NAME"

rm -rf "$STAGE" && mkdir -p "$STAGE"
cp -R "$ART/VST3/Polytwelve.vst3"    "$STAGE/"
cp -R "$ART/LV2/Polytwelve.lv2"      "$STAGE/"
cp    "$ART/Standalone/Polytwelve"   "$STAGE/"

cat > "$STAGE/install.sh" <<'EOF'
#!/bin/bash
# Installs Polytwelve for the current user (no root needed).
set -e
cd "$(dirname "$0")"
mkdir -p ~/.vst3 ~/.lv2 ~/.local/bin
rm -rf ~/.vst3/Polytwelve.vst3 ~/.lv2/Polytwelve.lv2
cp -R Polytwelve.vst3 ~/.vst3/
cp -R Polytwelve.lv2  ~/.lv2/
cp Polytwelve ~/.local/bin/ && chmod +x ~/.local/bin/Polytwelve
echo "Installed: ~/.vst3/Polytwelve.vst3, ~/.lv2/Polytwelve.lv2, ~/.local/bin/Polytwelve"
echo "Rescan plug-ins in your DAW (Bitwig, Reaper, Ardour...)."
EOF

cat > "$STAGE/uninstall.sh" <<'EOF'
#!/bin/bash
rm -rf ~/.vst3/Polytwelve.vst3 ~/.lv2/Polytwelve.lv2 ~/.local/bin/Polytwelve
echo "Polytwelve removed (presets in ~/.local/share/Ticco Ross/Polytwelve are kept)."
EOF
chmod +x "$STAGE/install.sh" "$STAGE/uninstall.sh"

cat > "$STAGE/README.txt" <<EOF
Polytwelve $VERSION - Programmable 12 voice synth - Ticco Ross 2026

Install:    ./install.sh      (VST3 -> ~/.vst3, LV2 -> ~/.lv2, app -> ~/.local/bin)
Uninstall:  ./uninstall.sh
Needs: ALSA or JACK, X11, FreeType (standard on desktop distributions).
EOF

tar -C dist -czf "dist/$NAME.tar.gz" "$NAME"
rm -rf "$STAGE"
echo "==> Done: dist/$NAME.tar.gz"
