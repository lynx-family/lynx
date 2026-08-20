# Lynx Front-End Development Best Practices

## Performance Optimization

### Layout Performance

#### 1. Choose the Appropriate Layout Type

```css
/* ✅ Use Linear for simple lists (best performance) */
.simple-list {
  /* Linear is the default, so display does not need to be set */
  padding: 16px;
}

/* ✅ Use Flex for complex flexible layouts */
.complex-layout {
  display: flex;
  flex-direction: row;
  justify-content: space-between;
}

/* ❌ Avoid Flex for simple layouts */
.simple-column {
  display: flex;
  flex-direction: column; /* Less efficient than using Linear directly */
}
```

#### 2. Reduce Layout Nesting

```jsx
// ❌ Too many nested layers
<view className="wrapper">
  <view className="container">
    <view className="inner">
      <view className="content">
        <text>Content</text>
      </view>
    </view>
  </view>
</view>

// ✅ Flattened structure
<view className="content">
  <text>Content</text>
</view>
```

#### 3. Avoid Changing the Layout Type Dynamically

```jsx
// ❌ Avoid switching display types at runtime
const [isGrid, setIsGrid] = useState(false);

<view style={{ display: isGrid ? 'grid' : 'flex' }}>
  {/* Content */}
</view>

// ✅ Use opacity or transform instead
<view className="flex-container">
  <view className="flex-view" style={{ opacity: isGrid ? 0 : 1 }} />
  <view className="grid-view" style={{ opacity: isGrid ? 1 : 0 }} />
</view>
```

### Style Calculation

#### 4. Prefer CSS Classes to Inline Styles

```jsx
// ❌ Inline styles are recalculated on every render
<view style={{
  width: 100,
  height: 200,
  backgroundColor: '#fff'
}} />

// ✅ CSS classes are parsed once
<view className="card" />
```

```css
.card {
  width: 100px;
  height: 200px;
  background-color: #fff;
}
```

#### 5. Avoid Complex CSS Selectors

```css
/* ❌ Complex selectors increase matching overhead */
.app .container > .wrapper .item .content {
  color: red;
}

/* ✅ Use a simple class selector */
.item-odd {
  color: red;
}
```

#### 6. GPU Acceleration

```css
/* ✅ Use transform and opacity for automatic GPU acceleration */
.animated-element {
  transition: transform 0.3s ease, opacity 0.3s ease;
}

/* ✅ Lynx manages cleanup after the animation; no manual cleanup is needed */
```

> **Note:** Lynx automatically enables GPU acceleration for `transform` and `opacity`; no explicit declaration is required.

### Rendering Performance

#### 7. Optimize Images

```css
/* ✅ Specify image dimensions to avoid layout shifts */
.product-image {
  width: 200px;
  height: 200px;
  /* Alternatively, set one dimension and preserve the aspect ratio */
  width: 100%;
  aspect-ratio: 16 / 9;
}
```

```jsx
// ✅ Use lazy loading
<list>
  {items.map((item) => (
    <list-item key={item.id}>
      <image src={item.image} lazy-load={true} placeholder="placeholder.png" />
    </list-item>
  ))}
</list>
```

#### 8. Optimize Lists

```jsx
// ✅ Use list instead of scroll-view with many view children
<list className="item-list" scroll-y>
  {items.map((item) => (
    <list-item key={item.id} item-key={item.id}>
      <view className="item">
        <text>{item.title}</text>
      </view>
    </list-item>
  ))}
</list>
```

```css
/* ✅ Give list-item a fixed height when possible */
list-item {
  height: 80px;
}
```

#### 9. Limit the Number of Simultaneously Rendered Elements

```jsx
// ❌ Rendering too many elements at once
{
  items.map((item) => <HeavyComponent key={item.id} data={item} />);
}

// ✅ Pagination or a virtualized list
<list>
  {visibleItems.map((item) => (
    <list-item key={item.id}>
      <HeavyComponent data={item} />
    </list-item>
  ))}
</list>;
```

## Layout Best Practices

### Flex Layout Patterns

#### 10. Responsive Layout

```css
/* Mobile/default */
.container {
  display: flex;
  flex-direction: column;
}

/* Note: Lynx does not support @media. Adjust with JavaScript or viewport units. */
/* JavaScript: if (viewportWidth >= 768) { setLayout('row') } */
```

#### 11. Flex Item Ratios

```css
/* ✅ Use the flex shorthand */
.item {
  flex: 1; /* flex-grow: 1, flex-shrink: 1, flex-basis: 0% */
}

.item-large {
  flex: 2; /* Takes twice as much available space */
}

/* ❌ Avoid setting the longhands separately */
.item {
  flex-grow: 1;
  flex-shrink: 1;
  /* Omitting flex-basis can produce unexpected behavior */
}
```

#### 12. Manage Spacing

```css
/* ✅ Use the gap property */
.grid {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
}

/* Traditional approach (requires manual margin handling) */
.grid-item {
  width: calc(33.33% - 11px);
  margin-right: 16px;
  margin-bottom: 16px;
}

.grid-item--row-end {
  margin-right: 0;
}
```

### Grid Layout Patterns

**Note:** Lynx does **not** support `@media` queries. See [Responsive Layout Patterns](patterns/responsive.md) for responsive layout strategies.

## Style Organization

### CSS Architecture

#### 15. BEM Naming Convention

```css
/* Block */
.card {
}

/* Element */
.card__title {
}
.card__content {
}
.card__button {
}

/* Modifier */
.card--featured {
}
.card__button--primary {
}
.card__button--disabled {
}
```

#### 16. CSS Custom Properties (Theming)

See `patterns/theming.md` for a complete set of theme custom properties (colors, spacing, typography, border radii, and shadows) and a dark-theme switching example. Follow these recommendations when managing themes with CSS custom properties:

- **Naming convention**: Use the `--category-property` format, such as `--color-primary`.
- **Separation of concerns**: Define colors, spacing, and typography separately.
- **Semantic names**: Prefer `--color-primary` to `--color-red`.
- **Fallback values**: Provide a fallback, for example `var(--prop, default)`.

#### 17. Isolate Component Styles

```css
/* Keep each component in a separate file */
/* Button.css */
.button {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 12px 24px;
}

/* Card.css */
.card {
  display: flex;
  flex-direction: column;
  padding: 16px;
}

/* Avoid leaking styles globally */
/* ❌ Not recommended */
view {
  box-sizing: border-box;
}

/* ✅ Use a class selector */
.reset-box {
  box-sizing: border-box;
}
```

## Responsive Design

### Responsive Strategies

#### 18. Use `rem` with `vw` (Recommended)

> **Recommended approach:** Combine `rem` with `vw` to set the root font size and scale the interface responsively.

```css
/* Set the reference font size on the root element */
page {
  font-size: calc(100vw / 23.4375); /* 1rem = 16px at a 375px viewport width */
}

/* Use rem for responsive scaling */
.container {
  width: 100%; /* Full width */
  padding: 2rem; /* 2rem on each side, approximately 32px at 375px */
}

.card {
  width: 21.4rem; /* Approximately the viewport width minus 32px at 375px */
  margin-bottom: 1.5rem;
}
```

**About `rpx`:** `rpx` is a fully supported Lynx-specific unit that scales automatically with the screen width, but it is not Web-compatible. Using `rem` with `vw` is a more standard cross-platform responsive approach.

#### 19. Account for Safe-Area Insets

```css
/* Account for an iPhone display cutout */
.safe-area {
  padding-top: env(safe-area-inset-top);
  padding-bottom: env(safe-area-inset-bottom);
}

/* Button fixed to the bottom edge */
.fixed-bottom {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  padding-bottom: env(safe-area-inset-bottom);
  background-color: #fff;
}
```

#### 20. Scale Typography

⚠️ **Note:** Lynx does **not** support `@media`. Use one of the following alternatives:

**Option 1: Use `rem` (recommended)**

```css
/* Set the reference font size on the root element */
page {
  font-size: calc(100vw / 23.4375); /* 1rem = 16px at a 375px viewport width */
}

/* Scale automatically with rem */
.title {
  font-size: 1.125rem; /* Approximately 18px at a 375px viewport width */
}

.body {
  font-size: 0.875rem; /* Approximately 14px at a 375px viewport width */
}
```

**Option 2: Use `vw` directly**

```css
/* Use vw directly */
.title {
  font-size: 4.8vw; /* Approximately 18px at a 375px viewport width */
}

.body {
  font-size: 3.73vw; /* Approximately 14px at a 375px viewport width */
}
```

**Option 3: Adjust dynamically with JavaScript**

```javascript
// Calculate the font size from the viewport width
const baseFontSize = viewportWidth >= 768 ? 18 : 16;
// Set the root element's font size
```

## Animations and Transitions

### Performance-Oriented Animations

#### 21. Use `transform` and `opacity`

```css
/* ✅ Performance-friendly properties */
.animated {
  transition: transform 0.3s ease, opacity 0.3s ease;
}

.animated:hover {
  transform: scale(1.1);
  opacity: 0.8;
}

/* ❌ Avoid animating these properties */
.animated-bad {
  transition: width 0.3s, height 0.3s, margin 0.3s;
}
```

#### 22. Hardware Acceleration

```css
/* Enable GPU acceleration */
.gpu-accelerated {
  transform: translateZ(0);
  /* Or */
  transform: translate3d(0, 0, 0);
}
```

#### 23. Keyframe Animations

```css
@keyframes slideIn {
  from {
    transform: translateX(100%);
    opacity: 0;
  }
  to {
    transform: translateX(0);
    opacity: 1;
  }
}

.slide-in {
  animation: slideIn 0.3s ease-out;
}
```

## Accessibility

#### 24. Provide Sufficiently Large Touch Targets

```css
/* ✅ Minimum 44x44 touch target */
.button {
  min-width: 44px;
  min-height: 44px;
  padding: 12px 24px;
}
```

#### 25. Color Contrast

```css
/* ✅ Ensure sufficient contrast */
.text-primary {
  color: #333333; /* 12.6:1 contrast ratio on a white background */
}

.text-secondary {
  color: #666666; /* 5.7:1 contrast ratio */
}

/* ❌ Insufficient contrast */
.text-weak {
  color: #cccccc; /* 1.9:1 contrast ratio; difficult to read */
}
```

## Debugging Tips

See `quick-reference.md` for CSS debugging snippets.

#### 27. Performance Monitoring

```jsx
// Measure rendering performance
import { useEffect, useRef } from '@lynx-js/react';

function PerformanceMonitor({ children }) {
  const startTime = useRef(performance.now());

  useEffect(() => {
    const endTime = performance.now();
    console.log(`Render time: ${endTime - startTime.current}ms`);
  });

  return children;
}
```

## Common Pitfalls and Solutions

### Pitfall 1: Text Does Not Wrap

```css
/* Problem: text does not wrap by default */
text {
  /* The default is white-space: nowrap */
}

/* Solution A: allow wrapping */
.text-wrap {
  white-space: normal;
}

/* Solution B: truncate a single line */
.text-ellipsis {
  max-width: 200px;
  white-space: nowrap;
  text-overflow: ellipsis;
  overflow: hidden;
}
```

### Pitfall 2: An Image Does Not Render

```css
/* Problem: an image must have dimensions */
image {
  /* It does not render without dimensions */
}

/* Solution A: responsive dimensions */
.responsive-image {
  width: 100%;
  height: auto;
  aspect-ratio: 16 / 9;
}

/* Solution B: fixed dimensions */
.fixed-image {
  width: 200px;
  height: 200px;
}
```

### Pitfall 3: A Flex Item Collapses

```css
/* Problem: a child is compressed to zero */
.flex-container {
  display: flex;
}

.flex-item {
  /* May be compressed */
}

/* Solution A: preserve the item's base size */
.flex-item--fixed {
  flex-shrink: 0; /* Prevent shrinking */
}

/* Solution B: allow the item and its content to shrink */
.flex-item--shrinkable {
  min-width: 0; /* Allow shrinking */
  overflow: hidden; /* Use with ellipsis */
}
```

### Pitfall 3 Addendum: Lynx and the Web Use Different Minimum Main Sizes

```css
/* When a row lacks width or a column lacks height, Lynx may continue shrinking the content item */
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

```css
/* Web preview: remove the automatic minimum size on the main axis to match Lynx more closely */
.row .content-item {
  min-width: 0;
}

.column .content-item {
  min-height: 0;
}

/* Lynx: explicitly prevent shrinking when the content item must retain its size */
.content-item {
  flex-shrink: 0;
  /* Or set an explicit min-width or min-height along the main axis */
}
```

### Pitfall 4: Sticky Positioning Does Not Work

```css
/* Problem: sticky positioning requires an appropriate containing parent */
.container {
  /* No height is set */
}

.sticky-element {
  position: sticky;
  top: 0;
}

/* Solution */
.container {
  height: 100vh; /* Or use a fixed height */
  overflow: scroll; /* The container must scroll */
}

.sticky-element {
  position: sticky;
  top: 0;
  z-index: 10; /* Keep it above the scrolling content */
}
```

### Pitfall 5: A z-indexed Child Does Not Scroll with Its Container

```css
/* Problem: a z-indexed child of the coordinator header may not follow scrolling */
scroll-coordinator-header .overlay {
  position: absolute;
  z-index: 100; /* A separate compositing layer may remain fixed in the viewport */
}

/* Solution: establish a stacking context on the parent container */
scroll-coordinator-header {
  position: relative;
  z-index: 0;
}

scroll-coordinator-header .overlay {
  position: absolute;
  z-index: 100; /* Now follows the coordinator header while it scrolls */
}
```

**Other Solutions**:

```css
/* Option 1: Remove unnecessary z-index */
.header-item {
  /* Omit z-index and control stacking order through document order */
}

/* Option 2: Move z-index up to the container */
scroll-coordinator-header {
  position: relative;
  z-index: 10; /* Control stacking order at the container level */
}

/* Option 3: On Android, place the header over the slot */
/* <scroll-coordinator android-header-over-slot={true}> */
```

### Pitfall 6: A Percentage Height Does Not Resolve

```css
/* Problem: a percentage height requires a definite parent height */
.parent {
  /* No height is set */
}

.child {
  height: 50%; /* Does not resolve */
}

/* Solution */
.parent {
  height: 400px; /* Or */
  display: flex;
  flex-direction: column;
}

.child {
  height: 50%;
  /* Or use Flex */
  flex: 1;
}
```

## Code Review Checklist

Before submitting code, verify the following:

- [ ] The layout uses the appropriate system (Linear, Flex, Grid, or Relative).
- [ ] Nesting is not excessive (no more than five levels is recommended).
- [ ] Images have explicit dimensions.
- [ ] Text content is placed in a text element.
- [ ] CSS classes are used instead of numerous inline styles.
- [ ] Animations use `transform` and `opacity`.
- [ ] Lists use the `list` element.
- [ ] Touch targets are sufficiently large (at least 44x44).
- [ ] Color contrast is sufficient.
- [ ] Performance has been tested on a physical device.
