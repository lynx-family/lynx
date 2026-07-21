# Supported CSS properties

Lynx supports most commonly used CSS properties, along with several Lynx-specific extensions.

## Layout properties

### Display

| Value               | Description                              | Version |
| ------------------- | ---------------------------------------- | ------- |
| `display: none`     | Hides the element                        | 1.0     |
| `display: flex`     | Flexbox layout                           | 1.0     |
| `display: grid`     | Grid layout                              | 1.0     |
| `display: linear`   | Linear layout (Lynx-specific and default) | 1.0     |
| `display: relative` | Relative layout (Lynx-specific)          | 2.0     |
| `display: block`    | Compatibility value that resolves to a Lynx layout mode | 2.0     |
| `display: auto`     | Selects a layout mode automatically      | 2.0     |

### Flexbox properties

- `flex-direction`
- `flex-wrap`
- `flex-flow` - Shorthand property
- `justify-content`
- `align-items`
- `align-self`
- `align-content`
- `flex-grow`
- `flex-shrink`
- `flex-basis`
- `flex` (shorthand)
- `order`
- `gap`, `row-gap`, `column-gap`

### Grid properties

- `grid-template-columns`
- `grid-template-rows`
- `grid-auto-columns`
- `grid-auto-rows`
- `grid-column-start`, `grid-column-end`
- `grid-row-start`, `grid-row-end`
- `grid-auto-flow`
- `justify-items`
- `justify-self`
- `align-items`
- `align-self`
- `gap`, `row-gap`, `column-gap`

### Linear layout properties (Lynx-specific)

- `linear-orientation`
- `linear-direction` - Lynx 2.2+
- `linear-gravity`
- `linear-layout-gravity`
- `linear-cross-gravity` - Lynx 1.6+
- `linear-weight`
- `linear-weight-sum`

### Relative layout properties (Lynx-specific)

- `relative-id`
- `relative-align-top`, `relative-align-bottom`
- `relative-align-left`, `relative-align-right`
- `relative-align-inline-start`, `relative-align-inline-end` - RTL support
- `relative-top-of`, `relative-bottom-of`
- `relative-left-of`, `relative-right-of`
- `relative-inline-start-of`, `relative-inline-end-of` - RTL support
- `relative-center`
- `relative-layout-once` - Layout optimization property

### Positioning properties

- `position: relative | absolute | fixed | sticky`
- `top`, `right`, `bottom`, `left`
- `z-index`
- `inset-inline-start`, `inset-inline-end` - RTL support

## Box model properties

### Sizing

- `width`, `height`
- `min-width`, `min-height`
- `max-width`, `max-height`
- `aspect-ratio`

### Margins

- `margin`
- `margin-top`, `margin-right`, `margin-bottom`, `margin-left`
- `margin-inline-start`, `margin-inline-end` (adapts to LTR/RTL)

### Padding

- `padding`
- `padding-top`, `padding-right`, `padding-bottom`, `padding-left`
- `padding-inline-start`, `padding-inline-end` (adapts to LTR/RTL)

### Borders

- `border`
- `border-top`, `border-right`, `border-bottom`, `border-left`
- `border-width`, `border-style`, `border-color`
- `border-radius`
- `border-top-left-radius`, `border-top-right-radius`
- `border-bottom-left-radius`, `border-bottom-right-radius`
- `border-start-start-radius`, `border-start-end-radius` - RTL support
- `border-end-start-radius`, `border-end-end-radius` - RTL support
- `box-shadow`

### Box model

- `box-sizing: border-box | content-box | auto` (defaults to `border-box`)
  - Both explicit `border-box` and `content-box` values are supported. When migrating a Web layout, do not evaluate compatibility based on `box-sizing` alone. Pixel-level differences between Lynx and the Web may remain if the original page also relies on floats, the browser's default `body` margin or line height, the containing block for absolutely positioned elements, or min/max size clamping.
- `overflow: visible | hidden | scroll` (partial support)
- `clip-path`

## Visual properties

### Backgrounds

- `background`
- `background-color`
- `background-image`
- `background-size`
- `background-position`
- `background-repeat`
- `background-origin`
- `background-clip`

**Compatibility notes:**

- `background-color` can be applied directly to ordinary `view` and `page` backgrounds. Common named colors, transparent colors, and the `inherit` global value are supported.
- The `background` shorthand supports common background values. When migrating a Web shorthand with multiple components, split it into supported longhand properties to verify that Lynx supports each component.
- `background-attachment` is not currently supported. If a Web shorthand contains an attachment keyword such as `fixed` or `scroll`, first determine whether its scroll-binding behavior is actually required. If only a color, image, repeat mode, or position is needed, specify the corresponding longhand properties explicitly:

  ```css
  .box {
    background-image: url('bg.png');
    background-repeat: repeat;
    background-position: 0 0;
    background-color: transparent;
  }
  ```

- On the Web, `html` and `body` backgrounds have special canvas painting and propagation rules. In Lynx, `page` is both the page root and the painting surface for the full-page background. For a full-page background, apply it to `page`. Move a Web `body` background to the root `view` only when the background belongs to the content container.
- `background-image: url(...)` can be combined with `background-size`, `background-position`, and `background-repeat` to control the image's size, position, and tiling. When migrating comma-separated background layers, avoid using `none` as a transparent placeholder layer. On the Web, a `none` layer still participates in index alignment across the other `background-*` layer lists:

  ```css
  /* Web: the middle none is a transparent placeholder, so the third position still applies to bottom.png */
  .panel {
    background-image: url(top.png), none, url(bottom.png);
    background-position: left top, center, right bottom;
  }
  ```

  To reproduce the same visual result in Lynx, remove the empty layer and the corresponding entry from every other `background-*` list. If preserving independent layer indices is required, use nested `<view>` elements to separate the background layers:

  ```css
  /* Lynx: remove the empty layer and keep the remaining background lists in the same order */
  .panel {
    background-image: url(top.png), url(bottom.png);
    background-position: left top, right bottom;
  }
  ```

- Small tiled bitmap backgrounds can produce seams of approximately 1 px at some Android display scale factors. For example, when a 15 px-wide image fills a 300 px container with `background-repeat: repeat-x`, Web browsers generally tile it without seams, while Lynx may expose the underlying background color at tile boundaries. For textures that must be strictly seamless, prefer a larger seamless asset or a solid-color background, or compose critical areas in separate `<image>` or `<view>` layers.
- Use caution when a `background` shorthand contains an image layer but omits clip and origin values. The default value of the `background-clip` longhand is `border-box` in both Lynx and Web CSS, but an image layer parsed from the shorthand may be painted relative to the `padding-box`. If the element also uses `border-style: dotted` or `dashed`, the parent or sibling background may show through the transparent gaps in the border. If the color beneath the border must not show through, split the shorthand into longhands such as `background-image`, `background-repeat`, `background-position`, and `background-size`, and set `background-clip: border-box` explicitly. Alternatively, use a solid border or separate the outer background and border from the inner image using two `<view>` elements.

### Color and opacity

- `color`
- `opacity`

### Transforms

- `transform: translate, translateX, translateY, translate3d`
- `transform: scale, scaleX, scaleY, scale3d`
- `transform: rotate, rotateX, rotateY, rotateZ, rotate3d`
- `transform-origin`

### Transitions and animations

- `transition`
- `transition-property`
- `transition-duration`
- `transition-timing-function`
- `transition-delay`
- `animation`
- `animation-name`
- `animation-duration`
- `animation-timing-function`
- `animation-delay`
- `animation-iteration-count`
- `animation-direction`
- `animation-fill-mode`

**Animation timing function support:** The keywords `ease`, `linear`, `ease-in`, `ease-out`, `ease-in-out`, and `ease-in-ease-out` are supported. Lynx also supports `cubic-bezier(...)`, `steps(...)`, and the Lynx-specific `square-bezier(...)`. See [CSS values and units](./values-and-units.md#animation-timing-functions) for details.

## Text properties

### Fonts

- `font-size`
- `font-weight`
- `font-family`
- `font-style`
- `line-height`

### Text styling

- `color`
- `text-align`
- `text-decoration`
- `text-overflow: clip | ellipsis`
- `white-space: normal | nowrap`

### Lynx-specific text properties

- `-x-auto-font-size`
- `-x-auto-font-size-preset-sizes`

## Other properties

### Visibility

- `visibility: visible | hidden`
- `display: none` (removes the element entirely)

### Content clipping

- `clip-path`
- `mask-image` (partial support)

### Writing direction

- `direction: ltr | rtl | normal`

### Lynx-specific properties

- `-x-handle-color`
- `-x-handle-size`
- `-x-app-region`
- `-x-overlay` (iOS only)

## Unsupported properties

The following Web CSS properties are **not supported** in Lynx:

- ❌ `display: inline` (use the `<text>` element)
- ❌ `display: inline-block`
- ❌ `float`
- ❌ `clear`
- ❌ `position: static`
- ❌ `table-*` properties
- ❌ `list-style-*` properties
- ❌ `writing-mode`

### Unsupported Grid properties

- ❌ `grid-area`
- ❌ Named grid lines: `[name] 1fr`
- ❌ `subgrid`

## Version history

- **1.0:** Initial layout systems (Linear, Flex, and Grid)
- **1.6:** Added `linear-cross-gravity` and the `stretch` value
- **2.0:** Added Relative layout and `display: block/auto`
- **2.1:** Added `justify-content: stretch` and expanded Grid support
- **2.2:** Added `linear-direction` as the replacement for `linear-orientation`
