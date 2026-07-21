# Lynx svg Element

This guide covers the current native Android, iOS, and Harmony `<svg>` element behavior.

## When to Use `<svg>`

### Use `<svg>` when:

- you need native SVG rendering on Android, iOS, and Harmony
- you have an SVG resource URL and want to render it through `src`
- you have inline SVG markup and want to render it through `content`
- you need a `load` callback after the SVG surface has rendered
- you want to opt into the Serval-backed renderer on iOS or Android through `enable-serval-svg`

### Do not use `<svg>` when:

- the asset is a normal bitmap and should go through `<image>`
- you want to rely on a portable frontend `error` event
- you want to call element-specific UI methods through `invoke(...)`
- you want to rely on `<x-svg>` as a cross-platform tag name
- you need a container that hosts Lynx child content

## Portability Guardrails

- prefer `<svg>`, not `<x-svg>`
- use either `src` or `content`, not both
- give the element a resolved size; rendering is driven by the current layout box
- use `enable-serval-svg` only on iOS or Android
- treat `bindload` as the only verified frontend event in the current native paths
- treat `allow-edge-antialiasing` as Harmony-only local `<svg>` behavior
- treat `data:image/svg+xml;base64,...` in `src` as Android and Harmony-proven only

## Quick Start

### Inline SVG markup

```tsx
<svg
  content={`<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
    <circle cx="12" cy="12" r="10" fill="#0f766e" />
  </svg>`}
  bindload={(event) => {
    const { width, height } = event.detail;
    console.log('svg loaded', width, height);
  }}
  style={{ width: '24px', height: '24px' }}
/>
```

### Remote SVG resource

```tsx
<svg
  src="https://example.com/icon.svg"
  style={{ width: '96px', height: '96px' }}
/>
```

### Serval mode on iOS or Android

```tsx
<svg
  enable-serval-svg={true}
  content={`<svg viewBox="0 0 48 48" xmlns="http://www.w3.org/2000/svg">
    <rect x="4" y="4" width="40" height="40" rx="8" fill="#0891b2" />
  </svg>`}
  bindload={() => console.log('serval-backed svg loaded')}
  style={{ width: '48px', height: '48px' }}
/>
```

## Properties

### Core props

| Prop | Type | What It Does |
| --- | --- | --- |
| `src` | `string` | Loads SVG markup from a resource path through the native SVG fetch path |
| `content` | `string` | Renders inline SVG markup directly |

### Platform-specific props

| Prop | Platforms | What It Does |
| --- | --- | --- |
| `allow-edge-antialiasing` | Harmony | Passes an anti-alias flag into Harmony's `SvgData` render input |
| `enable-serval-svg` | iOS, Android | Switches the internal wrapper path to the Serval-backed renderer |

### Source selection rule

- use `src` for remote, local, or host-resolved SVG resources
- use `content` for inline SVG markup
- do not keep both active at once in portable code

### Serval mode rule

- on iOS and Android, set `enable-serval-svg={true}` when the product wants the Serval-backed renderer path
- on Harmony, the registered path is already Serval-backed, so you should not assume the prop is needed or available
- for exact tag and attribute support in Serval mode, consult the published Serval SVG support matrix rather than inferring from wrapper behavior alone

## Events

### Frontend events

| Event Binding | Platforms | When It Fires | Payload |
| --- | --- | --- | --- |
| `bindload` | Android, iOS, Harmony | After the native SVG surface updates with rendered content | `width`, `height` |

### Event example

```tsx
const handleSvgLoad = (event) => {
  const { width, height } = event.detail ?? {};
  console.log('rendered size', width, height);
};

<svg
  content={svgMarkup}
  bindload={handleSvgLoad}
  style={{ width: '120px', height: '120px' }}
/>;
```

There is no verified native `<svg>` `binderror` event in the current mobile paths. If you need fallback UI, handle it outside the element instead of assuming a native error callback exists.

## UI Methods

This guide does not document any element-specific UI methods for `<svg>`.

- no local `focus`, `reload`, `setValue`, or other `invoke(...)` API is verified for the current native `<svg>` paths
- avoid selector-query or NodeRef method examples for `<svg>`

## Choosing Between `<svg>` and `<image>`

| Scenario | Use |
| --- | --- |
| SVG markup or `.svg` resource | `<svg>` |
| Bitmap formats such as PNG, JPG, GIF, or WebP | `<image>` |
| Need image loader reporting or image animation methods | `<image>` |
| Need native SVG rendering with a `load` callback | `<svg>` |

## Serval Support Questions

When you are using the Serval-backed path, use the published Serval SVG support contract for product-facing support questions such as:

- whether a tag is supported, partial, or unsupported
- which attributes are supported
- whether support differs by backend
- whether a concrete SVG is inside the published support surface

Useful references:

- `https://github.com/lynx-family/serval-kit/blob/main/svg/skills/serval-svg/SKILL.md`
- `https://github.com/lynx-family/serval-kit/blob/main/svg/docs/support-matrix.json`

## Practical Serval Capability Range

### Stable cross-platform core tags

- `svg`, `g`, `defs`, `use`
- `rect`, `circle`, `ellipse`, `line`, `polygon`, `polyline`, `path`
- `linearGradient`, `radialGradient`, `stop`
- `clipPath`, `mask`

### Important caveats

- width and height on `svg` are not the main portable sizing control; the renderer primarily uses the external viewport and layout box
- `svg` and `g` opacity do not behave like isolated group compositing; opacity is inherited into children
- `line` is effectively stroke-driven; do not depend on `fill` for visible output
- `radialGradient` is supported, but Android does not keep a distinct focus point in the final shader
- `clipPath` is supported, but `clipPath` container transforms are not fully applied

### Validation-required areas

- `image` is only partial support on Android and iOS, and unsupported on Harmony
- `text` and `tspan` are only partial support on Android and iOS, and unsupported on Harmony
- mobile Serval backends should treat the SVG filter family as unsupported in practice

## Platform Availability Matrix

| Feature | iOS | Android | Harmony |
| --- | --- | --- | --- |
| `<svg>` tag | Yes | Yes | Yes |
| `<x-svg>` alias | No | Yes | Not proven |
| `src` | Yes | Yes | Yes |
| `content` | Yes | Yes | Yes |
| `enable-serval-svg` prop | Yes | Yes | No local prop proven |
| Serval-backed renderer path | Yes | Yes | Yes |
| Explicit base64 `src` branch | Not locally proven | Yes | Yes |
| `bindload` | Yes | Yes | Yes |
| `binderror` | No | No | No |
| Element-specific UI methods | No | No | No |
| `allow-edge-antialiasing` local prop path | No local path proven | No | Yes |
