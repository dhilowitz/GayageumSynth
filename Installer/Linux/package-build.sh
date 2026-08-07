#!/bin/bash

set -e

die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -eq 1 ] || die "1 argument required, $# provided"
echo $1 | grep -E -q '^[0-9]+\.[0-9]+\.[0-9]+$' || die "Build number required, $1 provided"

TARGET_NAME="GayageumSynth"

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

    cp ./Output/*.tar.gz ~/Dropbox/Public/Builds/GayageumSynth/
done
