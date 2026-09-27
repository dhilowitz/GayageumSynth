#!/bin/bash
set -e
cd "$(dirname "$0")"

TARGET_NAME="GayageumSynth"

./apply-patch.sh
cd ../Builds/LinuxMakefile

echo "Doing STATIC Linux build..."

mkdir -p build-static
# Objects from another branch or version must never be reused.
rm -rf build
env PKG_CONFIG_PATH="$HOME/lib/lib/pkgconfig" make CONFIG=Release -j"${MAKE_JOBS:-$(nproc)}"
ldd "build/$TARGET_NAME"
cp "build/$TARGET_NAME" build-static/
cp -r "build/$TARGET_NAME.vst3" build-static/

echo "Doing DYNAMIC Linux build..."

mkdir -p build-dynamic
make clean
make CONFIG=DynRelease -j"${MAKE_JOBS:-$(nproc)}"
ldd "build/$TARGET_NAME"
cp "build/$TARGET_NAME" build-dynamic/
cp -r "build/$TARGET_NAME.vst3" build-dynamic/

echo ""
echo "Build completed successfully."
