# CHANGELOG

## 4.2.0

### Minor Changes

- Publish the LynxSDK 4.2 package line and add the backfilled `Config.enableLayoutOnlyEventThrough` type and runtime `configKeys` entry.

## 4.1.3

### Patch Changes

- Add `alignMouseEventWithW3C` to the exported `Config` type and `configKeys` runtime constant.

## 4.1.2

### Patch Changes

- Add `enableEventHandleRefactor` to the exported `Config` type and `configKeys` runtime constant.

## 4.1.1

### Patch Changes

- Add `enableFixedNew`, `enableParseIntFlex`, and `enableReloadLifecycle` to the exported `Config` type and `configKeys` runtime constant.

## 4.1.0

### Minor Changes

- Initial release of `@lynx-js/type-config` with exported Config type definitions and `configKeys` runtime constant.

- Include 28 exported `Config` entries covering layout, CSS, events, gestures, text, list, and accessibility configs, including `enableCSSInheritance`, `enableCSSInlineVariables`, `enableCSSRule`, `enableEventThrough`, `enableMultiTouch`, `enableGridPlacementShorthands`, `enableFlexBasisZeroPercent`, `enableTransformedTouchPosition`, `unifyVWVHBehavior`, `fontScaleEffectiveOnlyOnSp`, and more.

- Provide `configKeys` runtime constant for programmatic config key enumeration.
