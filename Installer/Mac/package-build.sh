#!/bin/bash

set -e

die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -eq 1 ] || die "1 argument required, $# provided"
echo $1 | grep -E -q '^[0-9]+\.[0-9]+\.[0-9]+$' || die "Build number required, $1 provided"

BUILDFILE="GayageumSynth-$1-Mac"
UNSIGNED_BUILDFILE="GayageumSynth-$1-Mac_Unsigned"

# Backup debug symbols, if any were produced. The archive's own dSYMs/
# folder is the canonical source for an archive build (not Builds/MacOSX/
# build/Release/, which only symlinks the .app/.vst3/.component themselves).
# GayageumSynth's Release config currently uses DEBUG_INFORMATION_FORMAT=dwarf
# (no separate .dSYM bundles), unlike the older Decidedly projects this
# script was modeled on, so this step is a no-op today - kept in case that
# setting ever changes.
mkdir -p ../../Debug\ Symbols/Mac/$1
if compgen -G "Archive/dSYMs/*.dSYM" > /dev/null; then
    cp -r Archive/dSYMs/*.dSYM ../../Debug\ Symbols/Mac/$1/
fi

# No AAX - GayageumSynth doesn't ship it, unlike the other Decidedly plugins.

# Standalone
# Let's make sure this thing gets the proper perms (we had an issue with this on other projects)
chmod uga+x Archive/Products/Applications/GayageumSynth.app/Contents/MacOS/GayageumSynth
codesign --force -s "Developer ID Application: Decidedly, LLC (5K8EG37W74)" --timestamp --options runtime --deep Archive/Products/Applications/GayageumSynth.app
# AU
codesign --force -s "Developer ID Application: Decidedly, LLC (5K8EG37W74)" --timestamp --options runtime --deep Archive/Products/Users/dhilowitz/Library/Audio/Plug-Ins/Components/GayageumSynth.component
# VST3
codesign --force -s "Developer ID Application: Decidedly, LLC (5K8EG37W74)" --timestamp --options runtime --deep Archive/Products/Users/dhilowitz/Library/Audio/Plug-Ins/VST3/GayageumSynth.vst3

# Update installer version (Regular version)
# NOTE: index count (0..2) assumes a 3-package pkgproj (VST3/AU/Standalone,
# no VST2, no factory-presets package). Adjust if the pkgproj built in
# Packages.app ends up with a different package count/order.
/usr/libexec/PlistBuddy -c "Set :PACKAGES:0:PACKAGE_SETTINGS:VERSION $1" GayageumSynth.pkgproj
/usr/libexec/PlistBuddy -c "Set :PACKAGES:1:PACKAGE_SETTINGS:VERSION $1" GayageumSynth.pkgproj
/usr/libexec/PlistBuddy -c "Set :PACKAGES:2:PACKAGE_SETTINGS:VERSION $1" GayageumSynth.pkgproj
/usr/libexec/PlistBuddy -c "Set :PROJECT:PROJECT_SETTINGS:NAME $BUILDFILE" GayageumSynth.pkgproj

# Update installer version (Unsigned version)
/usr/libexec/PlistBuddy -c "Set :PACKAGES:0:PACKAGE_SETTINGS:VERSION $1" GayageumSynth_Unsigned.pkgproj
/usr/libexec/PlistBuddy -c "Set :PACKAGES:1:PACKAGE_SETTINGS:VERSION $1" GayageumSynth_Unsigned.pkgproj
/usr/libexec/PlistBuddy -c "Set :PACKAGES:2:PACKAGE_SETTINGS:VERSION $1" GayageumSynth_Unsigned.pkgproj
/usr/libexec/PlistBuddy -c "Set :PROJECT:PROJECT_SETTINGS:NAME $UNSIGNED_BUILDFILE" GayageumSynth_Unsigned.pkgproj

echo "Building GayageumSynth.pkgproj..."
packagesbuild GayageumSynth.pkgproj
echo "Building GayageumSynth_Unsigned.pkgproj..."
packagesbuild GayageumSynth_Unsigned.pkgproj

cd Output
echo "Notarizing $BUILDFILE.pkg..."
dh-notarize-app --primary-bundle-id=ly.decided.GayageumSynth "$BUILDFILE.pkg"

mkdir -p GayageumSynth
cp "$BUILDFILE.pkg" GayageumSynth/
ditto -c -k --sequesterRsrc "GayageumSynth" "$BUILDFILE.zip"
rm -rf GayageumSynth

mkdir -p GayageumSynth
cp "$UNSIGNED_BUILDFILE.pkg" GayageumSynth/
ditto -c -k --sequesterRsrc "GayageumSynth" "$UNSIGNED_BUILDFILE.zip"
rm -rf GayageumSynth

cp "$BUILDFILE.zip" ~/Dropbox/Public/Builds/GayageumSynth/
cp "$UNSIGNED_BUILDFILE.zip" ~/Dropbox/Public/Builds/GayageumSynth/
