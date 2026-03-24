# AGENTS.md

## Scope

This directory contains value-conversion wrappers between Lynx public values and runtime/platform representations such as Lepus values, Piper values, NAPI values, and platform-specific wrapper types.

## Module Map

- `value_impl_lepus.*`: Lepus-backed value wrapper.
- `value_impl_piper.h`: Piper-facing value surface.
- `value_wrapper_utils.*`: shared conversion helpers.
- `napi/`: PrimJS/NAPI wrapper helpers and NAPI-backed value implementations.
- `android/`, `darwin/`, `harmony/`: platform-specific value-wrapper implementations.

## Edit Rules

- Keep shared conversion semantics in common files and platform-specific or backend-specific behavior in the corresponding subdirectory.
- Be careful with ownership, string/binary conversion, and null/opaque-value handling; many callers assume wrappers preserve semantics across language boundaries.
- If a change affects public value contracts, also inspect the shared public value contract in `../public/pub_value.h` and the owning runtime binding code.

## Common Regression Symptoms

- Values appear correct in one runtime/backend but not another when wrapper conversions drift.
- Callbacks or native-module parameters become `undefined`, null, or opaque unexpectedly after NAPI wrapper changes.
- Platform-only regressions often point to `android/`, `darwin/`, or `harmony/` wrapper files rather than shared logic.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and start with:

- `value_wrapper_unittest_exec`

If you changed NAPI-related conversions, also consider the nearest runtime NAPI tests in `../runtime/common/napi/`.
