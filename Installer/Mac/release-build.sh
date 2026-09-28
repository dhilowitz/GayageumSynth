#!/bin/bash
# Unprivileged Mac release build for GayageumSynth. It holds no signing
# credentials and builds no installer. Two steps belong to the trusted
# signing side, and this script stops and says so when it reaches one of
# them. Ported from DecentSampler's Installer/Mac/release-build.sh (by way of
# Equations); keep them in step.
#
#   1. Build the archive without signing and publish it to
#        /Users/Shared/BuildHandoff/GayageumSynth/<v>/Archive       (+ BUILD_INFO)
#   2. [trusted]  The bundles are signed; only a receipt is published:
#        /Users/Shared/SignedHandoff/GayageumSynth/<v>/mac-archive/<run>/RECEIPT.json
#   3. [trusted]  The installer is built, signed, notarized and zipped:
#        /Users/Shared/SignedHandoff/GayageumSynth/<v>/mac-pkg/<run>/
#          GayageumSynth-<v>-Mac.zip, GayageumSynth-<v>-Mac_Unsigned.zip, RECEIPT.json
#   4. Copy those two zips, byte for byte, into ~/BuildArtifacts/GayageumSynth/,
#      where staging fetches them with GayageumSynth-<v>-Mac.commit.
#
#   release-build.sh <version X.Y.Z> [--fresh]
#
# Each run continues from whatever state the handoff folders are in. A
# trusted step's output only counts if its receipt says it was made from this
# build: the archive receipt must name the SHA-256 of the current BUILD_INFO,
# and the package receipt the current archive run. Anything else is waited
# for again.
#
# The commit that built the published archive is recorded in BUILD_INFO. When
# HEAD moves on, or with --fresh, the BuildHandoff folder for this version is
# cleared and the archive is rebuilt.
#
# Exit status: 0 when the zips are ready, 10 while waiting for a trusted
# signing step, anything else on failure.
set -euo pipefail

EXIT_AWAITING_SIGNING=10

die () {
    echo >&2 "error: $*"
    exit 1
}

[ "$#" -ge 1 ] && [ "$#" -le 2 ] || die "usage: $0 <version X.Y.Z> [--fresh]"
VERSION="$1"
echo "$VERSION" | grep -E -q '^[0-9]+\.[0-9]+\.[0-9]+$' || die "version must be X.Y.Z, got '$VERSION'"
FRESH=0
if [ "$#" -eq 2 ]; then
    [ "$2" = "--fresh" ] || die "unknown option '$2'"
    FRESH=1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COMMIT="$(git -C "$SCRIPT_DIR" rev-parse HEAD)"

PRODUCT=GayageumSynth
PREFIX=GayageumSynth
BUILD_DIR="/Users/Shared/BuildHandoff/$PRODUCT/$VERSION"
SIGNED_DIR="/Users/Shared/SignedHandoff/$PRODUCT/$VERSION"
ARTIFACTS_DIR="$HOME/BuildArtifacts/$PRODUCT"

BUILD_INFO="$BUILD_DIR/BUILD_INFO"
UNSIGNED_ARCHIVE="$BUILD_DIR/Archive"
UNSIGNED_ZIP="$ARTIFACTS_DIR/$PREFIX-$VERSION-Mac_Unsigned.zip"
SIGNED_ZIP="$ARTIFACTS_DIR/$PREFIX-$VERSION-Mac.zip"
# The commit the zips were built from, for the release handoff's provenance check.
COMMIT_FILE="$ARTIFACTS_DIR/$PREFIX-$VERSION-Mac.commit"
# Which trusted run the zips came from; not fetched by staging.
ZIPS_SOURCE="$ARTIFACTS_DIR/$PREFIX-$VERSION-Mac.source.txt"

# Finished zips stay only while a run can vouch for them: any run that ends
# without confirming them (waiting for signing, failing, rebuilding) removes
# them, so staging can never pick up zips from an older build.
remove_zips () {
    rm -f "$SIGNED_ZIP" "$UNSIGNED_ZIP" "$ZIPS_SOURCE" "$COMMIT_FILE" "$SIGNED_ZIP.partial" "$UNSIGNED_ZIP.partial"
}

trap 'rc=$?; [ "$rc" -eq 0 ] || remove_zips' EXIT

# Says which trusted step it is waiting for, then exits with the "awaiting"
# status. The GATE: line is a fixed identifier (mac-archive or mac-pkg) that
# the release tooling records as this build's state.
await_signing () {
    local gate="$1" reason="$2"
    echo ""
    echo "$reason"
    echo "GATE: $gate"
    echo "ACTION: waiting for the trusted signing step $gate"
    exit "$EXIT_AWAITING_SIGNING"
}

sha256 () { shasum -a 256 < "$1" | awk '{ print $1 }'; }

# receipt_value <receipt> <key>...: one string from a trusted broker's receipt.
receipt_value () {
    /usr/bin/python3 -I -c 'import json, sys
v = json.load(open(sys.argv[1]))
for k in sys.argv[2:]:
    v = v[k]
print(v)' "$@" 2>/dev/null
}

# current_run <gate>: the run the gate's .current pointer names, if its receipt is there.
current_run () {
    local run
    run="$(cat "$SIGNED_DIR/$1.current" 2>/dev/null || true)"
    [[ "$run" =~ ^[0-9a-f]{32}$ ]] && [ -f "$SIGNED_DIR/$1/$run/RECEIPT.json" ] && echo "$run"
}

# --- 1. Archive --------------------------------------------------------------
if [ -f "$BUILD_INFO" ]; then
    BUILT_COMMIT="$(sed -n 's/^commit=//p' "$BUILD_INFO")"
    if [ "$FRESH" -eq 1 ]; then
        echo "--fresh: discarding the published $VERSION build ($BUILT_COMMIT)."
        rm -rf "$BUILD_DIR"
    elif [ "$BUILT_COMMIT" != "$COMMIT" ]; then
        echo "The published $VERSION build is from $BUILT_COMMIT; HEAD is now $COMMIT. Rebuilding."
        rm -rf "$BUILD_DIR"
    fi
elif [ -e "$BUILD_DIR" ]; then
    echo "$BUILD_DIR has no BUILD_INFO, so its commit is unknown. Rebuilding."
    rm -rf "$BUILD_DIR"
fi

if [ ! -f "$BUILD_INFO" ]; then
    echo "Building the unsigned $VERSION archive from $COMMIT..."
    # No identity is available to this account; the trusted step signs.
    "$SCRIPT_DIR/archive-build.sh" CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO

    # Publish under a temporary name first so the signing script can never
    # pick up a half-copied archive.
    mkdir -p "$BUILD_DIR"
    rm -rf "$UNSIGNED_ARCHIVE.partial"
    ditto "$SCRIPT_DIR/Archive" "$UNSIGNED_ARCHIVE.partial"
    mv "$UNSIGNED_ARCHIVE.partial" "$UNSIGNED_ARCHIVE"
    {
        echo "version=$VERSION"
        echo "commit=$COMMIT"
        echo "branch=$(git -C "$SCRIPT_DIR" rev-parse --abbrev-ref HEAD)"
        echo "built=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "host=$(hostname -s)"
    } > "$BUILD_INFO"
    chmod -R a+rX "$BUILD_DIR"
    echo "Published $UNSIGNED_ARCHIVE"
fi

# --- 2. Signed archive (trusted) -----------------------------------------------
ARCHIVE_RUN="$(current_run mac-archive || true)"
[ -n "$ARCHIVE_RUN" ] \
    || await_signing mac-archive "Waiting for the archive to be signed: no mac-archive receipt for $VERSION yet."
[ "$(receipt_value "$SIGNED_DIR/mac-archive/$ARCHIVE_RUN/RECEIPT.json" inputs build_info_sha256)" = "$(sha256 "$BUILD_INFO")" ] \
    || await_signing mac-archive "The newest signed archive (run $ARCHIVE_RUN) was made from an earlier build, not the one now in $BUILD_DIR."
echo "Signed archive: run $ARCHIVE_RUN, made from this build."

# --- 3. Installer (trusted) ------------------------------------------------------
PKG_RUN="$(current_run mac-pkg || true)"
[ -n "$PKG_RUN" ] \
    || await_signing mac-pkg "Waiting for the installer to be built, signed and notarized: no mac-pkg receipt for $VERSION yet."
PKG_RECEIPT="$SIGNED_DIR/mac-pkg/$PKG_RUN/RECEIPT.json"
[ "$(receipt_value "$PKG_RECEIPT" inputs archive_run_id)" = "$ARCHIVE_RUN" ] \
    || await_signing mac-pkg "The newest installer (run $PKG_RUN) was built from a different signed archive than run $ARCHIVE_RUN."
echo "Installer: run $PKG_RUN, built from signed archive run $ARCHIVE_RUN."

# --- 4. Zips for staging -----------------------------------------------------------
# Copied only when missing or different, so a resume leaves them as they are.
mkdir -p "$ARTIFACTS_DIR"
zips_from="mac_pkg_run=$PKG_RUN"
if [ "$(cat "$ZIPS_SOURCE" 2>/dev/null)" != "$zips_from" ]; then
    remove_zips
fi
for zip in "$SIGNED_ZIP" "$UNSIGNED_ZIP"; do
    name="$(basename "$zip")"
    want="$(receipt_value "$PKG_RECEIPT" outputs "$name" sha256)"
    [[ "$want" =~ ^[0-9a-f]{64}$ ]] || die "the mac-pkg receipt has no hash for $name"
    if [ ! -f "$zip" ] || [ "$(sha256 "$zip")" != "$want" ]; then
        cp "$SIGNED_DIR/mac-pkg/$PKG_RUN/$name" "$zip.partial"
        [ "$(sha256 "$zip.partial")" = "$want" ] || die "$name does not match its receipt"
        mv -f "$zip.partial" "$zip"
        echo "  copied $name"
    else
        echo "  $name already current"
    fi
done
echo "$zips_from" > "$ZIPS_SOURCE"

# Provenance: the commit the published archive was built from. Rewritten only
# when it differs, so a resume changes nothing.
built_from="$(sed -n 's/^commit=//p' "$BUILD_INFO")"
if [ "$(cat "$COMMIT_FILE" 2>/dev/null)" != "$built_from" ]; then
    echo "$built_from" > "$COMMIT_FILE.partial"
    mv -f "$COMMIT_FILE.partial" "$COMMIT_FILE"
fi

echo ""
echo "Mac $VERSION is ready for staging (built from $COMMIT)."
