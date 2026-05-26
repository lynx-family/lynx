# Harmony XElement Notes

## markdown

Harmony `markdown` should follow the native XElement integration path:

- `markdown` is registered from the native XElement registry. Keep
  `<markdown>` as the public tag under `lynx/`; internal `x-markdown` belongs
  outside the open-source boundary.
- Layout and rendering are implemented by C++ `UIBase` and `ShadowNode`
  classes through C UI/ArkUI native nodes.
- The core `@lynx/lynx` package must not depend on
  `@lynx/serval_markdown` or `@lynx/lynxtextra`. The native registry should
  resolve markdown creators from the `@lynx/xelement_markdown` native library
  through `dlopen` / `dlsym`.
- `@lynx/xelement_markdown` must call its native `initMarkdown()` before Lynx
  views using markdown are created. This loads `liblynx_xelement_markdown.so`
  and registers the direct creators.
- For Harmony arm64 and x86_64 builds, `@lynx/xelement_markdown` links against
  the C++ symbols exported by the `@lynx/serval_markdown` OHPM package.
- This path does not build or load the internal `liblynx_markdown.so` package.

Do not implement `markdown` as an ArkTS `UIBase` wrapper around
`@lynx/serval_markdown`. Serval markdown can provide the markdown parsing,
layout, and drawing core, but the Lynx XElement integration should stay native.

`@lynx/serval_markdown` must expose the C++ view-layer symbols used by the native
XElement, including `NativeServalMarkdownView` and `MarkdownView`. Shadow layout
should measure through `MarkdownView::Measure()` rather than depending on
internal serval measurer classes.
