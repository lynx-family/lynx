# Whisker fork bits

Everything the Whisker fork adds on top of upstream Lynx. The fork is
intentionally minimal — **no patches against upstream source** as of
v3.7.0-whisker.1. The fork's value is the new C ABI Whisker consumes
(`core/native_renderer_capi/`) plus CI that ships prebuilt tarballs.

## Structure

```
.whisker/
├── ios/
│   ├── build-xcframeworks.sh   — pod-install + xcodebuild Lynx pods
│   └── stage-tarball.sh        — pack the iOS half of the release
└── README.md                   — this file

core/native_renderer_capi/      — additions, not patches
├── BUILD.gn
├── lynx_native_renderer.cc     — thin extern "C" wrappers over Lynx C++
└── public/
    └── lynx_native_renderer_capi.h
```

The native renderer C API exposes the subset of Lynx internals that
Whisker drives (LynxShell + ElementManager + FiberElement family),
plus `lynx_aslr_reference()` as a subsecond hot-patch anchor. Lives
under `core/` rather than `platform/embedder/` because the LynxAndroid
AAR build doesn't include `platform/embedder/`; this way the new C
API is linked into the same `liblynx.so` Whisker depends on.

## Companion repo: whiskerrs/whisker

The Whisker repo at `whiskerrs/whisker` pins this fork's release tag
in `crates/whisker-build/src/lynx.rs`. After cutting a release here:

1. Get the SHA-256 of each tarball from the release page (or the
   `.sha256` sidecar files attached to the release).
2. Update `LYNX_FORK_TAG`, `LYNX_VERSION`, `LYNX_ANDROID_SHA256`,
   and `LYNX_IOS_SHA256` in `crates/whisker-build/src/lynx.rs`.
3. Commit + push the whisker repo. From the next `whisker run` /
   `whisker build` invocation, the new release tarball is
   downloaded + verified + unpacked under
   `~/.cache/whisker/lynx/<version>/`.

## Three publication channels (per release)

Since `v3.8.0-whisker.4` the `build-whisker-tarballs.yml` workflow
ships three flavors of the same Lynx + PrimJS build. Whisker
consumers pick one depending on which side of the build pipeline
they're integrating with:

| Channel | Where it lands | Consumer |
|---|---|---|
| **Tarballs** (`whisker-lynx-{android,ios}-<ver>.tar.gz`) | GitHub Releases | Legacy whisker-build cache (`~/.cache/whisker/lynx/`); will be retired once Maven + SwiftPM are the canonical path |
| **Maven repo** (`rs.whisker:lynx-{android,base-android,trace-android,service-api-android}:<ver>`) | gh-pages branch → `https://whiskerrs.github.io/lynx/maven/` | The new `whisker-gradle-plugin` and any user `build.gradle.kts` that wants to pull Lynx directly |
| **SwiftPM xcframework zips** (`{Lynx,LynxBase,LynxServiceAPI,PrimJS}-<ver>.xcframework.zip` + `.sha256`) | GitHub Releases | `whisker-cng`-generated `Package.swift` uses `binaryTarget(url:, checksum:)` to pull these |

### Maven consumer setup

```kotlin
// build.gradle.kts
repositories {
    maven { url = uri("https://whiskerrs.github.io/lynx/maven") }
}

dependencies {
    implementation("rs.whisker:lynx-android:3.8.0-whisker.4")
    implementation("rs.whisker:lynx-base-android:3.8.0-whisker.4")
    implementation("rs.whisker:lynx-trace-android:3.8.0-whisker.4")
    implementation("rs.whisker:lynx-service-api-android:3.8.0-whisker.4")
}
```

GitHub Pages must be enabled on the `gh-pages` branch (one-time
repo settings; the `peaceiris/actions-gh-pages@v3` step uses
`GITHUB_TOKEN` to push, no extra secrets).

### SwiftPM consumer setup

The release attaches a `swiftpm-manifest-<ver>.txt` that lists
each framework with its SwiftPM checksum:

```
Lynx=abc123…
LynxBase=def456…
LynxServiceAPI=ghi789…
PrimJS=jkl012…
```

Paste into the consumer `Package.swift`:

```swift
.binaryTarget(
    name: "Lynx",
    url: "https://github.com/whiskerrs/lynx/releases/download/v3.8.0-whisker.4/Lynx-3.8.0-whisker.4.xcframework.zip",
    checksum: "abc123…"
),
// … repeat for LynxBase / LynxServiceAPI / PrimJS
```

`whisker-cng`'s `Package.swift` template reads `swiftpm-manifest`
once per fork version bump and bakes those values into the
generated consumer-side `Package.swift`.

## Cutting a release

```bash
git checkout whisker/main
git tag v3.7.0-whisker.1
git push origin v3.7.0-whisker.1
```

The `build-whisker-tarballs.yml` workflow runs automatically on the
tag push (Android + iOS jobs in parallel). Wait for it to complete,
then check the resulting release page for the tarballs.

## Upstream relationship

Since v3.7.0-whisker.1 there are **no source patches** against
upstream — only the additive `core/native_renderer_capi/` subtree.
That makes a future PR to lynx-family/lynx straightforward: the
diff is a new file, no behavior change for upstream consumers.
