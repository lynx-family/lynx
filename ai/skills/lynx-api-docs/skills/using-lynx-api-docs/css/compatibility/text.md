# Text CSS compatibility

## Line height and spacing

`line-height: normal` delegates to native font metrics and is not guaranteed to
match the Web line box for the same downloadable font. Use an explicit number
or length when cross-engine vertical geometry matters.

`word-spacing` is not a registered Lynx CSS property.

## Text decoration

The `text-decoration` shorthand recognizes `none`, `underline`,
`line-through`, the `solid`, `double`, `dotted`, `dashed`, and `wavy` styles,
colors, and a length thickness. It does not recognize `overline`.

`text-decoration-line` and `text-decoration-style` are not registered
longhands. `text-decoration-color` has a registered property ID, but its
independent stored value is not the color consumed by native decoration
painting in this package baseline. Put the line, style, and color in the
shorthand.

```css
.emphasis {
  text-decoration: underline dashed #008000;
}
```

`text-decoration-thickness` is implemented on Android and iOS, not Harmony.
Decoration geometry is native-backend-specific: nested spans and bidirectional
runs can fragment or drop ancestor decorations, and patterned lines are not
pixel-identical to Web rendering. Prefer one simple decoration on one `text`
element when consistent cross-platform rendering matters.

For CSS-text serialization limits affecting `text-shadow` and
`text-decoration`, see [Computed-style CSS text](./computed-style.md).
