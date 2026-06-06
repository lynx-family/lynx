#!/usr/bin/env bash
# Package the iOS xcframeworks produced by build-xcframeworks.sh into
# individual SwiftPM-ready `.zip` files + SHA-256 checksums Whisker
# consumers paste into `Package.swift`'s `binaryTarget(url:, checksum:)`.
#
# Companion of `stage-maven.sh` on the Android side. Same release tag
# / same fork build, different distribution channel.
#
# `zip -y` preserves symlinks inside the xcframework (`Headers/`,
# `Modules/`, etc. inside each platform slice). Without `-y` the
# unzip in Xcode flattens them and the bundle fails to load at
# runtime.
#
# SwiftPM expects the checksum produced by `swift package
# compute-checksum`, NOT a raw `shasum -a 256`. The two formats
# differ (compute-checksum frames bytes through a Swift Package
# Manager-internal helper), so the consumer side's `binaryTarget`
# only validates against the compute-checksum output. Use the
# `swift` command (Xcode toolchain) here.
set -eu -o pipefail

VER="${VERSION:?VERSION env var required}"
XCF_DIR="${XCF_DIR:-${GITHUB_WORKSPACE:-$(pwd)}/lynx-ios}"
OUT_DIR="${OUT_DIR:-${GITHUB_WORKSPACE:-$(pwd)}/swiftpm-out}"

mkdir -p "$OUT_DIR"

if ! command -v swift >/dev/null; then
  echo "::error::swift command missing (xcframework checksum step needs the Xcode toolchain)"
  exit 1
fi

# Maps each xcframework subdir name → published artefact base name.
# Matches the four targets emitted by build-xcframeworks.sh.
FRAMEWORKS=(Lynx LynxBase LynxServiceAPI PrimJS)

for FRAMEWORK in "${FRAMEWORKS[@]}"; do
  SRC="$XCF_DIR/$FRAMEWORK.xcframework"
  if [ ! -d "$SRC" ]; then
    echo "::error::xcframework not produced: $SRC"
    exit 1
  fi
  ZIP_NAME="$FRAMEWORK-$VER.xcframework.zip"
  (cd "$XCF_DIR" && zip -ryq "$OUT_DIR/$ZIP_NAME" "$FRAMEWORK.xcframework")

  CHECKSUM=$(swift package compute-checksum "$OUT_DIR/$ZIP_NAME")
  printf '%s\n' "$CHECKSUM" > "$OUT_DIR/$ZIP_NAME.sha256"
  printf '%-20s %s\n' "$FRAMEWORK" "$CHECKSUM"
done

# `swiftpm-manifest-<ver>.txt`: machine-readable `<framework>=<checksum>`
# pair list. Cheaper for whisker-cng's Package.swift template to read
# this file once per fork version bump than to parse the release page.
MANIFEST="$OUT_DIR/swiftpm-manifest-$VER.txt"
: > "$MANIFEST"
for FRAMEWORK in "${FRAMEWORKS[@]}"; do
  CHECKSUM=$(cat "$OUT_DIR/$FRAMEWORK-$VER.xcframework.zip.sha256")
  printf '%s=%s\n' "$FRAMEWORK" "$CHECKSUM" >> "$MANIFEST"
done
cat "$MANIFEST"

ls -lh "$OUT_DIR"
