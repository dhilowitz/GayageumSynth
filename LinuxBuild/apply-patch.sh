#!/bin/bash
# Reapplies this project's custom Linux build tweaks (static-linking patch +
# linker version-script symbol file) on top of a freshly Projucer-regenerated
# Builds/LinuxMakefile. Safe to re-run - skips the patch if already applied.
set -e
cd "$(dirname "$0")"

TARGET="../Builds/LinuxMakefile"
MAKEFILE="$TARGET/Makefile"

if [ ! -f "$MAKEFILE" ]; then
  echo "error: $MAKEFILE not found." >&2
  echo "Regenerate it first: open the .jucer in Projucer and save (or run Projucer --resave <project>.jucer)." >&2
  exit 1
fi

mkdir -p "$TARGET/symbols"
cp symbols/vst3.version "$TARGET/symbols/"

if ! patch -R -p1 -s -f --dry-run "$MAKEFILE" < make_static.patch > /dev/null 2>&1; then
  patch -N -p1 "$MAKEFILE" < make_static.patch
fi
