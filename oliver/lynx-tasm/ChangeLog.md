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
