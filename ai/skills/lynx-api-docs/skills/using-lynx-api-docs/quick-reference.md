# Lynx CSS Quick Reference

Quick reference for the most commonly used Lynx CSS and layout system features.

## Layout System Cheat Sheet

| Scenario                           | Recommended Display | Description                            |
| ---------------------------------- | ------------------- | -------------------------------------- |
| Simple list (performance priority) | `linear`            | Default layout, similar to Flex Column |
| Flexible layout                    | `flex`              | Standard CSS Flexbox                   |
| 2D grid                            | `grid`              | CSS Grid subset                        |
| Relative positioning               | `relative`          | Android-style relative positioning     |

## Display Types

| Value      | Description                       | Version |
| ---------- | --------------------------------- | ------- |
| `none`     | Hide element                      | 1.0     |
| `flex`     | Flexbox layout                    | 1.0     |
| `grid`     | Grid layout                       | 1.0     |
| `linear`   | Linear layout (default)           | 1.0     |
| `relative` | Relative layout                   | 2.0     |
| `block`    | Compatibility value; resolves to a Lynx layout mode | 2.0     |
| `auto`     | Automatic selection               | 2.0     |

## Supported CSS Properties (Common)

### Box Model

```css
width, height, min-width, min-height, max-width, max-height
margin, margin-top, margin-right, margin-bottom, margin-left
padding, padding-top, padding-right, padding-bottom, padding-left
border, border-radius
box-sizing (default: auto)
```

### Flex Layout

```css
display: flex
flex-direction: row | column | row-reverse | column-reverse
flex-wrap: wrap | nowrap | wrap-reverse
flex-flow: <direction> <wrap>  /* shorthand */
justify-content: flex-start | center | flex-end | space-between | space-around | space-evenly | stretch  /* stretch: 2.1+ */
align-items: flex-start | center | flex-end | stretch | baseline
align-self: auto | flex-start | center | flex-end | stretch | baseline
flex-grow, flex-shrink, flex-basis
flex: <grow> <shrink> <basis>  /* shorthand */
order
gap, row-gap, column-gap
```

### Grid Layout (Subset)

```css
display: grid
grid-template-columns, grid-template-rows
grid-auto-columns, grid-auto-rows
grid-column-start, grid-column-end
grid-row-start, grid-row-end
gap, row-gap, column-gap
grid-auto-flow: row | column | dense | row dense | column dense
justify-items, justify-self
align-items, align-self
```

> **Note:** Lynx does not support `grid-area` or named grid lines. The `grid-column` and `grid-row` shorthand properties require Lynx 3.9 or later with `enableGridPlacementShorthands: true`. For compatibility, prefer the `grid-column-start`/`grid-column-end` and `grid-row-start`/`grid-row-end` longhand properties. `grid-column-span` and `grid-row-span` are Lynx-specific properties and are not recommended for new code.

### Linear Layout (Lynx-Specific)

```css
display: linear
linear-orientation: horizontal | vertical | row | column | row-reverse | column-reverse
linear-direction: row | column | row-reverse | column-reverse  /* 2.2+ */
linear-gravity: top | bottom | left | right | center | center-vertical | center-horizontal | start | end | space-between
linear-layout-gravity: none | top | bottom | left | right | center | fill-vertical | fill-horizontal | stretch
linear-cross-gravity: none | start | end | center | stretch
linear-weight: <number>
linear-weight-sum: <number>
```

### Relative Layout (Lynx-Specific)

```css
display: relative
relative-id: <number>
relative-align-top: <id> | parent
relative-align-bottom: <id> | parent
relative-align-left: <id> | parent
relative-align-right: <id> | parent
relative-align-inline-start: <id> | parent  /* RTL support */
relative-align-inline-end: <id> | parent  /* RTL support */
relative-top-of: <id>
relative-bottom-of: <id>
relative-left-of: <id>
relative-right-of: <id>
relative-inline-start-of: <id>  /* RTL support */
relative-inline-end-of: <id>  /* RTL support */
relative-center: none | vertical | horizontal | both
```

### Positioning

```css
position: relative | absolute | fixed | sticky
top, right, bottom, left
inset-inline-start, inset-inline-end /* RTL support */
z-index
```

> **⚠️ z-index Note**: In scrollable containers such as `scroll-view` and `scroll-coordinator`, child elements with `z-index` may not follow scrolling. **Solution**: Add `z-index: 0` to the parent container to establish the same stacking context.

### Visual

```css
background, background-color, background-image
background-size, background-position, background-repeat
opacity
transform: translate, scale, rotate
```

### Text

```css
color, font-size, font-weight, font-family, font-style
line-height, text-align, text-decoration
white-space: normal | nowrap
text-overflow: clip | ellipsis
```

## Lynx vs Web CSS Key Differences

| Web CSS                   | Lynx Support                     | Notes                                                |
| ------------------------- | -------------------------------- | ---------------------------------------------------- |
| `display: block`          | ⚠️ Compatibility value (2.0+)   | Resolves to a Lynx layout mode; prefer explicit Flex/Linear |
| `display: inline`         | ❌ Not supported                 | Use `<text>` component                               |
| `display: inline-block`   | ❌ Not supported                 | Use Flex/Grid                                        |
| `float`                   | ❌ Not supported                 | Use Flex/Grid                                        |
| `position: static`        | ❌ Not supported                 | Use `position: relative`                             |
| `box-sizing: content-box` | ⚠️ Supported but not default | Default is `border-box` (Web default is `content-box`) |
| `min-content`             | ❌ Unsupported for width/height  | `flex-basis: min-content` degrades to `0px`          |
| `max-content`             | ✅ Supported                     | Available                                            |
| `fit-content`             | ✅ Supported                     | Available                                            |
| Margin collapsing         | ❌ Not supported                 | Margins do not collapse                              |

## Selector Support

### Fully Supported (Parsed + Matched)

```
✅ Tag selectors: view, text, image
✅ Class selectors: .class-name
✅ ID selectors: #id-name
✅ Descendant selectors: .parent .child
✅ Child selectors: .parent > .child
✅ Adjacent sibling: .prev + .next
✅ General sibling: .prev ~ .sibling
✅ Attribute selectors: [type="text"]
✅ Pseudo-classes: :hover, :active, :focus, :not(), :root
✅ Pseudo-elements: ::placeholder, ::selection
```

### Not Supported (Parsed but NOT Matched)

```
❌ Structural pseudo-classes: :first-child, :last-child, :nth-child()
❌ Pseudo-elements: ::before, ::after
```

> **Note:** These selectors are recognized by the CSS parser but lack matching implementation in the selector engine. They will not apply styles.

## Unit Support

```
✅ px - Pixels
✅ % - Percentage
✅ vw, vh - Viewport units
✅ rem - Root element font size (recommended for responsive)
✅ em - Relative font units
⚠️ rpx - Responsive pixels (Lynx-specific, lacks web compatibility)
❌ cm, mm, in, pt, pc - Physical units not supported
```

**Responsive Adaptation (Recommended)**:

```css
/* Set base font size on root element */
page {
  font-size: calc(100vw / 23.4375); /* 1rem = 16px @ 375px */
}

/* Use rem for responsive sizing */
.container {
  padding: 2rem;
  font-size: 1rem;
}
```

## CSS Function Support

### calc() - Partial Support

- ✅ **Supported in:** Length properties (width, height, margin, padding, flex-basis, etc.)
- ❌ **Not supported in:** Enum properties (flex-direction, justify-content, align-items, etc.), colors, or numbers

Example:

```css
/* ✅ Works */
width: calc(100% - 20px);
margin: calc(var(--spacing) * 2);

/* ❌ Does NOT work */
flex-direction: calc(row); /* Enum property */
color: calc(#ff0000); /* Color property */
```

### var() - Full Support

```css
/* Works in all properties */
color: var(--primary-color);
width: var(--sidebar-width);
```

### env() - Partial Support

- ✅ **Supported:** `safe-area-inset-*` for notched devices
- ❌ **Not supported:** Other environment variables
