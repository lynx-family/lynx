#!/usr/bin/env bash
# Pack the build-xcframeworks.sh output into the
# `whisker-lynx-ios-<version>.tar.gz` layout the
# `whiskerrs/whisker::whisker-build::lynx` fetcher expects.
#
# Inputs:
#   $LYNX_BUILD_OUT  — dir produced by build-xcframeworks.sh
#                      (default: $(pwd)/lynx-ios)
#   $VERSION         — tarball version segment (default: 0.0.0-whisker.dev)
#
# Output:
#   $(pwd)/whisker-lynx-ios-<VERSION>.tar.gz
#   $(pwd)/whisker-lynx-ios-<VERSION>.tar.gz.sha256
#
# Layout:
#   whisker-lynx-ios-<VERSION>/
#   ├── Lynx.xcframework/
#   ├── LynxBase.xcframework/
#   ├── LynxServiceAPI.xcframework/
#   ├── PrimJS.xcframework/
#   └── headers/{Lynx,LynxBase,LynxServiceAPI,PrimJS}/
#
# The headers tree under each <pod>/ contains the .h files from the
# pod source, mirroring the upstream directory structure — same
# layout the Android tarball uses, so `whisker-driver-sys/build.rs`
# picks them up with identical -I paths.

set -euo pipefail

LYNX_BUILD_OUT="${LYNX_BUILD_OUT:-$(pwd)/lynx-ios}"
VERSION="${VERSION:-0.0.0-whisker.dev}"
STAGE_NAME="whisker-lynx-ios-$VERSION"
STAGE="$(pwd)/$STAGE_NAME"
TARBALL="$(pwd)/$STAGE_NAME.tar.gz"

echo "==> LYNX_BUILD_OUT=$LYNX_BUILD_OUT"
echo "==> VERSION=$VERSION"
echo "==> STAGE=$STAGE"

if [ ! -d "$LYNX_BUILD_OUT" ]; then
  echo "::error::expected build output dir $LYNX_BUILD_OUT — did build-xcframeworks.sh run?"
  exit 1
fi

rm -rf "$STAGE"
mkdir -p "$STAGE"

# ----- xcframeworks ----------------------------------------------------------

echo "==> Copy xcframeworks"
PODS=(Lynx PrimJS LynxBase LynxServiceAPI)
for fw in "${PODS[@]}"; do
  SRC="$LYNX_BUILD_OUT/$fw.xcframework"
  if [ ! -d "$SRC" ]; then
    echo "::error::$fw.xcframework missing at $SRC"
    exit 1
  fi
  cp -R "$SRC" "$STAGE/"
done

# ----- Headers ---------------------------------------------------------------

echo "==> Copy headers/"
if [ ! -d "$LYNX_BUILD_OUT/headers" ]; then
  echo "::error::headers/ missing at $LYNX_BUILD_OUT/headers — did build-xcframeworks.sh stage them?"
  exit 1
fi
cp -R "$LYNX_BUILD_OUT/headers" "$STAGE/"

# ----- Tarball ---------------------------------------------------------------

echo "==> Pack tarball"
tar czf "$TARBALL" -C "$(dirname "$STAGE")" "$STAGE_NAME"
shasum -a 256 "$TARBALL" | tee "$TARBALL.sha256"
ls -lh "$TARBALL"
