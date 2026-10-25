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

The native implementation links against the C++ view-layer symbols exported by
`@lynx/serval_markdown`. Harmony arm64 and x86_64 builds package the
implementation in `liblynx_xelement_markdown.so`, which links to the
package-provided `libserval_markdown.so` and lynxtextra native library. The core
`@lynx/lynx` package does not depend on these markdown packages directly. This
path does not use the internal `liblynx_markdown.so`.

## Platform gaps

The first native bridge covers registration, basic content/style/config props,
native measuring, native drawing, selection toggling, and a small set of UI
methods. Remaining platform work includes Lynx resource loading for markdown
images, event/exposure forwarding, inline/replacement Lynx child views, and full
method coverage.
