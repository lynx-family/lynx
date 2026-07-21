# Migrating from the Web to Lynx

A complete guide to migrating an existing Web project to Lynx.

## Migration Overview

### File Structure Changes

**Web project**:

```
src/
├── components/
│   ├── Button.js
│   └── Card.js
├── pages/
│   ├── Home.js
│   └── About.js
├── styles/
│   ├── global.css
│   └── variables.css
└── index.html
```

**Lynx project**:

```
src/
├── components/
│   ├── Button.jsx
│   └── Card.jsx
├── pages/
│   ├── Home.jsx
│   └── About.jsx
├── styles/
│   ├── global.scss
│   └── variables.scss
└── App.jsx
```

### Main Changes

1. **File extension**: `.js` → `.jsx`
2. **HTML tags** → **Lynx elements**: `div` → `view`, `span/p` → `text`, and `img` → `image`
3. **CSS files**: `.css` → `.scss` (recommended)
4. **Event handling**: `onClick` → `bindtap`
5. **Routing**: Use the Lynx navigation API

## Mapping Web Root Containers

### Mapping `html` and `body` to Lynx `<page>`

On the Web, `html` and `body` form the page's root containers and provide these default responsibilities:

- Serving as the document root
- Serving as the default page scroll container, commonly called body scrolling
- Providing the viewport reference for `position: fixed`
- Providing the viewport-sized canvas established by `html, body { height: 100% }`

In Lynx, `<page>` assumes these responsibilities:

- `<page>` is the root of the page DOM and layout tree, and a framework may generate it implicitly.
- `<page>` is **not** itself a scroll container; its default is `overflow: hidden`.
- To reproduce a long Web page that uses body scrolling, normally add an explicit `<scroll-view>` inside `<page>`.

### Considerations When Migrating Scrolling

A typical migration of a Web page that uses ordinary body scrolling looks like this:

```html
<!-- Web: ordinary body scrolling -->
<body>
  <header>Header</header>
  <main><!-- Long content --></main>
</body>
```

```jsx
// Lynx: equivalent implementation
<page>
  <scroll-view scroll-orientation="vertical" style={{ height: '100%' }}>
    <view className="header">Header</view>
    <view className="main">{/* Long content */}</view>
  </scroll-view>
</page>
```

**Key difference**: After introducing `<scroll-view>`, the containing block for `position: fixed` may change from the viewport to the `<scroll-view>` scroll context. Stacking order may also be affected by the stacking context inside `<scroll-view>`. Pay particular attention to fixed-positioned elements and overlays during migration.

## Step 1: Convert HTML Tags

### Basic Tag Mapping

| Web                  | Lynx                     |
| -------------------- | ------------------------ |
| `div`                | `view`                   |
| `span`, `p`, `h1-h6` | `text`                   |
| `img`                | `image`                  |
| `input`              | `input`                  |
| `button`             | `view` + event handler   |
| `a`                  | `text` + event handler   |
| `ul/ol/li`           | `list` + `list-item`     |
| `form`               | Handle manually          |
| `table`              | Use Grid or Flex layout  |

### Conversion Example

**Web code**:

```jsx
function Card({ title, description, image }) {
  return (
    <div className="card">
      <img src={image} alt={title} />
      <div className="content">
        <h3>{title}</h3>
        <p>{description}</p>
      </div>
    </div>
  );
}
```

**Lynx code**:

```jsx
function Card({ title, description, image }) {
  return (
    <view className="card">
      <image src={image} />
      <view className="content">
        <text className="title">{title}</text>
        <text className="description">{description}</text>
      </view>
    </view>
  );
}
```

### Migrating Table Layouts

Lynx does **not support** the `<table>`, `<tr>`, or `<td>` elements, or table-related properties such as `display: table*`, `border-spacing`, and `border-collapse`. Rewrite table structures with Grid or Flex layout.

**Web code for a simple table**:

```html
<table>
  <tr>
    <td>Cell 1</td>
    <td>Cell 2</td>
    <td>Cell 3</td>
  </tr>
  <tr>
    <td>Cell 4</td>
    <td>Cell 5</td>
    <td>Cell 6</td>
  </tr>
</table>
```

```css
table { border-spacing: 2px; }
td { border: 1px solid; padding: 8px; }
```

**Lynx code using Grid**:

```jsx
<view className="grid-table">
  <view className="cell"><text>Cell 1</text></view>
  <view className="cell"><text>Cell 2</text></view>
  <view className="cell"><text>Cell 3</text></view>
  <view className="cell"><text>Cell 4</text></view>
  <view className="cell"><text>Cell 5</text></view>
  <view className="cell"><text>Cell 6</text></view>
</view>
```

```css
.grid-table {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 2px;  /* Replaces border-spacing. */
}
.cell {
  border: 1px solid;
  padding: 8px;
}
```

**Lynx code using Flexbox for a single row with multiple columns**:

```jsx
<view className="flex-table">
  <view className="cell"><text>Cell 1</text></view>
  <view className="cell"><text>Cell 2</text></view>
  <view className="cell"><text>Cell 3</text></view>
</view>
```

```css
.flex-table {
  display: flex;
  flex-direction: row;
  gap: 2px;
}
.cell {
  width: 10px;
  height: 10px;
  border: 1px solid;
}
```

### Text Handling

**Web**: Text can appear directly inside a container.

```jsx
<div>Hello World</div>
```

**Lynx**: Use the `text` element.

```jsx
<view>
  <text>Hello World</text>
</view>
```

## Step 2: Convert CSS

### 1. The `display` Property and Default Layout

**Key difference**: Lynx uses Linear layout by default, rather than Web block flow layout.

```css
/* Web: block flow is the default. A block box with width: auto fills the available inline size. */
div { /* Default display: block; width: auto */ }

/* Lynx: Linear Layout is the default; children stretch on the cross axis by default. */
view { /* Default: display: linear; linear-direction: column */ }
```

**Remove**:

```css
/* Remove these declarations. */
display: inline;
display: inline-block;
```

**Prefer one of these explicit layout modes**:

```css
display: flex;
display: grid;
display: linear; /* Lynx-specific */
display: relative; /* Lynx-specific */
```

Lynx 2.0+ accepts `display: block` as a compatibility value, but it resolves to a Lynx layout mode rather than Web block layout. Prefer an explicit layout mode when authoring new code.

**Sizing nested elements**:

When the original Web CSS uses `div { width: X; height: Y; }` to size all nested elements, make sure the migrated selector matches the nested Lynx `view` elements.

```css
/* Web: every nested div fills the specified dimensions. */
div { width: 5em; height: 1em; }

/* Lynx: set the dimensions explicitly or enable stretching. */
view { width: 5em; height: 1em; }
/* Or: */
.nested-view {
  width: 100%;
  height: 100%;
}
/* Or: */
.parent {
  display: linear;
  linear-cross-gravity: stretch;
}
```

### 2. Remove Floats

**Web**:

```css
.left {
  float: left;
}
.right {
  float: right;
}
.clear {
  clear: both;
}
```

**Lynx**:

```css
.container {
  display: flex;
  flex-direction: row;
}

.left {
  /* Participates in normal flow. */
}
.right {
  margin-left: auto;
}
```

### 3. Convert Units and Sizing Keywords

**Distinguish between these cases**:

```css
/* Lynx supports max-content and fit-content; keep them as written. */
width: max-content;
width: fit-content;

/* min-content is not supported for width or height. */
width: min-content;

/* In flex-basis, min-content degrades to 0px. */
flex-basis: min-content;
```

**Use alternatives for unsupported Web values**:

```css
width: auto;
width: 100%;
width: 200px;
/* Or use rem, which is recommended. */
page {
  font-size: calc(100vw / 23.4375);
}
width: 23.4rem;
```

### 4. Adjust Positioning

**Web**:

```css
.element {
  /* Default: position: static */
  z-index: 10;
}
```

**Lynx**:

```css
.element {
  position: relative; /* Must be set explicitly. */
  z-index: 10;
}
```

### 5. Enable Text Wrapping

**Web**: Text wraps by default.

```css
p {
  white-space: normal; /* Initial behavior on the Web. */
}
```

**Lynx**: Text does not wrap by default.

```css
text {
  white-space: normal; /* Set explicitly to enable wrapping. */
}
```

### 6. Account for Margin Collapsing

**Web**: Margins collapse automatically.

```css
.top {
  margin-bottom: 20px;
}
.bottom {
  margin-top: 20px;
}
/* Actual gap: 20px */
```

**Lynx**: Margins do not collapse.

```css
.top {
  margin-bottom: 10px;
}
.bottom {
  margin-top: 10px;
}
/* Actual gap: 20px after adjustment */
```

**Additional compensation for nested containers**:

The difference between Web and Lynx margin behavior is more apparent in nested structures. For example, suppose an outer container has `margin: 8px` and its first child container has `margin-top: 16px`:

```css
/* Web: margin collapsing produces a final gap of 16px. */
.body {
  margin: 8px;
}
.content-wrapper {
  margin-top: 16px;  /* Collapses with the outer 8px, producing 16px. */
}

/* Lynx: adding the margins produces a final gap of 24px. */
.body {
  margin: 8px;
}
.content-wrapper {
  margin-top: 16px;  /* 8px + 16px = 24px */
}
```

**Migration options**:

```html
<!-- Migration example: outer container and its first child container -->
<view class="body">
  <view class="content-wrapper"></view>
</view>
```

```css
/* Option 1: adjust the inner margin so the sum matches the expected value. */
.content-wrapper {
  margin-top: 8px;   /* For a total of 16px: 8px + 8px = 16px */
}

/* Option 2: replace the margin with padding. */
.body {
  margin: 0;
  padding: 8px;
}
.content-wrapper {
  margin-top: 0;
}

/* Option 3: use Flexbox and gap on the parent. */
.body {
  display: flex;
  flex-direction: column;
  gap: 16px;
  margin: 8px;
}
```

**Common scenarios**:

- When both outer and inner containers have margins, the total spacing is greater than expected.
- Margins accumulate between inner and outer containers at component boundaries.
- Margins accumulate across multiple nesting levels.

## Step 3: Convert Event Handling

### Tap Events

**Web**:

```jsx
<button onClick={handleClick}>Click me</button>
```

**Lynx**:

```jsx
<view bindtap={handleClick}>
  <text>Click me</text>
</view>
```

### Event Name Mapping

| Web            | Lynx             |
| -------------- | ---------------- |
| `onClick`      | `bindtap`        |
| `onTouchStart` | `bindtouchstart` |
| `onTouchMove`  | `bindtouchmove`  |
| `onTouchEnd`   | `bindtouchend`   |
| `onChange`     | `bindchange`     |
| `onInput`      | `bindinput`      |
| `onFocus`      | `bindfocus`      |
| `onBlur`       | `bindblur`       |
| `onScroll`     | `bindscroll`     |

## Step 4: Convert Images

**Web**:

```jsx
<img src="image.png" alt="Description" />
```

**Lynx**:

```jsx
<image src="image.png" />
```

**Considerations**:

- A Lynx `image` must have an explicit size or a parent with a definite size.
- The `alt` attribute is not supported.
- Lazy loading is supported through `lazy-load={true}`.

## Step 5: Convert Lists

**Web**:

```jsx
<ul>
  {items.map((item) => (
    <li key={item.id}>{item.name}</li>
  ))}
</ul>
```

**Lynx**:

```jsx
<list className="item-list">
  {items.map((item) => (
    <list-item key={item.id} item-key={item.id}>
      <view className="item">
        <text>{item.name}</text>
      </view>
    </list-item>
  ))}
</list>
```

## Step 6: Convert Forms

**Web**:

```jsx
<form onSubmit={handleSubmit}>
  <input type="text" value={value} onChange={handleChange} />
  <button type="submit">Submit</button>
</form>
```

**Lynx**:

```jsx
<view className="form">
  <input type="text" value={value} bindinput={handleChange} />
  <view bindtap={handleSubmit} className="button">
    <text>Submit</text>
  </view>
</view>
```

## Step 7: Organize Styles

### CSS Custom Properties (Fully Supported)

Lynx **fully supports** CSS custom properties through `var()`. Define them on `:root` or inline.

**Defining custom properties**:

```css
:root {
  --primary-color: #ff351a;
  --secondary-color: #666;
  --spacing-unit: 8px;
  --border-radius: 4px;
}
```

**Using custom properties**:

```css
.button {
  background-color: var(--primary-color);
  padding: var(--spacing-unit);
  border-radius: var(--border-radius);
}

.card {
  margin: calc(var(--spacing-unit) * 2);
  color: var(--secondary-color);
}
```

**Considerations**:

- ✅ Custom properties can be referenced from any CSS property, including inside `calc()` expressions.
- ✅ Fallback values are supported: `var(--undefined, fallback-value)`.
- ✅ The cascade and scoped overrides are supported.
- ✅ Behavior matches the Web standard.

### Using CSS Custom Properties: SCSS Example

**Web**:

```css
:root {
  --primary-color: #ff351a;
}
```

**Lynx**:

```css
/* Same syntax */
:root {
  --primary-color: #ff351a;
}
```

### Using SCSS

SCSS is recommended in Lynx projects for improved style organization.

```scss
// variables.scss
$primary-color: #ff351a;
$spacing-unit: 8px;

// component.scss
@import './variables.scss';

.component {
  color: $primary-color;
  padding: $spacing-unit * 2;
}
```

## Migration Checklist

- [ ] Replace HTML tags with Lynx elements.
- [ ] Wrap all text in `<text>` elements.
- [ ] Remove `float` and `clear`.
- [ ] Remove `display: inline` and `display: inline-block`; Lynx 2.0+ retains `display: block` and falls back to Flex or Linear layout.
- [ ] Replace `position: static` with `relative`.
- [ ] Keep `max-content` and `fit-content`; replace `min-content` for width or height, and note that `flex-basis: min-content` degrades to `0px`.
- [ ] Add `position` wherever `z-index` is used.
- [ ] Check whether `z-index` in a scroll container prevents elements from moving with scrolling.
- [ ] Verify compatibility of CSS custom properties through `var()`; Lynx fully supports them.
- [ ] Remove every use of `::before` and `::after`; they are not supported.
- [ ] Check for margin-collapsing dependencies.
- [ ] Set `white-space: normal` to enable text wrapping.
- [ ] Convert event-handler names.
- [ ] Convert image elements.
- [ ] Convert list elements.
- [ ] Convert form elements.
- [ ] Test every interaction.
- [ ] Test responsive layouts.
- [ ] Optimize performance, including list virtualization where appropriate.
