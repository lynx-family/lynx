# Host gperf Helpers

This directory provides the shared build integration for generated lookup
tables that use the repository-managed host gperf executable.

`gperf.gni` declares the `gperf_executable` override and discovers the default
host tool under `buildtools/gperf`. `gperf.py` runs that executable, normalizes
output from the supported gperf versions for C++17, and writes generated files
only when their contents change.

Generators remain responsible for their own input format, declarations, and
output-specific post-processing. Current users include the CSS keyword
generator and the Clay component keyword generator.

The executable is a build dependency only. It is never linked into or shipped
with Lynx artifacts.
