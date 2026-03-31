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
- `template_codec/binary_decoder/lynx_config.yml`: source of truth for template-bundle config schema, including root-level `PageConfig` entries and `compilerOptions`.
- `template_codec/binary_decoder/lynx_config_decoder.h`: committed generated decoder entry for bundle config parsing.
- `../../tools/config/check_and_run.py` and `../../tools/config/gen_config.py`: generators for config-related C++, type-config, and optional doc outputs.

## Lynx Config

- Treat `template_codec/binary_decoder/lynx_config.yml` as the schema source of truth for bundle config and native config decoding behavior in this module.
- Root-level config entries feed `LynxConfig` / `PageConfig`; `readNative: true` also wires the key into native JSON parsing through generated code.
- `compilerOptions` affect `template_codec/compile_options.h` and related type-config outputs. Prefer not to add new compiler options unless the setting is truly compile-time.
- After changing `lynx_config.yml`, regenerate from the repo root with `python3 lynx/tools/config/check_and_run.py --all`, then review:
  - `template_codec/binary_decoder/lynx_config_decoder.h`
  - `template_codec/compile_options.h`
  - `lynx/js_libraries/type-config/`

## Edit Rules

- Keep bundle wrappers separate from codec internals; not every bundle change should become a wire-format change.
- When changing binary format behavior, think about versioning and backward compatibility, not just the local encoder or decoder.
- Codec changes often affect renderer, runtime, and testing tools together. Watch for cross-module assumptions about constants and version fields.
- For `lynx_config.yml`, preserve nearby ordering and schema patterns instead of reformatting the file wholesale.
- Be careful with advanced config fields such as `readSettings`, `bindMemberTo`, `nameAs`, `versionOverrides`, and partial `codeGen`; these can change generated code shape, not just metadata.

## Common Regression Symptoms

- Bundles decode on one side but fail on another when format changes are only partially wired.
- Style or CSS data serializes successfully but reads back with missing fields when encoder/decoder expectations drift.
- Lepus command or template metadata regressions often point to template codec helpers rather than renderer logic.
- A new `lynx_config.yml` key appears in YAML but not in generated code when `codeGen`, `bindMemberTo`, or value-type compatibility was chosen incorrectly.
- JS type-config exports drift from C++ config behavior when generated outputs are not refreshed after a schema change.

## Skill Index

- `ai/skills/lynx-config/SKILL.md`: use when adding, updating, or debugging `lynx_config.yml` entries and their generated outputs.
- `ai/skills/cpp-unittest/SKILL.md`: use when validating nearby codec or decoder unit tests after C++ behavior changes.

## Validate

For config-schema changes, regenerate with `python3 lynx/tools/config/check_and_run.py --all` first, then review the generated diffs before running tests.

For C++ unit tests here, prefer the `cpp-unittest` skill and use the nearest codec target.

Common starting points:

- `binary_decoder_unittest_exec`
- `css_encoder_test_exec`
- `style_object_encoder_testset_exec`

If you changed Lepus command encoding or bundle-to-runtime contracts, consider the nearest Lepus/runtime tests as follow-up validation.
