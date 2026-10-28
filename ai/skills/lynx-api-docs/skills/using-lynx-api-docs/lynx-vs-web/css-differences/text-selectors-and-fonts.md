# Text, Selectors, and Fonts

## Text Handling

**Text must use a `<text>` element**:

```html
<!-- Web: text can appear directly inside a div. -->
<div>Hello World</div>

<!-- Lynx: use the text element. -->
<view>
  <text>Hello World</text>
</view>
```

**Text wrapping**:

```css
/* Web: text wraps by default. */
/* Lynx text: text does not wrap by default; opt in explicitly. */
text {
  white-space: normal; /* Allow wrapping. */
}
```

## Pseudo-elements

**`::before` and `::after` are not supported**:

```css
/* ❌ Not supported */
.element::before {
  content: 'Prefix';
}

.element::after {
  content: '';
  background-color: red;
}
```

> **Note**: Lynx does not support `::before` or `::after`. Although the CSS parser recognizes these pseudo-elements, the selector matcher and rendering engine do not implement them, so they have no effect.

## Selector Limitations

**Not supported**:

- ❌ Newer selectors such as `:is()`, `:where()`, and `:has()`
- ❌ Attribute selectors in stylesheets, including `[attr]` and `[attr="val"]`: parsed but not matched for CSS styling. Use explicit classes; attribute support in node queries does not imply CSS support.
- ❌ The general sibling combinator may be limited in complex cases

## Custom Fonts

**Font formats**:

```css
/* Web: supports multiple formats. */
@font-face {
  src: url('font.woff2') format('woff2');
}

/* Lynx: use system fonts or inline base64 data. */
font-family: 'PingFang SC', 'Helvetica Neue', Arial, sans-serif;
```
