# Flexbox Layout

Lynx supports commonly used CSS Flexbox capabilities and generally follows the Flexbox flexible sizing model. When migrating a Web layout, pay attention to differences in the default layout system, intrinsic sizing, and default property values.

## Enable Flexbox

```css
.container {
  display: flex;
}
```

## Common Layout Patterns

### Horizontal Centering

```css
.center-horizontal {
  display: flex;
  flex-direction: row;
  justify-content: center; /* Center along the main axis */
}
```

### Vertical Centering

```css
.center-vertical {
  display: flex;
  flex-direction: column;
  justify-content: center; /* Center along the main axis */
}
```

### Centering on Both Axes

```css
.center-both {
  display: flex;
  justify-content: center; /* Center along the main axis */
  align-items: center; /* Center along the cross axis */
}
```

```jsx
<view className="center-both">
  <text>Centered Content</text>
</view>
```

### Space Between Items

```css
.space-between {
  display: flex;
  flex-direction: row;
  justify-content: space-between;
}
```

```jsx
<view className="space-between">
  <text>Left</text>
  <text>Right</text>
</view>
```

### Equal-Width Items

```css
.equal-width {
  display: flex;
}

.equal-width .item {
  flex: 1; /* Give each item an equal share of the space */
}
```

### Fixed Sidebar and Flexible Main Content

```css
.layout {
  display: flex;
}

.sidebar {
  width: 200px; /* Fixed width */
}

.main {
  flex: 1; /* Take the remaining space */
}
```

```jsx
<view className="layout">
  <view className="sidebar">
    <text>Sidebar</text>
  </view>
  <view className="main">
    <text>Main Content</text>
  </view>
</view>
```

### Footer Button at the Bottom

```css
.page {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
}

.content {
  flex: 1; /* Let the content area take the remaining space */
}

.footer {
  /* Stays at the bottom */
}
```

## Flex Wrapping

```css
.wrap-container {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
}

.wrap-container .item {
  width: calc(50% - 8px); /* Two-column layout */
}
```

## Multi-Line Alignment

```css
.multi-line {
  display: flex;
  flex-wrap: wrap;
  align-content: space-between; /* Alignment between flex lines */
  gap: 16px;
}
```

## Property Reference

### `flex-direction`

| Value            | Description                            |
| ---------------- | -------------------------------------- |
| `row`            | Arranges items horizontally (default) |
| `column`         | Arranges items vertically             |
| `row-reverse`    | Arranges items horizontally in reverse order |
| `column-reverse` | Arranges items vertically in reverse order   |

### `flex-wrap`

| Value          | Description                         |
| -------------- | ----------------------------------- |
| `nowrap`       | Places all items on one line (default) |
| `wrap`         | Wraps items onto multiple lines     |
| `wrap-reverse` | Wraps items in the reverse direction |

### `flex-flow`

A shorthand for `flex-direction` and `flex-wrap`.

```css
.container {
  flex-flow: row wrap; /* Equivalent to flex-direction: row; flex-wrap: wrap; */
}
```

### `justify-content`

| Value           | Description                                      |
| --------------- | ------------------------------------------------ |
| `flex-start`    | Aligns items to the start                        |
| `center`        | Centers items                                    |
| `flex-end`      | Aligns items to the end                          |
| `space-between` | Places the first and last items at opposite ends and distributes space evenly between items |
| `space-around`  | Distributes space evenly around items            |
| `space-evenly`  | Distributes space evenly between and around items |
| `stretch`       | Stretches items (default in Lynx 2.1+)           |

### `align-items`

| Value        | Description                              |
| ------------ | ---------------------------------------- |
| `stretch`    | Stretches items to fill the cross axis (default) |
| `flex-start` | Aligns items to the cross-axis start     |
| `center`     | Centers items on the cross axis          |
| `flex-end`   | Aligns items to the cross-axis end       |
| `baseline`   | Aligns items by their baselines          |

### `align-self`

Controls the alignment of an individual flex item and overrides the flex container's `align-items` value.

| Value        | Description                                      |
| ------------ | ------------------------------------------------ |
| `auto`       | Uses the parent container's alignment (default) |
| `stretch`    | Stretches the item to fill the cross axis       |
| `flex-start` | Aligns the item to the cross-axis start         |
| `center`     | Centers the item on the cross axis              |
| `flex-end`   | Aligns the item to the cross-axis end           |
| `baseline`   | Aligns the item by its baseline                 |

### `align-content`

Controls the alignment of multiple flex lines and requires `flex-wrap: wrap`.

| Value           | Description                              |
| --------------- | ---------------------------------------- |
| `stretch`       | Stretches lines to fill the container (default) |
| `flex-start`    | Aligns lines to the start                |
| `center`        | Centers lines                            |
| `flex-end`      | Aligns lines to the end                  |
| `space-between` | Distributes space evenly between lines   |
| `space-around`  | Distributes space evenly around lines    |

### `flex`

A shorthand for `flex-grow`, `flex-shrink`, and `flex-basis`.

```css
.item {
  flex: 1; /* Equivalent to flex: 1 1 0% */
  flex: auto; /* Equivalent to flex: 1 1 auto */
  flex: none; /* Equivalent to flex: 0 0 auto */
}
```

### `flex-grow`

The flex grow factor of an item. The default is `0`, which prevents the item from growing.

### `flex-shrink`

The flex shrink factor of an item. The default is `1`, which allows the item to shrink.

`flex-shrink` only takes effect when there is insufficient available space along the main axis. With `flex-direction: row`, the relevant constraint is the container's width; with `flex-direction: column`, it is the container's height. If the parent flex container's main-axis size is not constrained by `width` or `height`, `max-width` or `max-height`, an outer container, or the viewport, the combined size of its children does not produce negative free space, so shrinking is not triggered. Omitting an explicit main-axis size usually avoids shrinking, but constraints from an outer container, the viewport, or another layout system can still trigger `flex-shrink`.

The shrink amount is not distributed solely according to the numeric `flex-shrink` values. Lynx's current Flexbox implementation distributes negative free space in proportion to each item's scaled flex shrink factor:

```text
scaled shrink factor = flex-shrink * flex base size
```

The `flex base size` usually comes from `flex-basis`. When `flex-basis: auto`, Lynx uses the main-axis `width` or `height`, or the measured content size. Consequently, among items with the same `flex-shrink` value, an item with a larger flex base size absorbs more of the shrinkage.

For example, suppose the parent flex container is `500px` wide and two children have a combined flex base width of `600px`, producing `100px` of negative free space.

```css
.item-a {
  width: 200px;
  flex-shrink: 1;
}

.item-b {
  width: 400px;
  flex-shrink: 2;
}
```

The scaled flex shrink factor of `item-a` is `200 * 1 = 200`, while that of `item-b` is `400 * 2 = 800`. As a result, `item-a` shrinks by `20px` to `180px`, and `item-b` shrinks by `80px` to `320px`.

When multiple items have the same flex base size, the result may appear to be proportional only to their `flex-shrink` values. Once their flex base sizes differ, those sizes also affect the distribution.

### `flex-basis`

The initial main size of an item before free space is distributed. The default is `auto`.

```css
.item {
  flex-basis: 100px; /* Fixed size */
  flex-basis: auto; /* Size based on content */
}
```

### `order`

Controls the order of flex items. The default is `0`; items with lower values are placed earlier.

```css
.item--first {
  order: -1; /* Place this item first */
}
```

### `gap` / `row-gap` / `column-gap`

Sets the spacing between items.

```css
.container {
  gap: 16px; /* 16px between both rows and columns */
  row-gap: 8px; /* 8px between rows */
  column-gap: 12px; /* 12px between columns */
}
```

## Notes

1. **`flex-basis: min-content` is not supported as an intrinsic flex basis:** it is treated as `0px`.

   ```css
   /* Unsupported */
   .item {
     flex-basis: min-content;
   }

   /* Alternative */
   .item {
     flex-basis: auto;
   }
   ```

2. **`<text>` elements:** If a flex item is a `<text>` element, you may need to set `white-space: nowrap` to prevent wrapping.

   ```css
   .flex-item-text {
     white-space: nowrap;
   }
   ```

3. **Content does not create a Web-style automatic minimum size:** Lynx `flex-shrink` does not apply the `min-content` or automatic minimum size behavior of Web flex items. When space is insufficient, a flex item containing text, an image, or nested children may continue to shrink along the main axis. On the Web, the item's content may establish a default minimum width or height that prevents further shrinking. You can think of Lynx's default behavior as equivalent to explicitly setting the Web flex item's minimum main size to `0`. Set `flex-shrink: 0`, or explicitly set the main-axis `min-width` or `min-height`, for items that must preserve a fixed size or protect their content. To make a Web preview match Lynx's shrinking behavior, you usually need `min-width: 0` in the row direction or `min-height: 0` in the column direction on the Web.

4. **Default `justify-content` value:** In Lynx, the default value of `justify-content` is `stretch`; on the Web, it is `flex-start`.
