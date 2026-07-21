# CSS values and units

CSS value types and units supported by Lynx.

## Length units

### Absolute units

- `px` - Pixels (most commonly used)

### Relative units

- `%` - Relative to a property-specific reference size, often the containing block
- `vw` - A percentage of the viewport width
- `vh` - A percentage of the viewport height
- `rem` - Relative to the root element's font size (**recommended**)
- `em` - Relative to the font size
- `rpx` - Responsive pixels (Lynx-specific; scales automatically with the screen width)

### Unsupported units

- ❌ `cm`, `mm`, `in`, `pt`, `pc` - Physical units
- ❌ `ch`, `ex` - Font-relative units

### Partially supported units

- ⚠️ `vmin`, `vmax`

Lynx does not automatically normalize unsupported CSS units to `px` at runtime. If styles come from the Web, a design system, or external configuration, convert them to Lynx-supported units before generating the styles. Do not emit unsupported units such as `in`, `pt`, or `ex` directly into Lynx styles.

Lynx does not natively support the Web CSS `in` unit. However, a style generator or migration layer can convert it according to the CSS absolute-length conversion: `1in = 96px`. For example, `0.25in = 24px` and `1.25in = 120px`. This conversion does not vary with Android, iOS, or the device's physical DPI. Other unsupported units must still be treated as unsupported; this document does not define conversion rules for them. Unless the migration layer defines an explicit rule, do not infer that a unit can be converted automatically. In particular, do not convert font-relative units such as `ex` or `ch` without supporting evidence.

## Responsive scaling with rem (recommended)

For responsive scaling, use `rem` together with `vw`. This standards-based Web approach provides better compatibility.

```css
/* Set the base font size on the root element */
page {
  font-size: calc(100vw / 23.4375); /* 1rem = 16px at a viewport width of 375px */
}

/* Scale dimensions with rem */
.container {
  width: 100%;
  padding: 2rem; /* 32px at a viewport width of 375px */
}

.card {
  width: 21.4rem; /* Approximately 342px at a viewport width of 375px */
  margin: 1rem;
}
```

**Configuration:**

1. Set `font-size: calc(100vw / 23.4375)` on the `page` element.
2. Calculate the divisor as `reference width / desired base font size` (for example, 375px / 16px = 23.4375).
3. Express all dimensions in `rem`.

## Responsive pixels with rpx (Lynx-specific)

⚠️ **Note:** `rpx` is a **Lynx-specific** unit and is not Web-compatible. Use `rem` if the same styles must also render on the Web.

An `rpx` value scales automatically with the screen width according to `value × screen_width / 750`. The unit is fully supported and suitable for adaptive layouts.

**Conversion rules** (using 750rpx as the reference width):

- On a 375px-wide screen: 1rpx = 0.5px
- On a 750px-wide screen: 1rpx = 1px
- On a 1125px-wide screen: 1rpx = 1.5px

```css
/* Full width */
.full-width {
  width: 750rpx;
}

/* Half width */
.half-width {
  width: 375rpx;
}
```

**Use cases for rpx:**

- Lynx-only projects that do not require Web compatibility
- Dimensions that need to scale with the screen width

## Color values

### Named colors

```css
color: red;
background-color: blue;
border-color: transparent;
```

### Hexadecimal notation

```css
/* Shorthand */
color: #f00;

/* Full notation */
color: #ff351a;

/* With an alpha channel */
color: #ff351a80; /* 50% opacity */
color: rgba(255, 53, 26, 0.5);
```

### RGB/RGBA

```css
color: rgb(255, 53, 26);
color: rgba(255, 53, 26, 0.5);
```

### HSL/HSLA

```css
color: hsl(9, 100%, 55%);
color: hsla(9, 100%, 55%, 0.5);
```

## Numeric values

```css
/* Unitless numbers */
opacity: 0.5;
flex-grow: 1;
z-index: 100;
line-height: 1.5;

/* Dimensions */
width: 100px;
font-size: 14px;
```

## Percentages

```css
/* Relative to the parent element's width */
width: 50%;

/* Relative to the parent element's height */
height: 100%;

/* Relative to the font size */
line-height: 150%;
```

## Special values

### auto

```css
width: auto;
height: auto;
margin: auto;
```

### none

```css
display: none;
background: none;
border: none;
```

### inherit

```css
color: inherit;
font-size: inherit;
```

### initial

```css
color: initial;
```

### Global keywords

- `initial` - The property's initial value
- `inherit` - The computed value from the parent element

## Strings

```css
/* Font family names */
font-family: 'PingFang SC', 'Helvetica Neue', sans-serif;

/* Image URL */
background-image: url('https://example.com/image.png');
```

## Functions

### Calculations

```css
/* Partial calc() support: length-valued properties only */
width: calc(100% - 20px);
height: calc(50% + 100px);
padding: calc(var(--base) * 2);
```

**Properties that support `calc()`:**

- Sizing: `width`, `height`, `min-width`, `max-width`, `min-height`, `max-height`
- Box model: `padding`, `margin`, `flex-basis`
- Gaps: `gap`, `column-gap`, `row-gap`

**Properties that do not support `calc()`:**

- Keyword-valued properties: `flex-direction`, `justify-content`, `linear-orientation`, and others
- Integer-valued properties: `order`, `z-index`, `font-weight`, and others
- Color properties: `color`, `background-color`, and others

### Environment variables

```css
/* Safe area insets */
.safe-area {
  padding-top: env(safe-area-inset-top);
  padding-bottom: env(safe-area-inset-bottom);
}
```

### Gradients

```css
/* Linear gradients */
background: linear-gradient(to bottom, #ff351a, #00ebeb);
background: linear-gradient(45deg, #ff351a 0%, #00ebeb 100%);

/* Radial gradient */
background: radial-gradient(circle, #ff351a, #00ebeb);
```

### Animation timing functions

Used by properties such as `transition-timing-function` and `animation-timing-function`.

#### Keywords

- `linear`
- `ease` (equivalent to `ease-in-out` in Lynx)
- `ease-in`
- `ease-out`
- `ease-in-out`
- `ease-in-ease-out` (a synonym for `ease` and `ease-in-out` in Lynx)
- `step-start` (equivalent to `steps(1, start)`)
- `step-end` (equivalent to `steps(1, end)`)

> **Note:** In the Lynx engine, `ease`, `ease-in-out`, and `ease-in-ease-out` all map to the same timing function. This differs from Web CSS, where `ease` and `ease-in-out` use different cubic-bezier curves.

#### Functions

```css
/* Custom cubic-bezier curve */
transition-timing-function: cubic-bezier(0.4, 0, 0.2, 1);

/* Step timing functions: the direction argument is required */
animation-timing-function: steps(5, end);
animation-timing-function: steps(3, jump-start);
animation-timing-function: steps(4, jump-none);
animation-timing-function: steps(4, jump-both);

/* Lynx-specific extension */
transition-timing-function: square-bezier(1, 0.5);
```

**`cubic-bezier(...)` details:**

- The syntax is `cubic-bezier(x1, y1, x2, y2)`.
- Control points outside the `[0, 1]` range are parsed and applied.
- When an x-coordinate falls outside `[0, 1]`, the engine extrapolates linearly using the gradient at the start or end point; it neither reports an error nor clamps the value.
- If both x1 and x2 are within `[0, 1]`, the curve is monotonically increasing. Outside that range, the curve may be non-monotonic.

**`steps(...)` details:**

- The syntax is `steps(n, direction)`, and the direction argument is **required**.
- Omitting the direction, as in `steps(1)`, is unsupported and makes the value invalid.
- Supported direction values: `start`, `jump-start`, `end`, `jump-end`, `jump-none`, and `jump-both`.

## Intrinsic sizing

### Supported values

- ✅ `max-content` - Maximum intrinsic content width
- ✅ `fit-content` - Fits the width to the content

```css
.container {
  width: max-content; /* Size automatically to the content */
  width: fit-content; /* Fit the content without exceeding the containing block */
}
```

### Unsupported value

- ❌ `min-content` - Not supported for intrinsic `width` or `height`; in `flex-basis`, it degrades to `0px`

```css
/* ❌ Unsupported for intrinsic width */
.container {
  width: min-content;
}

/* ⚠️ Parses but degrades to 0px */
.flex-item {
  flex-basis: min-content; /* Do not use this */
}
```
