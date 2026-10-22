Lynx XElement Markdown
====

Lynx XElement Markdown for the Harmony platform.

## Overview

Lynx XElement Markdown provides `<markdown>` on Harmony by registering a native
C++ XElement. Registration resolves the implementation from
`liblynx_xelement_markdown.so`; layout and rendering stay on the C UI/native
side.

## Installation

```bash
ohpm install @lynx/xelement_markdown
```

## How to use

Add the dependency in `oh-package.json5`:

```json5
{
  "dependencies": {
    "@lynx/xelement_markdown": "0.0.1-alpha.1",
  }
}
```

Then initialize it before creating Lynx views:

```ts
import { XElementMarkdown } from '@lynx/xelement_markdown';

XElementMarkdown.initialize();
```

`initialize()` loads the package native module and registers the markdown
creators. Call it once before creating Lynx views that may use markdown.

## Native dependency

The native implementation links against the C++ symbols exported by
`@lynx/serval_markdown` 0.3.3 and uses `@lynx/lynxtextra` 0.2.0. The shadow node
owns a `NativeMarkdownMeasurer` and passes it with the measured content size in
physical pixels to the UI. The UI creates a `NativeServalMarkdownView(false)`,
disables its internal VSync, enables external measurement and attaches the
measurer. ArkUI consumes the supplied size without measuring or aligning the
document again. Harmony arm64 and x86_64 builds package the
implementation in `liblynx_xelement_markdown.so`, which links to the
package-provided `libserval_markdown.so` and lynxtextra native library. The core
`@lynx/lynx` package does not depend on these markdown packages directly. This
path does not use the internal `liblynx_markdown.so`.

Lynx drives layout frames on the layout thread and renderer frames on the UI
thread. Image completion notifications also return to the layout thread through
`NativeMarkdownMeasurer::OnImageLoaded()`. Pending frame callbacks stop when the
shadow node is destroyed or its UI view is detached.

`animation-frame-rate` defaults to 15, matching Android and iOS. The
`pauseAnimation` UI method freezes the displayed frame and returns its
`animationStep`; `resumeAnimation` accepts an optional step and resumes the
layout animation clock. Layout timestamps continue while paused.

## Platform gaps

The native bridge covers content/style/config props, measurement, drawing,
canvas images, inline Lynx child views, selection toggling and animation control.
Remaining platform work includes custom fonts, replacement views,
event/exposure forwarding and the remaining UI methods.
