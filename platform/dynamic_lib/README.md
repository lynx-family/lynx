# Lynx Dynamic Library

This directory owns the `libLynx_clay` dynamic library distribution targets.
It keeps the platform-specific macOS/Linux packaging logic and the small C++
wrapper layer together so external language bindings can move independently
from this repository.

## Layout

- `BUILD.gn` builds the shared pure C ABI wrapper objects.
- `lynx_rust_capi.cc` exports `lynx_rust_*` wrapper symbols for existing C++
  CAPI functions that are not directly Rust-FFI friendly.
- `macos/` builds and packages `libLynx_clay.dylib`.
- `linux/` builds and packages `libLynx_clay.so`.

The dynamic library exports both the existing `lynx_*` C APIs and the
`lynx_rust_*` wrapper symbols from the same image. Consumers should not need a
second shim library.

## Build

macOS:

```sh
gn gen out/Release --root-target=//platform/dynamic_lib/macos:package_sdk
ninja -C out/Release platform/dynamic_lib/macos:package_sdk
```

Linux:

```sh
gn gen out/Release --root-target=//platform/dynamic_lib/linux:package_sdk
ninja -C out/Release platform/dynamic_lib/linux:package_sdk
```

The package actions write:

- `lynx_clay_sdk_macos_${target_cpu}.zip`
- `lynx_clay_sdk_linux_${target_cpu}.zip`

Each SDK package contains `lib/libLynx_clay.{dylib,so}`, public C API headers,
and `data/icudtl.dat`. The `lynx_rust_*` wrapper symbols are exported from the
library image, but this package does not ship a separate wrapper header; Rust
bindings should keep their ABI declarations in the Rust-side repository. macOS
packages also include Lynx resource bundles. Linux packages include optional
resource bundles and `lynx_core.js` files when they are present in the build
directory.

## Runtime Loading

External bindings should prefer an explicit library path:

```sh
export LYNX_LIB_PATH=/path/to/libLynx_clay.dylib
export LYNX_LIB_PATH=/path/to/libLynx_clay.so
```

SDK-directory loading should look for:

- `$LYNX_SDK_DIR/lib/libLynx_clay.dylib` on macOS
- `$LYNX_SDK_DIR/lib/libLynx_clay.so` on Linux

Local GN build directories are also usable when `libLynx_clay.{dylib,so}` sits
directly under the build directory. Consumers that need ICU should also point
Lynx at the packaged `data/icudtl.dat`.

## Signing

Linux shared objects do not require OS-level code signing. Release pipelines
should publish checksums and downstream products should apply their normal
supply-chain verification policy.

macOS development builds can be ad-hoc signed for local runtime loading:

```sh
codesign --force --sign - libLynx_clay.dylib
```

Apps or command-line tools that redistribute `libLynx_clay.dylib` should sign
and notarize the final product with their own Apple Team ID.

## GitHub Release

The `publish-release` workflow builds and uploads these dynamic library SDK
packages to GitHub Releases:

- `lynx_clay_sdk_macos_arm64.zip`
- `lynx_clay_sdk_linux_x64.zip`

Each package is uploaded with a matching `.sha256` checksum file.
The workflow invokes `platform/dynamic_lib/tools/build_release.py` after the
standard `common-deps` habitat setup.
