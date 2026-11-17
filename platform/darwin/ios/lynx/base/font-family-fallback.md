# Font-family fallback on iOS

Enable `enableTextFontFamilyCascade` in the page config to use explicit font
fallback lists on iOS. This option defaults to `false`; pages that omit it keep
first-available-font selection and do not request later font resources.

```json
{ "enableTextFontFamilyCascade": true }
```

For an enabled page, an ordered `font-family` list selects the first available
font as the primary font and attaches subsequent available fonts to its CoreText
cascade list. For example:

```css
.simplified-chinese-content {
  font-family: "BrandFont", "PingFang SC";
}
```

Characters covered by `BrandFont` keep that font. Missing characters can use
PingFang SC, which is supplied by iOS and requires no font download. Choose the
fallback for the content language: a Simplified Chinese fallback is not
appropriate for every Japanese or Traditional Chinese text run.

Candidates may be system fonts, registered aliases, or `@font-face` families.
Unavailable and empty candidates are skipped. Each candidate uses the requested
size, weight, and style. Explicit candidates precede any cascade already
attached to the primary font; system missing-glyph fallback remains available.
The result is preserved when applying font feature or variation settings.

Custom fallback fonts may load asynchronously even when the primary font is
already available. Their existing font-face observer requests another layout
after loading, allowing the cascade to be rebuilt. Resolved combinations are
cached by their actual font objects, so a pending fallback cannot permanently
cache an incomplete list.

This behavior applies to the iOS UIFont/CoreText path. It does not add a CSS
property named `cascadeList`, and does not change Android or other renderers.

Explicit CSS lists work independently of `enableTextFontCascadeOpt`. That setting
continues to control only the existing locale-selected default cascade and its
prewarming. An explicit CSS cascade takes precedence; a single-family value
retains the existing optimization behavior.

Multi-family lists in enabled pages participate in missing-glyph selection.
Later custom font resources can therefore be requested even when the primary is
available; single-family values do not gain additional resource requests. The
page setting is stored in the page's font-face context, so enabling one page
does not change other pages that share the font manager.

The independent `enable_text_font_family_cascade` external environment setting
(default: `true`) can disable explicit CSS lists even for pages that opted in.
Set it to `false` or `0` to restore first-available-font selection, skip later
font resource requests, and bypass the explicit cascade cache. It cannot enable
pages that did not opt in. This does not disable the separate locale-selected
optimization. Configure it through the host's existing external environment
provider before the first font resolution in an enabled page. Like other cached
LynxEnv switches, it is read once per process; applying a changed value requires
restarting the app.
