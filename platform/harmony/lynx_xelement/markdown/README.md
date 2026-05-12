Lynx XElement Markdown
====

Lynx XElement Markdown for the Harmony platform.

## Overview

Lynx XElement Markdown provides `<markdown>` on Harmony by registering a native
C++ XElement. Registration happens from the `liblynx.so` native XElement
registry; layout and rendering stay on the C UI/native side.

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

`initialize()` is kept as a compatibility no-op for callers that already import
this package. The native registration is performed by `@lynx/lynx` module
initialization.

## Native dependency

The native implementation links against the C++ view-layer symbols exported by
`@lynx/serval_markdown`. Harmony arm64 and x86_64 builds register `<markdown>`
inside `liblynx.so` and link to the package-provided
`libserval_markdown.so`; there is no separate `liblynx_markdown.so` in this
path. The build guard matches the native ABIs currently shipped by
`@lynx/serval_markdown` and `@lynx/lynxtextra`.

## Platform gaps

The first native bridge covers registration, basic content/style/config props,
native measuring, native drawing, selection toggling, and a small set of UI
methods. Remaining platform work includes Lynx resource loading for markdown
images, event/exposure forwarding, inline/replacement Lynx child views, and full
method coverage.
