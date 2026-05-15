# Whisker fork bits

This subtree is everything the Whisker fork adds on top of upstream
Lynx. Keep additions here narrow — patches live under `.whisker/patches/`
as files that CI applies at build time, and the build glue stays
under `.whisker/ios/` (for the iOS CocoaPods carrier project) once
that lands.

## Structure

```
.whisker/
├── patches/
│   ├── buildroot.patch    — applied to build/ after `tools/hab sync`
│   │                        in the build-whisker-tarballs workflow
│   └── README.md          — what each patch does + why upstreaming is hard
└── README.md              — this file
```

## Companion repo: whiskerrs/whisker

The Whisker repo at `whiskerrs/whisker` pins this fork's release
tag in `crates/whisker-build/src/lynx.rs`. After cutting a release
here:

1. Get the SHA-256 of each tarball from the release page (or the
   `.sha256` sidecar files attached to the release).
2. Update `LYNX_FORK_TAG`, `LYNX_VERSION`, `LYNX_ANDROID_SHA256`,
   and `LYNX_IOS_SHA256` in `crates/whisker-build/src/lynx.rs`.
3. Commit + push the whisker repo. From the next `whisker run` /
   `whisker build` invocation, the new release tarball is
   downloaded + verified + unpacked under
   `~/.cache/whisker/lynx/<version>/`.

## Cutting a release

```bash
# From this repo
git checkout whisker/main
git tag v3.7.0-whisker.0
git push origin v3.7.0-whisker.0
```

The `build-whisker-tarballs.yml` workflow runs automatically on
the tag push. Wait for it to complete, then check the resulting
release page for the tarballs.

## Direct-from-source iOS build (not yet)

The iOS Lynx framework build is currently driven from
`whiskerrs/whisker`'s `xtask/src/ios/build_lynx_frameworks.rs`,
which sets up a CocoaPods carrier project, pod-installs Lynx +
PrimJS source pods, and `xcodebuild`s xcframeworks. That logic
needs to be ported into `.whisker/ios/` so this fork's CI can
build iOS too. Until then the iOS half of the
`build-whisker-tarballs` workflow is commented out, and Whisker
users on iOS still need a Whisker contributor's local build (set
via `WHISKER_LYNX_DIR` env var).
