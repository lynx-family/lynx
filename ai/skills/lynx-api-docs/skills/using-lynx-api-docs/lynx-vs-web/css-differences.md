# Differences Between Lynx and Web CSS

Lynx supports most commonly used CSS features, but there are several important differences.

## Key Differences at a Glance

### 1. The `display` Property

| Web                     | Lynx              | Notes                                                     |
| ----------------------- | ----------------- | --------------------------------------------------------- |
| `display: block`        | ⚠️ Compatibility value | Resolves to a Lynx layout mode; prefer explicit Flex or Linear layout |
| `display: inline`       | ❌ Not supported  | Use the `<text>` element instead                          |
| `display: inline-block` | ❌ Not supported  | Use Flex or Linear layout                                 |
| `display: table*`       | ❌ Not supported  | Use Grid or Flex instead; see the migration example below |
| `display: flex`         | ✅ Supported      | Standard Flexbox                                          |
| `display: grid`         | ✅ Supported      | A subset of CSS Grid                                      |
| `display: linear`       | ✅ Lynx-specific  | Similar to a Flex column, with better performance         |
| `display: relative`     | ✅ Lynx-specific  | Android-style relative layout                             |
| `display: none`         | ✅ Supported      | Hides the element                                         |

> **⚠️ Note**: Lynx does **not support** `display: table`, `table-row`, `table-cell`, or related values. The CSS parser treats them as invalid and falls back to the default layout. The Starlight layout engine has no `TableLayoutAlgorithm`.

> **⚠️ Warning**: Using `display: block` outside W3C standards mode produces this console warning:
>
> ```
> "Unexpected display type!! Fall back to default display."
> ```
>
> Use `display: flex` or `display: linear` instead.

### 2. Flex Shrinking and Content-Based Minimum Sizes

`flex-shrink` takes effect only when the available space on the main axis is insufficient: width for `flex-direction: row`, or height for `flex-direction: column`. If the parent container has no explicit `width` or `height` and is not constrained by an outer layout, a maximum size, or the viewport, it expands to fit its content. No negative free space is created, so `flex-shrink` does not change the final size.

The key difference between Lynx and the Web is the lower bound for main-axis shrinking. Web flex items default to `min-width: auto` or `min-height: auto`. Text, images, nested children, and other intrinsic content can therefore establish a content-based or `min-content` minimum size. Lynx does not automatically apply this protection when resolving `flex-shrink`; you must explicitly set `min-width`, `min-height`, or `flex-shrink: 0`. You can think of Lynx's default behavior as explicitly setting the flex item's main-axis minimum to `0` on the Web: similar to `min-width: 0` in a row or `min-height: 0` in a column.

This difference applies to both rows and columns:

```css
.row {
  display: flex;
  width: 80px;
}

.row .content-item {
  width: 120px;
  flex-shrink: 1;
}

.column {
  display: flex;
  flex-direction: column;
  height: 80px;
}

.column .content-item {
  height: 120px;
  flex-shrink: 1;
}
```

In Lynx, `.content-item` continues to shrink along the main axis, so its actual width or height may become smaller than its contents require. On the Web, the default `min-width: auto` in a row or `min-height: auto` in a column may establish an automatic minimum size based on the content. The item then stops shrinking, which can cause overflow, force sibling items to shrink, or produce different results between a Web preview and Lynx.

To make a Web preview match Lynx's shrinking behavior, explicitly disable the automatic minimum size on the Web:

```css
/* Row: allow the text item to keep shrinking */
.text-flex-item {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

/* Column: allow the vertical item to keep shrinking */
.column-flex-item {
  min-height: 0;
}
```

To make Lynx behave more like the Web when content must not shrink, explicitly protect the item in Lynx:

```css
.content-item {
  flex-shrink: 0;
  /* Alternatively, set an explicit minimum on the main axis. */
  min-width: 120px;
  min-height: 120px;
}
```

### 3. Default Layout Behavior (Important)

**The Web defaults to block flow layout, while Lynx defaults to Linear layout**:

```css
/* Web: block flow is the default when display is omitted. */
div {
  /* Default: display: block */
}
/* Block-level children with width: auto use the parent's available width
 * and are laid out vertically. */

/* Lynx: Linear layout is the default when display is omitted. */
view {
  /* Default: display: linear; linear-direction: column */
}
/* Children without a cross-axis size fill the parent's cross axis. */
```

**Effect on child sizing**:

In Web block layout, a block-level child with `width: auto` uses the parent's available width. In Lynx Linear layout, a child does not inherit the parent's size in every direction, but the cross axis stretches by default: when the parent's cross-axis size is definite and the child has no size in that direction, or its size is `auto`, the child grows to fill the parent's cross axis after subtracting margins on that axis.

With the default `linear-direction: column`, width is the cross axis. If the parent has an explicit `width` and the child does not, the child fills the parent's width. Height is the main axis, so a child without a `height` is still sized by its content, `linear-weight`, or an explicit size; it does not automatically fill the parent's height.

```css
/* Web: the child automatically fills the parent's available width. */
.parent {
  width: 200px;
}
.child {
  /* Default width: auto; uses the parent's available width. */
}

/* Lynx: in a column Linear layout, a child without width fills the cross axis. */
.parent {
  width: 200px;
  display: linear;
  linear-direction: column;
}
.child {
  /* Default width: auto; equals the parent's content width minus horizontal margins. */
}

/* Lynx: the main-axis size does not automatically stretch. */
.parent {
  height: 200px;
  display: linear;
  linear-direction: column;
}
.child {
  /* Without height, the content determines the height; it does not become 200px. */
}
```

**Nested element sizing**:

On the Web, the width of nested block elements uses the available width of each successive parent. In Lynx Linear layout, a nested `view` can follow its parent's cross-axis size, but it does not automatically fill the main axis. An empty `view` with no content, explicit main-axis size, or `linear-weight` may have a main-axis size of `0`.

```css
/* Web: nested div elements use each parent's available width. */
.grandparent {
  width: 100px;
  height: 100px;
}
.parent {
  /* width: auto resolves to 100px; content still determines the height. */
}
.child {
  /* width: auto follows the parent's available width; content determines the height. */
}

/* Lynx: cross-axis width follows the parent; main-axis height does not stretch. */
.grandparent {
  width: 100px;
  height: 100px;
  display: linear;
  linear-direction: column;
}
.parent {
  /* Without width, this is 100px wide; if empty and without height, its height may be 0. */
}
.child {
  /* Without width, this follows the parent; if empty and without height, its height may be 0. */
}

/* Explicitly set the main-axis size when both dimensions must be filled. */
.parent {
  width: 100%;
  height: 100%;
}
.child {
  width: 100%;
  height: 100%;
}
```

### 4. Cyclic Percentage Sizing

When a child uses a percentage size, its resolved size depends on its parent. If the corresponding size of the parent or an ancestor is `auto`, `fit-content`, or `max-content` and must be derived from its contents, a cyclic dependency is created:

1. The content size determines the container size.
2. The container size determines the content's percentage size.

Web browsers generally resolve such cycles according to CSS Sizing rules. If a percentage cannot initially be resolved, it participates in intrinsic-size calculation as `auto`. In layouts such as Flexbox, the browser may remeasure after the container receives a definite size and produce a new percentage result. For performance, Lynx Linear layout favors a single measurement pass. It does not repeatedly resolve cyclic dependencies as Web or Flex layouts do, so percentage sizes are more likely to have no effect, an empty node may have a main-axis size of `0`, or Lynx and Web may produce different results.

Typical risk scenarios include:

- A child has `height: 50%` inside a container with `height: auto`, while the subtree's content determines the container height.
- Text or a component has `width: 50%` inside a container whose `width` is `fit-content` or otherwise content-sized.
- A ReactLynx 2 component has no explicit `display`, `width`, or `height`, causing an intermediate layer to use the default layout and interrupt the expected Flex percentage-resolution chain.
- Multiple nested percentage sizes in the default Linear layout depend on an `auto` main-axis size, such as multiple levels of `height: 50%` in a horizontal Linear layout.

```css
/* Risky: content determines the parent container's height,
 * while the child's height depends on that container. */
.outer {
  display: linear;
  height: auto;
  width: 200px;
}

.middle {
  display: linear;
  height: 50%;
}

.inner {
  height: 100px;
}
```

Migration guidance:

- Avoid percentage sizes beneath a content-sized ancestor. Set an explicit `width` or `height` on key ancestors in the dependency chain.
- When allocating remaining space, prefer `flex-grow` and `flex-shrink`, or `linear-weight` in Linear layout, instead of first sizing the container from its content and then applying percentages.
- If you must rely on cyclic percentage sizing, verify that the entire subtree uses Flex layout. An intervening Linear layout, an element with custom measurement such as `<text>`, or a ReactLynx 2 component's default layout can all produce results that differ from the Web.
- In ReactLynx 2, a component is itself a layout element. Explicitly set `display`, `width`, and `height` on the component's root element to prevent an implicit default layout from changing percentage resolution.

### 5. Box Model

**Default `box-sizing`**:

```css
/* Web default */
box-sizing: content-box;

/* Lynx default */
box-sizing: border-box;
```

> **Note**: Lynx uses `border-box` by default, so `width` and `height` include padding and borders. This differs fundamentally from the Web default, `content-box`, where `width` and `height` apply only to the content box.
>
> When migrating from the Web, you can preserve the original box-model behavior with a global reset:
>
> ```css
> * {
>   box-sizing: content-box;
> }
> ```

Explicitly setting `box-sizing: border-box` or `box-sizing: content-box` controls box-model calculations, but does not eliminate differences caused by other Web layout dependencies. When migrating a page that relies on `float`, the browser's default `<body>` margin or line height, the containing block for absolute positioning, or min/max-size clamping, you must also rewrite those layout conditions. Otherwise, pixel-level differences may remain even when the box model is correct.

**Margin collapsing is not supported**:

Vertical margins of adjacent elements do **not collapse** in Lynx; they are added together. This can produce unexpected spacing on pages migrated from the Web.

```css
/* Web: the vertical margins collapse to 20px. */
.item1 {
  margin-bottom: 20px;
}
.item2 {
  margin-top: 20px;
}

/* Lynx: the margins do not collapse, so the gap is 40px. */
```

**General solutions**:

| Scenario | Web behavior | Lynx behavior | Migration strategy |
| --- | --- | --- | --- |
| Adjacent siblings | `margin-bottom` and `margin-top` collapse to the larger value | The two margins are **added** | Set margin on only one element, or use `gap` or padding |
| Parent and first child | The parent's `margin-top` and child's `margin-top` collapse | The two margins are **added** | Remove the parent's margin and let the child control it |
| Parent and last child | The parent's `margin-bottom` and child's `margin-bottom` collapse | The two margins are **added** | Remove the parent's margin and let the child control it |
| Adjacent empty element | The empty element's margins collapse with other margins | The empty element's margins are **added normally** | Remove its margin or add `min-height: 1px` |

**Option 1: Set margin on only one element (for siblings)**

```css
.item1 { margin-bottom: 20px; }
.item2 { margin-top: 0; }
```

**Option 2: Replace one margin with padding**

```css
.item1 { margin-bottom: 0; padding-bottom: 20px; }
.item2 { margin-top: 0; }
```

**Option 3: Use `gap` when the parent uses Flex or Grid layout**

```css
.parent {
  display: flex;
  flex-direction: column;
  gap: 20px;
}
.item1, .item2 { margin: 0; }
```

**Option 4 (recommended): Remove the parent container's margin and let the child control it (for parent-child scenarios)**

To reproduce the Web result where a container has `margin-top: 8px`, its child has `margin-top: 1em`, and the collapsed result is `1em`:

```css
/* Original Web code */
.container {
  margin-top: 8px;
}
.child {
  margin-top: 1em;    /* 16px; collapsing with the container's 8px still produces 16px. */
}

/* Equivalent Lynx code */
.container {
  margin-top: 0;      /* Remove the container's top margin. */
}
.child {
  margin-top: 1em;    /* 16px; directly produces the target spacing. */
}
```

**Common scenarios**:

- Consecutive block-level elements use margins for spacing, such as paragraphs, headings, and list items.
- Spacing between a container and its first or last child, such as the body and its content or a card and its contents.
- Margins accumulate between outer and inner containers in nested components.
- Margins accumulate across multiple nesting levels.
- Empty elements, such as a `div` containing only a border, have margins.

**Negative margins in Linear layout**:

Negative margins work normally in Lynx's default `display: linear; linear-direction: column` layout:

| Axis | Scenario | Behavior |
| --- | --- | --- |
| **Cross axis** (`margin-left` / `margin-right`) | A negative child margin pulls the child back to the parent's boundary | ✅ **Works as expected** |
| **Main axis** (`margin-top` / `margin-bottom`) | A negative margin overlaps sibling elements | ✅ **Works as expected** |

```css
/* Pull a child back with a negative margin -- works. */
.parent {
  border-left: 2px solid red;
  padding-left: 96px;
}
.child {
  border-left: 2px solid black;
  margin-left: -98px;  /* Pull left to cover the parent's red border. */
}

/* Overlap siblings with a negative margin -- works. */
.sibling1 {
  border-top: 2px solid red;
  padding-bottom: 96px;
}
.sibling2 {
  border-bottom: 2px solid black;
  margin-top: -98px;   /* Move upward to meet sibling1's red border. */
}
```

**Migration guidance**:

- Negative margins work on both the cross axis and main axis in Lynx Linear layout and need no special workaround.
- If you see a small rendering difference, first verify any margin-collapsing compensation rather than blaming the negative margin itself.

**Negative padding values are invalid in CSS but accepted in Lynx**:

In CSS, negative padding values such as `padding-bottom: -1px` are **invalid**. When no other valid declaration exists, the property falls back to its initial value, `0`. Lynx's padding length parser (`LengthHandler::Process`) does **not** enforce non-negative values, so it parses a negative value as a valid length and passes it to layout. The layout path calls only `ClampIndefiniteToZero()`, which handles indefinite values rather than negative ones. As a result, the negative padding is applied directly and causes a layout difference.

```css
/* Web: -1px is invalid and falls back to 0. */
padding-bottom: -1px; /* Actual effect: 0 */

/* Lynx: -1px is parsed as a valid length and applied directly. */
padding-bottom: -1px; /* Actual effect: -1px */
```

**Workaround**: Replace negative padding with `0` during WPT migration or code review:

```css
/* Replace invalid negative padding with 0. */
.element {
  padding: 0; /* Instead of padding: -1px */
}
```

**Special handling for the `<body>` margin and the first child's `margin-top`**:

On the Web, `<body>` with `margin: 8px` and its first `<p>` with `margin-top: 1em` collapse to a final top spacing of `max(8px, 1em)`, approximately 16px.

In Lynx, `.body { margin: 8px; }` and the first child's `margin-top` do **not collapse**; they are added:

```css
/* Lynx migration strategy */
.body {
  display: linear;
  linear-direction: column;
  margin: 8px;
}

/* If the first child's margin-top is 0, total top spacing is only 8px. */
/* Add margin-top: 8px to the first child to compensate. */
.first-child {
  margin-top: 8px;
}
/* Total top spacing = 8px (.body) + 8px (.first-child) = 16px. */
```

### 6. Length Values

**Intrinsic sizing keywords**:

- ✅ `max-content` - Fully supported
- ⚠️ `fit-content` - Works in normal flow. For an element with `position: absolute` and `top: 0; bottom: 0`, or `left: 0; right: 0`, `max-height` or `max-width` does not constrain the size. Use an explicit `height` or `width`.

  ```css
  /* ❌ Lynx: max-height: fit-content has no effect;
   * the element stretches to the containing block's height. */
  .abs-box {
    position: absolute;
    top: 0;
    bottom: 0;
    max-height: fit-content;
  }

  /* ✅ Use an explicit height. */
  .abs-box {
    position: absolute;
    top: 0;
    bottom: 0;
    height: 100px;
  }
  ```

- ❌ `min-content` - Not supported

**Unsupported units**:

- ❌ `ch`, `ex`
- ❌ Physical units: `cm`, `mm`, `in`, `pt`, `pc`

#### Support for `calc()`

**✅ Supported properties (34 length properties)**:

- Sizing: `width`, `height`, `max-width`, `min-width`, `max-height`, `min-height`
- Positioning: `top`, `left`, `right`, `bottom`
- Padding: `padding-left`, `padding-right`, `padding-top`, `padding-bottom`
- Margins: `margin-left`, `margin-right`, `margin-top`, `margin-bottom`
- Flexbox: `flex-basis`
- Flex/Grid gaps: `grid-column-gap`, `grid-row-gap`, `column-gap`, `row-gap`
- Text: `font-size`, `text-indent`
- Transforms: `perspective`
- RTL support: `margin-inline-start/end`, `padding-inline-start/end`, `inset-inline-start/end`

**❌ Unsupported properties**:

- Color properties such as `color` and `background-color`
- Enumerated properties such as `flex-direction`, `justify-content`, and `align-items`
- Border radii such as `border-radius`
- Transform properties such as `transform`
- Numeric properties such as `opacity` and `z-index`

```css
/* ✅ Valid */
width: calc(100% - 20px);
margin: calc(var(--spacing) * 2);

/* ❌ Invalid */
flex-direction: calc(row); /* Enumerated property */
color: calc(#ff0000); /* Color property */
```

**Recommended units**:

- ✅ `px` - Pixels
- ✅ `%` - Percentages
- ✅ `vw`, `vh` - Viewport units; **recommended** when used with `rem`
- ✅ `rem` - Multiples of the root font size; **recommended** for responsive design
- ⚠️ `rpx` - Lynx-specific responsive pixels; fully functional but not Web-compatible

### 7. Positioning

**`static` is not supported**:

```css
/* Web */
position: static; /* Default */

/* Lynx: use relative instead. */
position: relative; /* Default */
```

**`position: fixed` requires explicit `left` and `top` values**:

On the Web, a fixed-positioned element with `left: auto` or `top: auto` is placed at its **static position**, its original position in normal flow. Lynx does not support static positioning, so `auto` for `left` or `top` is equivalent to **`left: 0; top: 0`**, the top-left corner of the viewport.

```css
/* ❌ Lynx: omitting left/top places the element at the viewport's top-left corner. */
#fixed-box {
  position: fixed;
  /* Actual effect: left: 0; top: 0 */
}

/* ✅ Lynx: always specify left and top explicitly. */
#fixed-box {
  position: fixed;
  left: 20px;
  top: 20px;
}
```

### 8. Floats Are Not Supported

```css
/* ❌ Not supported */
float: left;
float: right;
clear: both;

/* ✅ Use Flexbox instead. */
display: flex;
flex-direction: row;
```

### 9. Text Handling

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

### 10. Pseudo-elements

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

### 11. `z-index`

**Requires `position`**:

```css
/* Web: z-index can apply without position. */
z-index: 10;

/* Lynx: position must also be set. */
position: relative; /* Or absolute, fixed, or sticky */
z-index: 10;
```

**Stacking contexts and compositing layers**:

In Lynx, `z-index` creates a new **stacking context** and may promote an element to a **compositing layer**. As a result, child elements in scroll containers such as `scroll-view` and `scroll-coordinator` may fail to move with scrolling.

```css
/* ❌ Problem: children with z-index may not move with the scroll. */
scroll-coordinator-header {
  /* Children in the header set z-index. */
}
.header-item {
  position: relative;
  z-index: 10; /* This element may be promoted and fail to move with the header. */
}

/* ✅ Solution: establish the same stacking context on the parent with z-index: 0. */
scroll-coordinator-header {
  position: relative;
  z-index: 0; /* Make the header itself a compositing layer. */
}
.header-item {
  position: relative;
  z-index: 10; /* Now moves correctly with scroll-coordinator-header. */
}
```

**How it works**:

- When a child sets `z-index`, it is promoted to an independent compositing layer.
- That layer's coordinate transform may not include the parent container's scroll offset.
- Adding `z-index: 0`, or any `z-index`, to the parent creates a stacking context so the children remain in the same layer and move with scrolling.

**Common scenarios**:

- Elements with `z-index` inside `scroll-coordinator-header` do not move with scrolling.
- Fixed-positioned elements inside `scroll-view` interact incorrectly with scrollable content in the stacking order.
- Overlay popups interact incorrectly with scrollable content in the stacking order.

### 12. Selector Limitations

**Not supported**:

- ❌ Newer selectors such as `:is()`, `:where()`, and `:has()`
- ❌ Complex attribute selectors such as `[attr^="val"]` and `[attr$="val"]`; support is partial
- ❌ The general sibling combinator may be limited in complex cases

### 13. Custom Fonts

**Font formats**:

```css
/* Web: supports multiple formats. */
@font-face {
  src: url('font.woff2') format('woff2');
}

/* Lynx: use system fonts or inline base64 data. */
font-family: 'PingFang SC', 'Helvetica Neue', Arial, sans-serif;
```

### 14. Subset of 3D Transform Functions

Web CSS supports `scale3d(x, y, z)` and `rotate3d(x, y, z, angle)`, but the Lynx transform parser does not currently recognize either function name. A declaration that contains an unsupported transform function is parsed as invalid. If it appears in `@keyframes`, the corresponding `transform` keyframe is not added to the animation curve.

| Web syntax | Lynx status | Migration guidance |
| --- | --- | --- |
| `scale3d(x, y, z)` | ❌ Not supported | Use `scale(x, y)` when only XY scaling is required; use `matrix3d(...)` to express Z-axis scaling explicitly |
| `rotate3d(x, y, z, angle)` | ❌ Not supported | Use `rotateX(...)`, `rotateY(...)`, or `rotateZ(...)` for single-axis rotation; use `matrix3d(...)` for arbitrary-axis rotation |
| `translate3d(x, y, z)` | ✅ Supported | Use directly |
| `matrix3d(...)` | ✅ Supported | Use as an explicit compatibility form for complex 3D transforms |

```css
/* ❌ Web syntax: Lynx does not recognize scale3d or rotate3d. */
.card {
  transform: scale3d(0.6, 0.6, 1) rotate3d(0, 1, 0, 180deg);
}

/* ✅ Lynx: split the transform into supported functions. */
.card {
  transform: scale(0.6, 0.6) rotateY(180deg);
}
```

Take particular care when migrating animations:

```css
/* ❌ The transform keyframe fails to parse in Lynx. */
@keyframes zoom {
  0% { transform: scale3d(0.6, 0.6, 1); }
  100% { transform: scale3d(1, 1, 1); }
}

/* ✅ Use scale(x, y). */
@keyframes zoom {
  0% { transform: scale(0.6, 0.6); }
  100% { transform: scale(1, 1); }
}
```

### 15. The `steps(...)` Animation Timing Function

On the Web, the second argument to `steps(n)` may be omitted; the default is equivalent to `steps(n, jump-end)`. In Lynx, `steps(...)` requires an explicit step-position argument. Forms such as `steps(1)` and `steps(4)` are invalid.

| Web syntax | Lynx status | Lynx syntax |
| --- | --- | --- |
| `steps(4)` | ❌ Not supported | `steps(4, end)` or `steps(4, jump-end)` |
| `steps(4, start)` | ✅ Supported | Use directly |
| `steps(4, end)` | ✅ Supported | Use directly |
| `steps(4, jump-start)` | ✅ Supported | Use directly |
| `steps(4, jump-end)` | ✅ Supported | Use directly |
| `steps(4, jump-none)` | ✅ Supported | Use directly |
| `steps(4, jump-both)` | ✅ Supported | Use directly |
| `step-start` | ✅ Supported | Equivalent to `steps(1, start)` |
| `step-end` | ✅ Supported | Equivalent to `steps(1, end)` |

```css
/* ❌ Valid on the Web, invalid in Lynx. */
animation-timing-function: steps(4);

/* ✅ Lynx requires an explicit step position. */
animation-timing-function: steps(4, end);
```

As on the Web, `animation-timing-function` inside a keyframe applies to the interval from the current keyframe to the next, rather than to the current point itself:

```css
@keyframes fade-out-after-hold {
  0% {
    opacity: 1;
  }
  80% {
    opacity: 1;
    animation-timing-function: step-end;
  }
  100% {
    opacity: 0;
  }
}
```

Here, `step-end` controls the interval from `80%` to `100%`, so the opacity remains unchanged until it jumps at the end of the interval.

## Quick Migration Checklist

When migrating from the Web to Lynx, check the following:

- [ ] Replace `display: inline` and `display: inline-block`; prefer an explicit Lynx layout mode over the `display: block` compatibility value.
- [ ] Wrap all text in `<text>` elements.
- [ ] Remove every `float` and `clear`; use Flexbox instead.
- [ ] Replace `position: static` with `relative`.
- [ ] Replace `min-content` for intrinsic width or height; `flex-basis: min-content` degrades to `0px`, while `max-content` is supported.
- [ ] Add `position` wherever `z-index` is used.
- [ ] Check whether `z-index` in a scroll container such as `scroll-view` or `scroll-coordinator` prevents an element from moving with scrolling.
- [ ] If so, add `z-index: 0` to the parent to establish the same stacking context.
- [ ] Remove every use of `::before` and `::after`; they are not supported.
- [ ] Check for margin-collapsing dependencies.
- [ ] Replace Web `scale3d()` and `rotate3d()` calls with Lynx-supported `scale()`, `rotateX/Y/Z()`, or `matrix3d()` calls.
- [ ] Add the step-position argument to `steps(n)`, for example `steps(n, end)`.
- [ ] Test every `white-space` setting.

## Common Pitfalls

### Pitfall 1: Omitting the `text` Element

```jsx
// ❌ Incorrect
<view>Text content</view>

// ✅ Correct
<view>
  <text>Text content</text>
</view>
```

### Pitfall 2: Relying on Margin Collapsing

```css
/* Web: the margins collapse to 20px. */
.top {
  margin-bottom: 20px;
}
.bottom {
  margin-top: 20px;
}

/* Lynx: the margins do not collapse, so this produces 40px; adjust them. */
.top {
  margin-bottom: 10px;
}
.bottom {
  margin-top: 10px;
}
/* Or: */
.top {
  margin-bottom: 20px;
}
.bottom {
  margin-top: 0;
}
```

### Pitfall 3: Using Unsupported or Partially Supported Units

```css
/* ❌ Not supported */
width: 1in;

/* ⚠️ Partially supported */
width: 100vmin;

/* ✅ Use an alternative. */
width: auto;
width: 100%;
```

### Pitfall 4: Omitting `white-space`

```css
/* Text does not wrap by default. */
text {
  white-space: nowrap;
}

/* Explicitly enable wrapping when required. */
text {
  white-space: normal;
}
```

### Pitfall 5: Failing to Copy `body` Styles to the `page` Element

Web user-agent style sheets commonly give `body` a default margin, while authored styles may add padding or replace that margin. The Lynx root element, `page`, has neither margin nor padding by default.

```css
/* Web: a common user-agent default is margin: 8px; padding is authored here. */
body {
  margin: 8px;
  padding: 0 8px;
}

/* Lynx: reproduce the computed or authored body spacing explicitly. */
page {
  margin: 8px;
  padding: 0 8px;
}
```

To preserve the layout during migration, inspect the Web `body` element's computed and authored styles, then reproduce the relevant spacing on `page` rather than assuming a fixed browser default.

### Pitfall 6: Mapping Web `html` and `body` Backgrounds to Ordinary Containers

On the Web, `html` is the document root and `body` is the page-content container. They follow special canvas background-propagation rules. Lynx has no equivalent elements with those names. `page` is both the page root and the surface for the full-page background.

```css
/* Web: html provides the page color; body provides the content-area background. */
html {
  background-color: #f5f5f5;
}

body {
  margin: 8px;
  background-color: white;
}
```

When migrating to Lynx, first determine the purpose of each Web background:

- Set `page` when a full-page background is required.
- Set the root `view` when a content-container background is required.
- When the Web version uses separate backgrounds for both `html` and `body`, normally map the `html` background to `page` and the `body` background to the root `view`.

```css
/* Lynx */
page {
  background-color: #f5f5f5;
}

.root {
  margin: 8px;
  background-color: white;
}
```

If the Web page relies only on the `body` background filling the viewport, setting `page` directly in Lynx produces the closest full-page result:

```css
/* Web */
body {
  background-color: green;
}

/* Lynx */
page {
  background-color: green;
}
```

Do not treat an ordinary `view` named `.body` as equivalent to the Web `body`. An ordinary `view` paints its background only within its own layout bounds; `page` is the Lynx root and full-page background surface.

### Pitfall 7: Migrating a `background` Shorthand That Contains `background-attachment`

The Web `background` shorthand may include the `background-attachment` slot with values such as `fixed` or `scroll`. The current supported-property list does not include `background-attachment`. When migrating, expand the shorthand into the background properties Lynx supports and separately determine whether the attachment behavior must be preserved:

```css
/* Web */
.box {
  background: url('bg.png') 0 0 / cover no-repeat fixed #fff;
}
```

```css
/* Lynx */
.box {
  background-image: url('bg.png');
  background-position: 0 0;
  background-size: cover;
  background-repeat: no-repeat;
  background-color: #fff;
}
```

```css
/* For a solid background, use background-color directly. */
.box {
  background-color: green;
}
```
