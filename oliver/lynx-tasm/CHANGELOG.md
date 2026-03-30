# CHANGELOG
# 0.0.34
* optimize lepus bytecode phase 2;

# 0.0.33
* fix encode JsBytecode issue.

# 0.0.32
* Support encoding and decoding template attribute arrays, slot indices, and object-form template entries.

# 0.0.31
* add lepus IR optimize module to optimize lepus bytecode;

# 0.0.30
* enlarge wasm MAXIMUM_MEMORY to 4G;

# 0.0.29
* Introducing cli command for `@lynx-js/tasm`;

# 0.0.28
* Support parsing comma-separated keyframe key lists in CSS keyframes (e.g., "0%,50%,100%" or "from, to"), which are flattened into multiple keyframe entries sharing the same style properties.

# 0.0.27
* Update supported lynx version to 3.7

# 0.0.26
* fix `Module.__malloc` not found issue. #5150

# 0.0.24
* Optimize the encoding process of CSS, unsupported CSS properties will not stop the encoding process.

# 0.0.23
* Support render functions for framework to load lazy CSS with `__LoadStyleSheet` and `__AdoptStyleSheet`.

# 0.0.22
* Change `path.resolve` to `require.resolve` for webpack pre-bundle

# 0.0.21
* supports to resolve `compilerOption` in a tasm file.

# 0.0.20
* support LEPUS_Eval with filename for sourcemap.

# 0.0.19
* add renderer functions for exposure control `stopExposure` and `resumeExposure`.

# 0.0.18
* fix encode error with `experimental_enableQuickjsByteCode`.

# 0.0.16
* Using `macOS-latest` runner to publish tasm artifact.

# 0.0.14
* add renderer functions for event handle `__AddEventListener`, `__RemoveEventListener`, `__CreateEvent`, `__DispatchEvent`, `__StopPropagation` and `__StopImmediatePropagation`.

# 0.0.13
* Support encode keyframe information in style objects into template for simple styling.

# 0.0.12
* Fix the issue that `StyleObjects` are not encoded as parsed `CSSValues` when `enableParallelElement` is false.

# 0.0.11
* add renderer functions for simple styling mode `__CreateStyleObject`, `__SetStyleObject` and `__UpdateStyleObject`.

# 0.0.10
* remove `FeatureCount` from template_codec module.

# 0.0.9
* add 'enableSimpleStyling' to compileOptions. And support encode style objects into template for simplified styling mode.

# 0.0.8
* support new css properties: `offset-path`, `offset-distance`, `offset-rotate`.

# 0.0.7
* support `decode_wasm` to resolve a tasm file.

# 0.0.6
* support `decode_napi` to resolve a tasm file.

# 0.0.5
* Update supported lynx version to 3.2

# 0.0.4
* Update supported lynx version to 3.0

# 0.0.3
* support compile `@lynx-js/tasm` as wasm binary.
