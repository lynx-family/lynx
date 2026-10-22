# Harmony XElement Notes

## markdown

Harmony `markdown` should follow the native XElement integration path:

- `markdown` is registered from the native XElement registry. Keep
  `<markdown>` as the only public tag under `lynx/`; do not add compatibility
  aliases for internal markdown tags.
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

`@lynx/serval_markdown` 0.3.3 exposes the C++ integration symbols used by the native
XElement, including `NativeMarkdownMeasurer` and `NativeServalMarkdownView`.
The shadow node owns the measurer and passes it with the measured content size
in physical pixels through its extra bundle. The UI creates
`NativeServalMarkdownView(false)` and
attaches the measurer with `SetMeasurer()`. Keep ArkUI view creation on the UI
thread and resource-triggered remeasurement on the layout thread. Enable
external measurement and disable the view's internal VSync. Drive
`OnLayoutFrame()` on the layout thread and `OnRendererFrame()` on the UI thread
through Lynx's Harmony VSync service. ArkUI must only consume the measured-size
snapshot, without reentering document measurement or alignment.
