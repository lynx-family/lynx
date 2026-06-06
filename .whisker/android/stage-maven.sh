#!/usr/bin/env bash
# Lay the four Whisker-fork Android AARs out as a Maven 2 repository
# under ${REPO_DIR} (default: $GITHUB_WORKSPACE/maven-out). The
# build-whisker-tarballs.yml workflow runs this after the gradle
# `assembleNoasanRelease` step and before the gh-pages publish step.
#
# Coordinates: `rs.whisker:<artifact-id>:<version>`. The four
# artifact IDs map to the Lynx subprojects 1:1 (`lynx-android`,
# `lynx-base-android`, `lynx-trace-android`, `lynx-service-api-android`).
#
# Why hand-written POMs (not `maven-publish`):
# the Lynx fork's gradle modules don't surface stable inter-module
# deps through `maven-publish`; the consumer side pins all four AARs
# together, so a hand-written POM keeps this script simple while
# producing exactly what Gradle's resolver needs (groupId / artifactId
# / version / packaging + md5/sha1 sidecars).
set -eu -o pipefail

VER="${VERSION:?VERSION env var required}"
REPO_DIR="${REPO_DIR:-${GITHUB_WORKSPACE:-$(pwd)}/maven-out}"
LYNX_ROOT="${LYNX_ROOT:-lynx}"
GROUP_PATH="rs/whisker"

publish_aar() {
  local SRC="$1"
  local ARTIFACT_ID="$2"
  if [ ! -f "$SRC" ]; then
    echo "::error::AAR not produced for Maven publish: $SRC"
    exit 1
  fi
  local DEST_DIR="$REPO_DIR/$GROUP_PATH/$ARTIFACT_ID/$VER"
  mkdir -p "$DEST_DIR"
  cp "$SRC" "$DEST_DIR/$ARTIFACT_ID-$VER.aar"

  cat > "$DEST_DIR/$ARTIFACT_ID-$VER.pom" <<POM
<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://maven.apache.org/POM/4.0.0"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="http://maven.apache.org/POM/4.0.0 http://maven.apache.org/xsd/maven-4.0.0.xsd">
  <modelVersion>4.0.0</modelVersion>
  <groupId>rs.whisker</groupId>
  <artifactId>$ARTIFACT_ID</artifactId>
  <version>$VER</version>
  <packaging>aar</packaging>
  <name>$ARTIFACT_ID</name>
  <description>Lynx fork AAR for Whisker $VER</description>
  <url>https://github.com/whiskerrs/lynx</url>
  <licenses>
    <license>
      <name>Apache License, Version 2.0</name>
      <url>https://www.apache.org/licenses/LICENSE-2.0.txt</url>
    </license>
  </licenses>
</project>
POM

  # md5 + sha1 sidecars suppress Gradle resolver warnings. Gradle
  # tolerates their absence (falls back, warns) but writing them out
  # is cheap and avoids noisy consumer logs.
  (cd "$DEST_DIR" \
    && md5sum "$ARTIFACT_ID-$VER.aar" | awk '{print $1}' > "$ARTIFACT_ID-$VER.aar.md5" \
    && shasum "$ARTIFACT_ID-$VER.aar" | awk '{print $1}' > "$ARTIFACT_ID-$VER.aar.sha1" \
    && md5sum "$ARTIFACT_ID-$VER.pom" | awk '{print $1}' > "$ARTIFACT_ID-$VER.pom.md5" \
    && shasum "$ARTIFACT_ID-$VER.pom" | awk '{print $1}' > "$ARTIFACT_ID-$VER.pom.sha1")

  # Per-artifact maven-metadata.xml. `keep_files: true` on the
  # gh-pages publish preserves prior versions, so each release
  # appends to the on-disk history; a future PR can add a
  # "rebuild master index" job that walks gh-pages and re-emits
  # this file with the full version list.
  local META_DIR="$REPO_DIR/$GROUP_PATH/$ARTIFACT_ID"
  cat > "$META_DIR/maven-metadata.xml" <<META
<?xml version="1.0" encoding="UTF-8"?>
<metadata>
  <groupId>rs.whisker</groupId>
  <artifactId>$ARTIFACT_ID</artifactId>
  <versioning>
    <latest>$VER</latest>
    <release>$VER</release>
    <versions>
      <version>$VER</version>
    </versions>
    <lastUpdated>$(date -u +%Y%m%d%H%M%S)</lastUpdated>
  </versioning>
</metadata>
META
  (cd "$META_DIR" \
    && md5sum maven-metadata.xml | awk '{print $1}' > maven-metadata.xml.md5 \
    && shasum maven-metadata.xml | awk '{print $1}' > maven-metadata.xml.sha1)
}

cd "$LYNX_ROOT"
publish_aar "base/platform/android/build/outputs/aar/LynxBase-noasan-release.aar"             "lynx-base-android"
publish_aar "base/trace/android/build/outputs/aar/LynxTrace-noasan-release.aar"               "lynx-trace-android"
publish_aar "platform/android/lynx_android/build/outputs/aar/LynxAndroid-noasan-release.aar"  "lynx-android"
publish_aar "platform/android/service_api/build/outputs/aar/ServiceAPI-noasan-release.aar"    "lynx-service-api-android"

echo "::group::Maven repo layout"
find "$REPO_DIR" -type f | sort
echo "::endgroup::"
