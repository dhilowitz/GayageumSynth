#!/bin/bash
set -e
cd "$(dirname "$0")"

TARGET_NAME="GayageumSynth"

./apply-patch.sh
cd ../Builds/LinuxMakefile

echo "Doing STATIC Linux build..."

mkdir -p build-static
env PKG_CONFIG_PATH=/home/dhilowitz/lib/lib/pkgconfig make CONFIG=Release
ldd "build/$TARGET_NAME"
cp "build/$TARGET_NAME" build-static/
cp -r "build/$TARGET_NAME.vst3" build-static/

echo "Doing DYNAMIC Linux build..."

mkdir -p build-dynamic
make clean
make CONFIG=DynRelease
ldd "build/$TARGET_NAME"
cp "build/$TARGET_NAME" build-dynamic/
cp -r "build/$TARGET_NAME.vst3" build-dynamic/

echo ""
echo "Build completed successfully."
