# CHANGELOG

## 3.4.8
- Introduce `disabled` for input and textarea.

## 3.4.6
- fix `TextCodecHelper` defines.
- Update `fetch` defines for chunk streaming.

## 3.4.5
- Complete the documentation for image related APIs.

## 3.4.4
- Complete the documentation for text related APIs.

## 3.4.3
- Add `font-variation-settings`,`font-feature-settings` and `font-optical-sizing` CSS properties.
- Add `experimental-recycle-available-item-before-layout` and `enable-dynamic-span-count` property for list.

## 3.4.2
- And `lynx.fetchBundle` api defines.
- And `ResponseHandler` defines.

## 3.4.1

- Add the following properties: `custom-context-menu`, `custom-text-selection`, `text-selection`.
- Add the following methods: `setTextSelection`, `getTextBoundingRect`, `getSelectedText`.
- Add `bindselectionchange` event.
- Add `addFont` method.

## 3.4.0

### Major Changes

- Introduce <input> and <textarea>

## 3.3.2
- SelectorQuery FieldsParams adds query attribute
- Revert automatically generated cssTypes
- Add typing for the runtime interfaces.
- Codegen longhand and shorthand properties from css_defines
- Add `experimental-recycle-sticky-item` and `sticky-buffer-count` for list
- Add `experimental-update-sticky-for-diff` for list
- Add ReloadBundleEntry to standardize the timing of reload behavior.
- Add typing for MessageEvent.

## 3.3.1

- Codegen csstype.d.ts from css_defines
- Rename `visibleCellsAfterUpdate` to `visibleItemAfterUpdate` for `list`
- Rename `visibleCellsBeforeUpdate` to `visibleItemBeforeUpdate` for `list`

## 3.3.0

### Major Changes

- Update Types Version to 3.3.*
- Add default properties for PipelineEntry.frameworkRenderingTiming
- Add some missing typing of event props
- Add type testing for objects & methods mounted in global
- Add type testing to lynx react.JSX.IntrinsicElements
- Add some missing types of built-in element `list`
- Add some missing types of built-in element `image`
- Add more events like `LayoutChangeEvent` into `MainThread` namespace
- Add animate operate function in selectorQuery

In this commit, we add `AnimationEvent`, `TransitionEvent`, `LayoutChangeEvent`, `UIAppearanceEvent` into `MainThread` namespace.
Now you can use like this:

```
function handleLayoutChange(e: MainThread.LayoutChangeEvent) {
  // ...
}

<view
  main-thread:bindlayoutchange={handleLayoutChange}
/>
```

## 3.2.1

### Patch Changes

- Add some missing types of built-in element `list-item`
- Rename PipelineEntry.FrameworkPipelineTiming to PipelineEntry.FrameworkRenderingTiming
- Supplement the missing `lynx.onError` definition
- partially support TextEncoder/TextDecoder
- Support needVisibleItemInfo for native List
- Add prop 'ios-background-shape-layer' for iOS
- lynx.requireModule support setting timeout time

## 3.2.0

### Major Changes

- Rename @lynx-dev/types to @lynx-js/types

## 1.0.15

### Patch Changes

- Refine the related type of event

## 1.0.14

### Patch Changes

- Support nestedScrollOptions for hm

## 1.0.10

### Patch Changes

- Support lynx.queueMicrotask

## 1.0.5

### Patch Changes

- Format the error code of dynamic component with new ErrorCodeFormat

## 1.0.4

### Patch Changes

- Export common scrollEvent in type-lynx.

## 1.0.3

### Patch Changes

- Add new Trace API `lynx.performance.isProfileRecording` to minimizes the performance overhead.

## 1.0.2

### Patch Changes

- Support `inline-truncation` element.

## 1.0.1

### Major Changes

- Add all lynx public api on this packages.
