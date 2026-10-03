#!/bin/bash

set -e

die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -eq 1 ] || die "1 argument required, $# provided"
echo $1 | grep -E -q '^[0-9]+\.[0-9]+\.[0-9]+$' || die "Build number required, $1 provided"

TARGET_NAME="GayageumSynth"

cd "$(dirname "$0")"
ARTIFACTS=~/BuildArtifacts/GayageumSynth
mkdir -p "$ARTIFACTS"
# The commit these packages are built from, for the release handoff's
# provenance check. Dropped first, so a failed run never leaves it next to
# older packages.
COMMIT_FILE="$ARTIFACTS/GayageumSynth-$1-Linux-x86_64.commit"
rm -f "$COMMIT_FILE"

for product_format in "Static" "Dynamic"
do
    echo "Packaging the $product_format build..."

    BUILDFILE="GayageumSynth-$1-Linux-$product_format-x86_64"
    mkdir -p Output
    cd Output

    rm -rf "$BUILDFILE"
    mkdir "$BUILDFILE"

    if [ "$product_format" == "Static" ]
    then
        SOURCE_DIR=../../../Builds/LinuxMakefile/build-static
    else
        SOURCE_DIR=../../../Builds/LinuxMakefile/build-dynamic
    fi

    cp -r "$SOURCE_DIR/$TARGET_NAME.vst3" "$BUILDFILE/"
    cp "$SOURCE_DIR/$TARGET_NAME" "$BUILDFILE/"

    tar -czvf "$BUILDFILE.tar.gz" "$BUILDFILE"
    cd ..

    # Left where the release handoff fetches them from.
    cp "./Output/$BUILDFILE.tar.gz" "$ARTIFACTS/"
done

git rev-parse HEAD > "$COMMIT_FILE.partial"
mv "$COMMIT_FILE.partial" "$COMMIT_FILE"
