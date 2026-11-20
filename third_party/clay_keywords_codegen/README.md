# Clay Keywords Code Generator

This directory contains the host tool that generates Clay's keyword lookup
table from `clay/ui/component/keywords.in`.

The generator uses a deterministic two-level perfect hash table. It is built
with GN's host toolchain, so cross-compiles for Android, iOS, and Harmony run a
Linux or macOS executable rather than a target-platform executable. Native
Windows and macOS builds compile the same source with their local host
toolchains.

Android's GN-to-CMake export runs `run.py`, which compiles the same source with
the repository host compiler because CMake does not materialize GN
cross-toolchain executable targets. CocoaPods source builds use the same
wrapper from the generated `Clay.podspec`. On Apple hosts, the wrapper selects
the system C++ compiler through `xcrun --sdk macosx` so an inherited iOS or
simulator `SDKROOT` cannot contaminate the host-tool compilation.

The generated files are written under `$root_gen_dir/clay/ui/component` and
are not checked into the source tree.

The generator source, wrapper, and GN target are included in the open-source
repository. They use only the C++ standard library, Python's standard library,
and the repository-provided Clang toolchain. All files in this directory use
the project's Apache License 2.0 header; no GNU gperf code or other GPL
dependency is included.
