# Lynx text Element

This guide covers the common `<text>` element usage for text rendering and inline text composition in Lynx.

## When to Use `<text>`

### Use `<text>` when:

- you need to render visible text content in Lynx
- you need text-specific styling such as font, color, line height, decoration, or ellipsis
- you need nested text spans with inherited text styling
- you need multiline truncation through `text-maxline`

### Do not use `<text>` when:

- the node is only a structural container and should stay on `<view>`
- you need image loading, placeholder, or media playback behavior
- you need multiline text entry; use `<textarea>`
- you need single-line text entry; use `<input>`

## Quick Start

### Basic text

```tsx
<text className="title">Hello Lynx</text>
```

```css
.title {
  color: #333333;
  font-size: 14px;
  font-weight: bold;
  line-height: 1.5;
  text-align: left;
}
```

### Nested text

```tsx
<text className="rich-text">
  <text className="highlight">Highlighted</text>
  <text> normal text</text>
</text>
```

## Common Styling

### Text appearance

```css
.text {
  color: #333333;
  font-size: 14px;
  font-weight: bold;
  font-family: 'PingFang SC', -apple-system, sans-serif;
  line-height: 1.5;
  text-align: left;
  text-decoration: underline;
  text-overflow: ellipsis;
}
```

### Single-line ellipsis

```css
.single-line {
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 200px;
}
```

### Multi-line truncation

```tsx
<text text-maxline={3}>This is a long paragraph that should truncate after three lines.</text>
```

## Common Props

| Prop | Type | What It Does |
| --- | --- | --- |
| `text` | `string` | Sets text content directly instead of using children |
| `text-maxline` | `number` | Limits how many lines are shown before truncation |
| `text-overflow` | `string` | Controls overflow rendering such as `clip` or `ellipsis` |

## Text Styling Surface

| Style | What It Does |
| --- | --- |
| `color` | Text color |
| `font-size` | Text size |
| `font-weight` | Text weight |
| `font-family` | Font family |
| `font-style` | Font style |
| `line-height` | Line height |
| `letter-spacing` | Character spacing |
| `text-indent` | First-line indentation |
| `text-align` | Horizontal alignment |
| `text-decoration` | Underline or line-through decoration |
| `white-space` | Line wrapping behavior |
| `text-overflow` | Overflow display behavior |
| `vertical-align` | Inline vertical alignment |
| `text-shadow` | Text shadow |

## Advanced Styling

### Text stroke

```css
.stroke {
  text-stroke-width: 1px;
  text-stroke-color: #ff0000;
}
```

### Auto font size

```css
.auto-size {
  -x-auto-font-size: true;
  -x-auto-font-size-preset-sizes: 12px, 14px, 16px;
}
```

## Guardrails

- use `<text>` for all visible text instead of placing raw text directly in `<view>`
- when exact wrapping behavior matters, set `white-space` explicitly instead of depending on implicit defaults
- when using `text-overflow: ellipsis`, give the text a resolved width or `max-width`
- nested `<text>` nodes inherit outer text styling unless overridden locally

## Common Pattern

### Rich text span composition

```tsx
<text className="rich-text">
  <text className="highlight">Hot</text>
  <text> item</text>
</text>
```

```css
.rich-text {
  font-size: 14px;
  color: #666666;
}

.highlight {
  color: #ff351a;
}
```
