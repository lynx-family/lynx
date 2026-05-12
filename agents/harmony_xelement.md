# Harmony XElement Notes

## markdown

Harmony `markdown` should follow the native XElement integration path:

- `markdown` is registered from the native XElement registry.
- Layout and rendering are implemented by C++ `UIBase` and `ShadowNode`
  classes through C UI/ArkUI native nodes.
- For Harmony arm64 and x86_64 builds, Lynx links against the C++ symbols
  exported by the `@lynx/serval_markdown` OHPM package.
- This path does not build or load `liblynx_markdown.so`.

Do not implement `markdown` as an ArkTS `UIBase` wrapper around
`@lynx/serval_markdown`. Serval markdown can provide the markdown parsing,
layout, and drawing core, but the Lynx XElement integration should stay native.

`@lynx/serval_markdown` must expose the C++ view-layer symbols used by the native
XElement, including `NativeServalMarkdownView` and `MarkdownView`. Shadow layout
should measure through `MarkdownView::Measure()` rather than depending on
internal serval measurer classes.
