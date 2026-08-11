# Clay Keywords Code Generator

This directory contains the Clay-owned tools that generate the keyword lookup
API from `clay/ui/component/keywords.in`.

`generate_keywords.py` renders the generated header from
`keywords.h.template`, constructs a Clay-specific gperf input, and invokes the
shared helper in `tools/gperf/gperf.py`. The shared helper discovers the host
gperf installed by the repository dependency manifests, normalizes output from
supported gperf versions, and avoids rewriting unchanged files.

The generated files are written under `$root_gen_dir/clay/ui/component` and
are not checked into the source tree.

Generation always executes the build host's gperf. Android GN-to-CMake and iOS
GN-to-Podspec exports materialize the files while generating their build
metadata. Native GN/Ninja builds track the same generator action. The generated
files are compiled only by the selected target toolchain.
