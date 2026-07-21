# Lynx view Element

This guide covers the common `<view>` element usage for container layout and visual styling in Lynx.

## When to Use `<view>`

### Use `<view>` when:

- you need a generic container for layout, spacing, background, border, or overflow control
- you need a block-level wrapper around other Lynx elements
- you need a structural node for Flex, Grid, Linear, or Relative layout composition
- you need a tappable or styled container that is not text-specific or media-specific

### Do not use `<view>` when:

- the content is plain text and should be rendered through `<text>`
- you need an image-specific loading surface; use `<image>` or `<svg>`
- you need a scroll container; use `<scroll-view>` or `<list>`
- you need the page root; use the implicit or explicit `<page>` root instead

## Quick Start

### Basic container

```tsx
<view className="card">
  <text className="title">Card Title</text>
</view>
```

```css
.card {
  width: 100%;
  padding: 16px;
  margin: 8px;
  background-color: #ffffff;
  border: 1px solid #e0e0e0;
  border-radius: 8px;
}
```

### Flex container

```css
.row {
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  gap: 10px;
}
```

### Grid container

```css
.grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16px;
}
```

## Common Styling Patterns

### Box styling

```css
.panel {
  width: 100%;
  height: 200px;
  padding: 16px;
  margin: 8px;
  background-color: #ffffff;
  background-image: url('bg.png');
  background-size: cover;
  background-position: center;
  border: 1px solid #e0e0e0;
  border-radius: 8px;
}
```

### Visual effects

```css
.effects {
  opacity: 0.8;
  overflow: hidden;
  clip-path: inset(0 0 0 0);
  transform: translateX(10px) scale(1.1);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}
```

## Guardrails

- do not place raw text directly inside `<view>`; wrap text content in `<text>`
- use `<view>` for structure and styling, not for inline text behavior
- background images on `<view>` are for decorative backgrounds; use `<image>` when you need load events or image-specific props
- if the container needs to scroll, switch to `<scroll-view>` or `<list>` instead of expecting `<view>` overflow to behave like a Web scroll box by itself

## Common Pattern

### Wrap text correctly

```tsx
// Avoid raw text directly inside <view>
<view className="card">
  <text>Text content</text>
</view>
```
