# Clay Keyword Code Generation

## Scope

This specification defines how Clay generates the keyword lookup API declared
by `clay/ui/component/keywords.h`.

The source of truth is `clay/ui/component/keywords.in`. Generated C++ files are
build outputs and must not be checked into the source tree.

## Generated API

For every non-empty, non-comment line in `keywords.in`, in source order, the
generator must:

1. emit one `KeywordID` enumerator;
2. preserve the numeric value that follows from that source order;
3. emit a lookup entry that maps the exact keyword string to that enumerator.

`GetKeywordID` must return `KeywordID::kInvalid` for empty strings, unknown
strings, and strings that differ only by unsupported spelling or case changes.

The enumerator spelling must remain compatible with the historical Python
generator: each hyphen-separated word is capitalized and the remaining
characters in that word are lowercased.

## Build Contract

GN must generate:

- `$root_gen_dir/clay/ui/component/keywords.h`
- `$root_gen_dir/clay/ui/component/keywords.cc`

The Clay UI target must compile those generated files instead of source-tree
snapshots.

The generated header and implementation must be owned by one Clay-internal
source target. Other targets that directly consume the keyword API must depend
on that source target rather than the generation action, so generation details
do not leak across platform BUILD files.

The GN configuration-stage generators must:

- declare `keywords.in` as an input;
- declare both generated files as outputs;
- rerun when the input or generator changes;
- fail before compilation if the input is empty, contains duplicate keywords,
  or cannot produce a collision-free table;
- produce deterministic output for the same ordered input.

## Host And Target Toolchains

The header is rendered directly from enum values parsed out of `keywords.in`.
The perfect-hash implementation generator is compiled and executed by the same
configuration-stage wrapper. Both outputs must exist when GN generation
finishes, including for CocoaPods and GN-to-CMake exporters.

The configuration-stage wrapper compiles the generator source with the
repository host compiler before executing it. This remains a host-only step
and must not use a target-platform compiler.

CocoaPods source builds do not execute GN actions. GN must therefore invoke the
same checked-in wrapper and generator source while producing `Clay.podspec`,
writing both outputs under `out/gn_to_podspec/gen/clay/ui/component` before
the source package is assembled. Installing the published pod must not run the
generator. The `Native` subspec target retains the stable
`generate_keywords` dependency label so platform BUILD files keep one
ownership edge for generated sources; this dependency is build-graph metadata
and is not emitted as a consumer-side Pod prepare command.

| Target | Generator host |
| --- | --- |
| Android | Linux or macOS build host |
| iOS | macOS build host |
| Harmony | Linux or macOS build host |
| Linux | Linux build host |
| macOS | macOS build host |
| Windows | Windows build host |

Target-platform binaries must never be executed as part of keyword generation.
The generated C++ files remain platform-independent and are compiled by the
selected target toolchain.

The generator must remain self-contained in the open-source repository. It may
use the C++ standard library, Python's standard library, and the
repository-provided Clang toolchain, but must not add a runtime dependency or
copy code from an incompatible license.

## Hash Table Contract

The generated implementation uses a deterministic two-level perfect hash
table and a contiguous string pool with 16-bit offsets. Every declared keyword
must resolve without collisions. A candidate slot must still compare the
original string before returning a `KeywordID`, so unknown inputs that share a
hash slot return `kInvalid`.

Changes to the hash construction may change generated source layout and binary
size, but must not change the generated API, enumerator values, or lookup
behavior.
