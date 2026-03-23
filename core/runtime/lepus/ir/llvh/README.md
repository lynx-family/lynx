# LLVH (Lepus LLVM Helpers)

This directory vendors a small, self-contained subset of LLVM support utilities
used by Lepus IR.

## Build (GN)

- Library target: `//lynx/core/runtime/lepus/ir/llvh:llvh`
- Headers-only target (generated config + include dirs):
  `//lynx/core/runtime/lepus/ir/llvh:llvh_headers`

Generated config headers are emitted to:

- `$root_gen_dir/llvh/include/llvh/Config/llvh-config.h`
- `$root_gen_dir/llvh/include/llvh/Config/llvm-config.h`

Consumers should include LLVH headers via the repository path prefix, e.g.
`core/runtime/lepus/ir/llvh/include/llvh/...`, to match Lynx's include policy.

## Notes

- This is not a full LLVM checkout; only the subset required by Lepus is kept.
- CMake files are intentionally not used in Lynx GN builds.
- Use clang-format to format the code, and rename the include guard
