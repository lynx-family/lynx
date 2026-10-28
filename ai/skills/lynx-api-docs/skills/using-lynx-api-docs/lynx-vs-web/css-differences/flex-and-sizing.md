# Flex and Sizing

## Flex Shrinking and Content-Based Minimum Sizes

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

## Cyclic Percentage Sizing

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

## Length Values

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
