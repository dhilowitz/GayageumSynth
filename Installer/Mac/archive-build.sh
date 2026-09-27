#!/bin/bash
# Produces Installer/Mac/Archive via a Release archive build - the input
# package-build.sh expects (Archive/Products/Applications/GayageumSynth.app etc).
# Run from Installer/Mac. Safe to re-run: wipes any previous Archive first.
set -e
cd "$(dirname "$0")"

rm -rf Archive Archive.xcarchive

# Any arguments go to xcodebuild as extra build settings, e.g. release-build.sh's
# CODE_SIGNING_ALLOWED=NO for an account with no signing identity.
xcodebuild -project ../../Builds/MacOSX/GayageumSynth.xcodeproj -scheme "GayageumSynth - All" -configuration Release archive -archivePath Archive "$@"

# xcodebuild always appends .xcarchive to -archivePath unless it's already
# there, so a plain "-archivePath Archive" actually produces Archive.xcarchive.
# package-build.sh expects the plain "Archive" name - normalize it here so
# nobody has to remember to rename it by hand after every archive build.
mv Archive.xcarchive Archive

echo ""
echo "Archive completed successfully: $(pwd)/Archive"
