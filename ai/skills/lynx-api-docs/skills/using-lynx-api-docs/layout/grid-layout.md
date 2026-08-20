# Grid Layout

Grid Layout is designed for two-dimensional layouts, especially when multiple elements must be arranged in both rows and columns or span multiple rows or columns. Lynx Grid Layout generally follows the Web CSS Grid model but supports only a subset of commonly used properties.

## Supported Features

```css
.container {
  display: grid;

  /* Explicitly define columns and rows */
  grid-template-columns: 200px 1fr 2fr;
  grid-template-columns: repeat(3, 1fr);
  grid-template-rows: 100px auto;

  /* Implicit tracks that are not explicitly defined */
  grid-auto-columns: 100px;
  grid-auto-rows: minmax(50px, auto);

  /* Gutters */
  gap: 10px;
  row-gap: 10px;
  column-gap: 10px;

  /* Automatic placement */
  grid-auto-flow: row; /* Alternatives: column, dense, row dense, column dense */
}

.item {
  /* Grid item placement */
  grid-column-start: 1;
  grid-column-end: 3;
  grid-row-start: 1;
  grid-row-end: 3;
}
```

When rows or columns are not explicitly defined, Grid uses `grid-auto-rows` and `grid-auto-columns` to size the implicit tracks.

## Common Layouts

### Three Equal-Width Columns

```css
.grid-3 {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16px;
}
```

```jsx
<view className="grid-3">
  <text className="item">1</text>
  <text className="item">2</text>
  <text className="item">3</text>
  <text className="item">4</text>
  <text className="item">5</text>
  <text className="item">6</text>
</view>
```

### Sidebar and Main Content

```css
.layout {
  display: grid;
  grid-template-columns: 200px 1fr;
  gap: 20px;
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

### Fixed Header and Footer

```css
.page {
  display: grid;
  grid-template-rows: 60px 1fr 60px;
  height: 100vh;
}

.header {
  /* First row */
}

.content {
  /* Middle content row */
}

.footer {
  /* Last row */
}
```

### Grid Item Placement

Use grid line numbers to specify the position of a grid item.

```css
.grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  grid-auto-rows: 80px;
  gap: 10px;
}

/* Large image occupies the area between column lines 1-3 and row lines 1-3 */
.grid-item-large {
  grid-column-start: 1;
  grid-column-end: 3;
  grid-row-start: 1;
  grid-row-end: 3;
}

/* Wide image occupies the area between column lines 1-5 and row lines 3-4 */
.grid-item-wide {
  grid-column-start: 1;
  grid-column-end: 5;
  grid-row-start: 3;
  grid-row-end: 4;
}
```

```jsx
<view className="grid">
  <text className="grid-item-large">Large</text>
  <text>Small 1</text>
  <text>Small 2</text>
  <text className="grid-item-wide">Wide</text>
</view>
```

## Property Reference

### `grid-template-columns` / `grid-template-rows`

Defines the column and row tracks of the explicit grid.

```css
.container {
  /* Fixed sizes */
  grid-template-columns: 100px 200px 100px;

  /* Fractional distribution */
  grid-template-columns: 1fr 2fr 1fr;

  /* Repeated patterns */
  grid-template-columns: repeat(3, 1fr);
  grid-template-columns: repeat(2, 100px 1fr);

  /* Mixed track sizes */
  grid-template-columns: 200px 1fr 1fr;
}
```

### `grid-auto-columns` / `grid-auto-rows`

Defines the size of implicitly created grid tracks.

```css
.container {
  grid-auto-columns: 100px;
  grid-auto-rows: minmax(50px, auto);
}
```

### `grid-auto-flow`

Controls how grid items without an explicit position are automatically placed.

| Value          | Description                                  |
| -------------- | -------------------------------------------- |
| `row`          | Fills each row in sequence (default)         |
| `column`       | Fills each column in sequence                |
| `dense`        | Uses dense packing to fill holes when possible |
| `row dense`    | Uses dense packing in row order              |
| `column dense` | Uses dense packing in column order           |

```css
.container {
  grid-auto-flow: row dense;
}
```

### `grid-column-start` / `grid-column-end`

Defines the start and end column lines of a grid item.

```css
.item {
  grid-column-start: 1; /* Start at column line 1 */
  grid-column-end: 3;   /* End at column line 3 and span two columns */
}
```

### `grid-row-start` / `grid-row-end`

Defines the start and end row lines of a grid item.

```css
.item {
  grid-row-start: 1;
  grid-row-end: 3; /* Span two rows */
}
```

### `grid-column` / `grid-row` (Optional Shorthands)

`grid-column` and `grid-row` are Grid placement shorthands. They are available only in Lynx 3.9+ when `enableGridPlacementShorthands: true` is enabled. For compatibility with the default configuration and older versions, the examples prefer `grid-column-start` and `grid-column-end`, and `grid-row-start` and `grid-row-end`.

```css
.item {
  grid-column: 1 / 3;
  grid-row: 1 / 3;
}
```

### `gap` / `row-gap` / `column-gap`

Defines the gutters between grid tracks.

```css
.container {
  gap: 16px;        /* 16px between both rows and columns */
  row-gap: 8px;     /* Space between rows */
  column-gap: 12px; /* Space between columns */
}
```

### `justify-content` / `align-content`

Aligns the entire set of grid tracks when they do not fill the grid container.

| Property          | Axis                              |
| ----------------- | --------------------------------- |
| `justify-content` | Inline axis, which is horizontal in Lynx |
| `align-content`   | Block axis, which is vertical in Lynx    |

`justify-content` supports `start`, `end`, `flex-start`, `flex-end`, `center`, `stretch`, `space-between`, `space-around`, and `space-evenly`.

`align-content` supports only `start`, `end`, `flex-start`, `flex-end`, `center`, `stretch`, `space-between`, and `space-around`.

```css
.container {
  justify-content: center;
  align-content: start;
}
```

### `justify-items` / `justify-self`

Defines how grid items are aligned within their grid areas along the inline axis, which is horizontal in Lynx. `justify-self` applies to one grid item and overrides the grid container's `justify-items` value.

| Value     | Description                                           |
| --------- | ----------------------------------------------------- |
| `stretch` | Stretches the item to fill the inline axis (default) |
| `start`   | Aligns the item with the start edge of its grid area |
| `center`  | Centers the item                                      |
| `end`     | Aligns the item with the end edge of its grid area   |
| `auto`    | For `justify-self` only; uses the container's alignment |

```css
.container {
  justify-items: center;
}

.special-item {
  justify-self: end;
}
```

### `align-items` / `align-self`

Defines how grid items are aligned within their grid areas along the block axis, which is vertical in Lynx. `align-self` applies to one grid item and overrides the grid container's `align-items` value.

| Value        | Description                                           |
| ------------ | ----------------------------------------------------- |
| `stretch`    | Stretches the item to fill the block axis (default)  |
| `start`      | Aligns the item with the start edge of its grid area |
| `center`     | Centers the item                                      |
| `end`        | Aligns the item with the end edge of its grid area   |
| `flex-start` | Aligns the item with the start edge; compatible with Flexbox and Linear alignment values |
| `flex-end`   | Aligns the item with the end edge; compatible with Flexbox and Linear alignment values |
| `auto`       | For `align-self` only; uses the container's alignment |

```css
.container {
  align-items: center;
}

.special-item {
  align-self: start;
}
```

## Limitations

The following features are **unsupported or not recommended for new code**:

- ❌ Named grid lines: `grid-template-columns: [start] 1fr [end]`
- ❌ `grid-area`
- ❌ `subgrid`

The following properties are available but **not recommended for new code**:

- `grid-column` / `grid-row`: Requires Lynx 3.9+ with `enableGridPlacementShorthands: true`; default examples prefer `grid-*-start/end`.
- `grid-column-span` / `grid-row-span`: Lynx-specific syntax; prefer `grid-*-start/end`.

⚠️ **Partial support:**

- `minmax()`: Basic usage is supported, but `min-content` is not.

```css
/* Basic usage is supported */
grid-template-columns: minmax(100px, 1fr) 2fr;
grid-auto-rows: minmax(50px, auto);
```

## Migrating from Table Layout

Lynx **does not support** `display: table*` or related properties such as `border-spacing`, `border-collapse`, and `table-layout`. Grid Layout is the preferred alternative to Table Layout.

### Simple Table to Grid

**Web:**
```html
<table>
  <tr><td>A</td><td>B</td><td>C</td></tr>
  <tr><td>D</td><td>E</td><td>F</td></tr>
</table>
```

```css
table { border-spacing: 2px; }
td { border: 1px solid; width: 50px; height: 30px; }
```

**Lynx:**
```jsx
<view className="grid-table">
  <view className="cell"><text>A</text></view>
  <view className="cell"><text>B</text></view>
  <view className="cell"><text>C</text></view>
  <view className="cell"><text>D</text></view>
  <view className="cell"><text>E</text></view>
  <view className="cell"><text>F</text></view>
</view>
```

```css
.grid-table {
  display: grid;
  grid-template-columns: repeat(3, 50px);
  gap: 2px; /* Replaces border-spacing */
}

.cell {
  border: 1px solid;
  height: 30px;
}
```

### Key Mappings

| Table Feature | Grid Alternative |
| ------------- | ---------------- |
| `<table>` | A container with `display: grid` |
| `<tr>` | Automatic row wrapping controlled by `grid-template-columns` |
| `<td>` | A child `<view>` element |
| `border-spacing` | `gap` |
| `border-collapse` | Remove child borders and simulate them with a container background and `gap` |
| Fixed column widths | `grid-template-columns: repeat(N, width)` |

### When Migration Cannot Preserve the Layout

If the Web layout's essential behavior depends on Table Layout itself, such as the vertical-alignment behavior of `display: table-cell` or the table automatic column-width algorithm, Lynx cannot reproduce the layout exactly.

## Responsive Grids

```css
/* Note: Lynx does not support @media. Use viewport units or update the layout dynamically with JavaScript. */

/* Mobile/default: one column */
.grid {
  display: grid;
  grid-template-columns: 1fr;
  gap: 16px;
}

/* Responsive alternative: use flex-wrap or update the column count dynamically with JavaScript */
.responsive-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
}

.responsive-grid > .item {
  flex: 1 1 40vw;
}
```
