# Differences Between Lynx and Web CSS

Lynx supports most commonly used CSS features, but there are several important differences. **Use this index before generating or editing Lynx CSS, not only during migration.** Read every topic that applies to the task; the short rules below point to the complete constraints and examples.

## Generation-Critical Routing

| Task, keyword, or failure | Direct rule | Complete reference |
| --- | --- | --- |
| Choose `display`; replace `block`, `inline`, `inline-block`, or `table*` | Prefer explicit Flex or Linear; `inline`, `inline-block`, and `table*` are unsupported | [Display](css-differences/display-and-default-layout.md#the-display-property) |
| Omit `display`; size nested or empty containers | Lynx defaults to Linear, not block flow; cross axis stretches by default, main axis does not | [Default layout](css-differences/display-and-default-layout.md#default-layout-behavior-important) |
| Flex text, numbers, or children shrink, wrap, or break onto multiple lines; Web and Lynx differ | Lynx does not automatically apply the Web's content-based minimum; use `min-width`, `min-height`, or `flex-shrink: 0` with enough main-axis space, plus `white-space: nowrap` when text must stay on one line | [Flex shrinking](css-differences/flex-and-sizing.md#flex-shrinking-and-content-based-minimum-sizes) |
| Use percentages below `auto`, `fit-content`, or `max-content` ancestors | Avoid content-sized ancestors in the percentage dependency chain; set definite key sizes or use Flex/Linear space allocation | [Cyclic percentages](css-differences/flex-and-sizing.md#cyclic-percentage-sizing) |
| Set `box-sizing`; migrate exact widths and heights | Initial `auto` normally uses border-box sizing but can depend on compatibility configuration; set `border-box` or `content-box` explicitly | [Box model](css-differences/box-model.md#box-model) |
| Space siblings or parent/child with margins | Margins do not collapse in Lynx; remove duplicate margins, use padding, or use `gap` in Flex/Grid | [Margin collapsing](css-differences/box-model.md#box-model) |
| Use negative margins or negative padding | Negative margins work in Linear layout; negative padding is invalid on the Web but accepted by Lynx, so replace it with `0` for Web-compatible behavior | [Box model details](css-differences/box-model.md#box-model) |
| Use `min-content`, `max-content`, `fit-content`, `ch`, `ex`, or physical units | `min-content`, `ch`, `ex`, and physical units are unsupported; check the `fit-content` absolute-positioning limitation | [Length values](css-differences/flex-and-sizing.md#length-values) |
| Use `calc()` | Supported for listed length properties only, not colors, enums, border radii, transforms, or numeric properties | [`calc()` scope](css-differences/flex-and-sizing.md#support-for-calc) |
| Use `position: static` or `fixed` | Use `relative` instead of `static`; on fixed elements, set a definite inset on each axis (`left` or `right`, and `top` or `bottom`) | [Positioning](css-differences/positioning-and-stacking.md#positioning) |
| Use `float` or `clear` | Unsupported; use Flexbox | [Floats](css-differences/positioning-and-stacking.md#floats-are-not-supported) |
| Add text or control wrapping | Wrap text in `<text>`; Lynx text does not wrap by default, so set `white-space: normal` when wrapping is required | [Text handling](css-differences/text-selectors-and-fonts.md#text-handling) |
| Use `::before`, `::after`, `:is()`, `:where()`, `:has()`, or attribute selectors | Unsupported for styling; use real elements and explicit classes instead | [Pseudo-elements](css-differences/text-selectors-and-fonts.md#pseudo-elements), [selectors](css-differences/text-selectors-and-fonts.md#selector-limitations) |
| Set `z-index` or debug stacking, especially inside scrolling content | An explicit `z-index`, including `0`, creates a stacking context without requiring `position`; add or remove ancestor contexts deliberately instead of adding `position` just for `z-index` | [`z-index`](css-differences/positioning-and-stacking.md#z-index) |
| Load a custom font | Use system fonts or inline base64 data rather than assuming Web font-format support | [Custom fonts](css-differences/text-selectors-and-fonts.md#custom-fonts) |
| Use `scale3d()` or `rotate3d()` | Unsupported; use `scale()`, `rotateX/Y/Z()`, or `matrix3d()` as applicable | [3D transforms](css-differences/transforms-and-animations.md#subset-of-3d-transform-functions) |
| Use `steps(n)` | Add an explicit step position, such as `steps(n, end)` | [`steps(...)`](css-differences/transforms-and-animations.md#the-steps-animation-timing-function) |
| Migrate Web `body` spacing or `html`/`body` backgrounds | Reproduce computed body spacing on `page`; use `page` for a full-page background and the root `view` for a content-container background | [Body spacing](css-differences/migration-pitfalls.md#pitfall-5-failing-to-copy-body-styles-to-the-page-element), [backgrounds](css-differences/migration-pitfalls.md#pitfall-6-mapping-web-html-and-body-backgrounds-to-ordinary-containers) |
| Migrate `background` containing `fixed` or `scroll` attachment | `background-attachment` is unsupported; expand supported longhands and handle attachment behavior separately | [Background shorthand](css-differences/migration-pitfalls.md#pitfall-7-migrating-a-background-shorthand-that-contains-background-attachment) |

## Full Migration Review

- [Quick migration checklist](css-differences/migration-pitfalls.md#quick-migration-checklist)
- [Common pitfalls with examples](css-differences/migration-pitfalls.md#common-pitfalls)
- [Unsupported features](./unsupported-features.md)
- [Complete migration guide](./migration-guide.md)
