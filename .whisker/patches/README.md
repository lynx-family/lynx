# Lynx Android patches

Local patches against the Lynx 3.7.0 source tree required to produce
AARs that Whisker's Rust C++ bridge can link against.

## Why these aren't upstreamed

Lynx is designed around the workflow "ship JS templates, consume them
through the Java/Kotlin `LynxView` API." Its C++ side (`LynxShell`,
`ElementManager`, `FiberElement::*`, the Element PAPI) is treated as
internal implementation, and the public AARs are built with
`-fvisibility=hidden` + `-Wl,--exclude-libs,ALL` + `-static-libstdc++`
to:

- shrink the `.dynsym` (and so the .so),
- speed up RELRO / symbol resolution at load,
- keep the C++ ABI surface free to refactor,
- prevent accidental binary dependencies on internal API.

Whisker's design — Rust drives the element tree directly via the Element
PAPI — runs against every one of those goals on purpose. Upstreaming a
flip of the defaults is a non-starter; doing it behind a `gn` arg the
project has no use for would be hard to motivate. So we carry the
deltas locally.

If/when Whisker needs to ride a newer Lynx, re-record these patches with
`git diff > patches/lynx-android/<name>.patch` after porting the same
edits forward — the script below verifies they still apply cleanly.

## What each patch does

| Patch | Target repo | Change |
|---|---|---|
| `buildroot.patch` | `lynx-family/buildroot` (the `build/` submodule of Lynx) | Default `disable_visibility_hidden = true`; drop `-static-libstdc++`; drop `-Wl,--exclude-libs,ALL` from the global linker config. |
| `lynx.patch` | `lynx-family/lynx` | In `lynx_android_public_config`: `-fvisibility=hidden` → `-fvisibility=default`. In `lynx_android_private_config`: drop `-Wl,--exclude-libs,ALL`. |

Pinned commits we tested against:

- `lynx-family/lynx`       `248765e76fb0f889efd0b168b8b892819c1c17e4`
- `lynx-family/buildroot`  `917b38180c78da016b1023436d5b568ca5402bee`

## How to use

```sh
# 1. Bootstrap Lynx per its docs (source tools/envsetup.sh, tools/hab sync, etc.)
#    Default location:  target/lynx-src/  (override via LYNX_SRC env var)
# 2. Patch + build the AARs into target/lynx-android/
cargo xtask android build-lynx-aar
# 3. Unpack into target/lynx-android-unpacked/jni/<abi>/
cargo xtask android unpack-lynx
# 4. Build the example end-to-end (cargo + jniLibs + gradle assembleDebug)
cargo xtask android build-example
```

`build-lynx-aar` is idempotent: re-runs detect already-applied
patches and skip them. `build-example` will trigger `unpack-lynx`
automatically if the AAR contents haven't been extracted yet.
