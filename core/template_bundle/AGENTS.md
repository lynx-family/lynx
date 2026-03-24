# AGENTS.md

## Scope

This directory contains Lynx template bundle wrappers and the binary template codec stack used to encode, decode, and version template bundles.

## Module Map

- `lynx_template_bundle.*`: top-level bundle wrapper.
- `lynx_template_bundle_converter.*`: bundle conversion helpers.
- `template_codec/`: binary encoding/decoding, versioning, constants, magic numbers, and related codec helpers.

## Key Files And Types

- `lynx_template_bundle.*`: bundle object surface consumed by other engine layers.
- `template_codec/magic_number.*` and `template_codec/version.h`: wire-format identity and compatibility markers.
- `template_codec/template_binary.h`, `compile_options.h`, `moulds.h`: codec-level shared definitions.
- `template_codec/lepus_cmd.*`: codec pieces coupled to Lepus command representation.

## Edit Rules

- Keep bundle wrappers separate from codec internals; not every bundle change should become a wire-format change.
- When changing binary format behavior, think about versioning and backward compatibility, not just the local encoder or decoder.
- Codec changes often affect renderer, runtime, and testing tools together. Watch for cross-module assumptions about constants and version fields.

## Common Regression Symptoms

- Bundles decode on one side but fail on another when format changes are only partially wired.
- Style or CSS data serializes successfully but reads back with missing fields when encoder/decoder expectations drift.
- Lepus command or template metadata regressions often point to template codec helpers rather than renderer logic.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and use the nearest codec target.

Common starting points:

- `binary_decoder_unittest_exec`
- `css_encoder_test_exec`
- `style_object_encoder_testset_exec`

If you changed Lepus command encoding or bundle-to-runtime contracts, consider the nearest Lepus/runtime tests as follow-up validation.
