# Linear Layout (Default Layout)

Linear Layout is Lynx's default layout system and is similar to Android's `LinearLayout`. When you omit the `display` property, Lynx uses Linear Layout. It is suitable when child elements only need to be arranged in sequence and do not require the advanced sizing and placement capabilities of Flexbox or Grid.

Like Flexbox, Linear Layout uses a main axis and a cross axis. The main axis follows the direction in which child elements are laid out, and the cross axis is perpendicular to it.

## Basic Usage

```jsx
// Child elements are arranged vertically by default
<view className="container">
  <text>Item 1</text>
  <text>Item 2</text>
  <text>Item 3</text>
</view>
```

```css
.container {
  /* No display declaration is required; linear is the default */
  padding: 16px;
}

.explicit-container {
  display: linear;
}
```

## Direction

Use `linear-direction` to set the main-axis direction. Its default value is `column` (vertical), and it is similar to Flexbox's `flex-direction`.

```css
.row {
  display: linear;
  linear-direction: row;
}

.column {
  display: linear;
  linear-direction: column; /* Default */
}
```

**All `linear-direction` values:**

- `row` - Arranges items horizontally.
- `column` - Arranges items vertically (default).
- `row-reverse` - Arranges items horizontally in reverse order.
- `column-reverse` - Arranges items vertically in reverse order.

```jsx
<view className="row">
  <text>Left</text>
  <text>Center</text>
  <text>Right</text>
</view>
```

## Weight Distribution

Use `linear-weight` to distribute the remaining space along the main axis:

```css
.container {
  display: linear;
  linear-direction: row;
}

.sidebar {
  width: 200px; /* Fixed width */
}

.main {
  linear-weight: 1;
}

.aside {
  linear-weight: 0.5;
}
```

```jsx
<view className="container">
  <view className="sidebar">
    <text>Sidebar</text>
  </view>
  <view className="main">
    <text>Main Content</text>
  </view>
  <view className="aside">
    <text>Aside</text>
  </view>
</view>
```

In this example, `linear-weight` is a unitless ratio. The fixed-width `sidebar` consumes main-axis space first, and `main` and `aside` then divide the remaining space in a `1 : 0.5` ratio.

## Alignment

Linear Layout supports the following Web alignment properties:

```css
/* Center along the main axis */
.main-axis-center {
  display: linear;
  linear-direction: column;
  justify-content: center;
}

/* Center along the cross axis */
.cross-axis-center {
  display: linear;
  linear-direction: row;
  align-items: center;
}

/* Override cross-axis alignment for one child */
.cross-axis-center .first-item {
  align-self: end;
}
```

> **Note:** When the parent has a definite cross-axis size (for example, `width` when `linear-direction: column`) and a child has no specified size in that axis (or uses `auto`), the child grows along the cross axis to fill the parent.

## Property Reference

### `linear-direction`

Controls the layout direction:

| Value            | Description                           |
| ---------------- | ------------------------------------- |
| `row`            | Arranges items horizontally           |
| `column`         | Arranges items vertically (default)   |
| `row-reverse`    | Arranges items horizontally in reverse order |
| `column-reverse` | Arranges items vertically in reverse order   |

### `justify-content` (Container Property)

Controls child alignment along the main axis:

| Value           | Description                                      |
| --------------- | ------------------------------------------------ |
| `start`         | Aligns items to the main-axis start              |
| `end`           | Aligns items to the main-axis end                |
| `flex-start`    | Aligns items to the main-axis start              |
| `center`        | Centers items along the main axis                |
| `flex-end`      | Aligns items to the main-axis end                |
| `space-between` | Places the first and last items at opposite ends and distributes the remaining space evenly between items |

> **Note:** `space-around` and `space-evenly` exist in the CSS API, but in Linear Layout they behave like `start`. Use Flexbox when you need actual distributed spacing.

### `align-items` (Container Property)

Sets the default cross-axis alignment for all child elements:

| Value        | Description                       |
| ------------ | --------------------------------- |
| `center`     | Centers items on the cross axis   |
| `start`      | Aligns items to the cross-axis start |
| `end`        | Aligns items to the cross-axis end   |
| `flex-start` | Aligns items to the cross-axis start |
| `flex-end`   | Aligns items to the cross-axis end   |

> **Note:** In Linear Layout, `align-items` controls alignment only and does not support `stretch` or `auto`. Whether a child fills the parent's cross axis depends on whether the parent's cross-axis size is definite, as described above. `baseline` is currently supported only by Flexbox and is not available for cross-axis alignment in Linear Layout.

### `align-self` (Child Property)

Controls the cross-axis alignment of one child and overrides the container's `align-items` value:

| Value        | Description                              |
| ------------ | ---------------------------------------- |
| `auto`       | Uses the parent container's `align-items` value |
| `center`     | Centers the item on the cross axis       |
| `start`      | Aligns the item to the cross-axis start  |
| `end`        | Aligns the item to the cross-axis end    |
| `flex-start` | Aligns the item to the cross-axis start  |
| `flex-end`   | Aligns the item to the cross-axis end    |

> **Note:** `baseline` and `stretch` are currently supported only by Flexbox and are not available as `align-self` values for Linear Layout children.

### `linear-weight`

A numeric value that specifies a child's share of the remaining space along the main axis.

```css
.item {
  linear-weight: 1; /* Takes one share of the remaining main-axis space */
}
```

### Compatibility Properties

Legacy code may use the following Linear-specific properties. You can keep them when maintaining existing pages; for new examples, prefer `linear-direction` and the Web alignment properties.

| Legacy Property          | Recommended Replacement |
| ------------------------ | ----------------------- |
| `linear-orientation`     | `linear-direction`      |
| `linear-gravity`         | `justify-content`       |
| `linear-layout-gravity`  | `align-self`            |
| `linear-cross-gravity`   | `align-items`           |
| `linear-weight-sum`      | Usually let the engine calculate it automatically |

## Linear Layout vs. Flexbox

| Capability     | Linear Layout                  | Flexbox                 |
| -------------- | ------------------------------ | ----------------------- |
| Performance    | Faster for simple layouts      | Slightly slower         |
| Complex sizing | Does not handle complex shrink scenarios | Supports flex shrinking |
| Wrapping       | Not supported                  | Supports wrapping       |
| Multi-line alignment | Not supported             | Supports `align-content` |
| Child ordering | Can use `order`                | Supports `order`        |
| Best suited for | Simple lists                  | Complex flexible layouts |

**Recommendation:** Use Linear Layout for simple lists and Flexbox for complex layouts.

## Differences from Web Block Layout (Migration Notes)

**This is one of the most common pitfalls when migrating from the Web to Lynx.**

### Default Layout Differences

| Behavior             | Web Block Layout                         | Lynx Linear Layout (Default)                     |
| -------------------- | ---------------------------------------- | ------------------------------------------------ |
| Default `display`    | `block` (normal flow)                    | `linear` (`linear-direction: column` by default) |
| Child width          | `width: auto` uses the parent's available width | With the default `column` direction, width is the cross-axis size and fills the parent's cross axis when unspecified |
| Child height         | Determined by content                    | With the default `column` direction, height is the main-axis size and is determined by content, `linear-weight`, or an explicit size |
| Nested element size  | Width uses each parent's available width; height is determined by content | The cross-axis size can follow the parent, but the main-axis size does not fill automatically |
| Empty element size   | May be 0, depending on `overflow` and `min-height` | When the main-axis size is unspecified and there is no content, it is usually 0 |

### Child-Sizing Example

```css
/* Web: width: auto uses the parent's available width */
.parent { width: 200px; }
.child { /* width defaults to auto and uses the parent's available width */ }

/* Lynx: an unspecified cross-axis size fills the parent's cross axis */
.parent { width: 200px; display: linear; linear-direction: column; }
.child { /* width defaults to auto and equals the parent's content-box width minus horizontal margins */ }

/* Lynx: the main-axis size does not fill automatically */
.parent { height: 200px; display: linear; linear-direction: column; }
.child { /* Without a specified height, content still determines the height; it does not become 200px automatically */ }
```

### Nested-Element Sizing Example

```css
/* Web: a nested div automatically uses its parent's available width */
.grandparent { width: 100px; height: 100px; }
.parent { /* width defaults to auto and resolves to 100px; height is still determined by content */ }
.child { /* width defaults to auto and follows the parent's available width; height is still determined by content */ }

/* Lynx: cross-axis width follows the parent, but main-axis height does not fill automatically */
.grandparent { width: 100px; height: 100px; display: linear; linear-direction: column; }
.parent { /* Without width, it is 100px wide; without height and content, its height may be 0 */ }
.child { /* Without width, it follows the parent; without height and content, its height may be 0 */ }

/* Explicitly set the main-axis size when an element must fill both dimensions */
.parent { width: 100%; height: 100%; }
.child { width: 100%; height: 100%; }
```

## Common Layout Patterns

### Vertical List

```css
.list {
  display: linear;
  /* linear-direction: column is the default */
  gap: 12px;
  padding: 16px;
}

.list-item {
  padding: 12px;
  background-color: #f5f5f5;
  border-radius: 8px;
}
```

### Horizontal Button Group

```css
.button-group {
  display: linear;
  linear-direction: row;
  gap: 8px;
  padding: 16px;
}

.button-group .button {
  linear-weight: 1; /* Divides the remaining main-axis space equally */
}
```

### Sidebar and Main Content

```css
.layout {
  display: linear;
  linear-direction: row;
  height: 100vh;
}

.sidebar {
  width: 250px;
}

.main {
  linear-weight: 1; /* Takes the remaining main-axis space */
}
```

### Cross-Axis Centering

```css
.toolbar {
  display: linear;
  linear-direction: row;
  align-items: center;
}
```
