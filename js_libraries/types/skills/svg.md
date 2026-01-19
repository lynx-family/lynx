---
name: svg
description: Provide information about the `<svg>`. It is a element that renders SVG graphics. This document details the basic usage and best practices for the `<svg>` element.
---

# Lynx UI SVG SKILL.md

The `<svg>` element is a container for SVG graphics. It consists of a set of SVG elements that can be used to create complex graphics. This document details the basic usage for the `<svg>` element.

## 1. Core Capabilities

- **Render SVG Graphics**: The `<svg>` element can render a set of SVG elements to create complex graphics. 

## 2. Basic Usage

`<svg>` element usage example:

```tsx

function MyPage() {
  return (
    <svg
      content={`<svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
<rect x="10" y="20" width="50" height="50" fill="#ccff66" stroke="#ff0000"/>
<rect x="10" y="30" width="50" height="50" fill="none"/>
<circle cx="50" cy="50" r="20" fill="#ff000088" stroke-width="10" stroke="#00ff0088" />
<line x1="0" x2="0" y1="0" y2="100" stroke="black"/>
</svg>`}
      style={{
        width: '374px',
        height: '125px',
      }}
    />
  )
}
```

## 3. Supported SVG Tags && Attributes

### Supported SVG Tags

The `<svg>` element supports the following subset of SVG tags:

- `circle`
- `clipPath`
- `defs`
- `text`
- `ellipse`
- `g`
- `image`
- `line`
- `linearGradient`
- `path`
- `polygon`
- `polyline`
- `radialGradient`
- `rect`
- `stop`
- `svg`
- `use`

### Supported SVG Attributes

The `<svg>` element supports the following subset of SVG attributes:

- `'xlink:href'` – used on `<image>`, `<use>`
- `'style'` – global on all tags
- `'points'` – used on `<polygon>`, `<polyline>`
- `'offset'` – used on `<stop>`
- `'font-size'` – used on `<text>`
- `'id'` – global on all tags
- `'xmlns'` – used on `<svg>`
- `'clip-path'` – global on all tags
- `'clip-rule'` – used on `<path>`, `<polygon>`, `<polyline>`
- `'clipPathUnits'` – used on `<clipPath>`
- `'color'` – global on all tags
- `'cx'` – used on `<circle>`, `<ellipse>`
- `'cy'` – used on `<circle>`, `<ellipse>`
- `'d'` – used on `<path>`
- `'fill'` – global on shape tags (`<rect>`, `<circle>`, `<path>`, etc.)
- `'fill-opacity'` – global on shape tags
- `'fill-rule'` – used on `<path>`, `<polygon>`, `<polyline>`
- `'fx'` – used on `<radialGradient>`
- `'fy'` – used on `<radialGradient>`
- `'gradientTransform'` – used on `<linearGradient>`, `<radialGradient>`
- `'gradientUnits'` – used on `<linearGradient>`, `<radialGradient>`
- `'height'` – used on `<rect>`, `<image>`, `<svg>`
- `'width'` – used on `<rect>`, `<image>`, `<svg>`
- `'href'` – used on `<image>`, `<use>`
- `'opacity'` – global on all tags
- `'preserveAspectRatio'` – used on `<svg>`, `<image>`
- `'r'` – used on `<circle>`
- `'rx'` – used on `<rect>`, `<ellipse>`
- `'ry'` – used on `<rect>`, `<ellipse>`
- `'spreadMethod'` – used on `<linearGradient>`, `<radialGradient>`
- `'stop-color'` – used on `<stop>`
- `'stop-opacity'` – used on `<stop>`
- `'stroke'` – global on shape tags
- `'stroke-dasharray'` – global on shape tags
- `'stroke-dashoffset'` – global on shape tags
- `'stroke-linecap'` – global on shape tags
- `'stroke-linejoin'` – global on shape tags
- `'stroke-miterlimit'` – global on shape tags
- `'stroke-opacity'` – global on shape tags
- `'stroke-width'` – global on shape tags
- `'transform'` – global on all tags
- `'viewBox'` – used on `<svg>`
- `'x'` – used on `<rect>`, `<text>`, `<image>`, `<use>`
- `'x1'` – used on `<line>`, `<linearGradient>`
- `'x2'` – used on `<line>`, `<linearGradient>`
- `'y'` – used on `<rect>`, `<text>`, `<image>`, `<use>`
- `'y1'` – used on `<line>`, `<linearGradient>`
- `'y2'` – used on `<line>`, `<linearGradient>`

## 4. Critical Development Advices

- **MUST**: If there is any unsupported SVG tag or attribute, please use `<image>` with `src` attribute to render bitmap image instead.
- **MUST**: The size of SVG should be determined by the CSS `width` and `height` attributes on the `<svg>` element rather than the `viewBox` attribute in the SVG content.
