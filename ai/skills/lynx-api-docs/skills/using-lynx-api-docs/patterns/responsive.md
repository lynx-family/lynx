# Responsive Design

Build responsive layouts in Lynx.

## Scale Automatically with `rem` and `vw` (Recommended)

> **Recommended approach:** Use `rem` together with `vw` to set the root font size. `rpx` is a fully supported Lynx-specific unit, but it is not Web-compatible.

```css
/* Set the base font size on the root element */
page {
  font-size: calc(100vw / 23.4375); /* 1rem = 16px at a 375px viewport width */
}

/* Use rem for responsive scaling */
.container {
  width: 100%; /* Full width */
  padding: 2rem; /* 2rem on each side, approximately 32px at 375px */
}

.card {
  width: 100%; /* Fill the container's content box */
  margin-bottom: 1.5rem;
}

/* Font sizes */
.title {
  font-size: 1.125rem; /* Approximately 18px at a 375px viewport width */
}

.body {
  font-size: 0.875rem; /* Approximately 14px at a 375px viewport width */
}
```

## Use Viewport Units (`vw` and `vh`)

> **Note:** Lynx does **not** support CSS `@media` queries. Use viewport units to create fluid layouts.

```css
/* Use vw instead of @media */
.container {
  display: flex;
  flex-direction: column;
  padding: 4vw; /* Scales with the viewport width */
}

/* Optimize for larger screens */
.sidebar {
  width: 26vw; /* Approximately 200px at a 768px viewport width */
}

.main {
  flex: 1;
}
```

## Responsive Grid

Combine Grid layout with percentages or `vw` units:

```css
.grid {
  display: grid;
  gap: 16px;
  grid-template-columns: 1fr; /* Mobile: one column */
}
```

## Responsive Flex Layout

```css
.flex-container {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
}

.flex-item {
  flex: 1 1 40vw; /* Scales with the viewport width */
}
```

## Safe-Area Insets

```css
/* Account for notched iPhone displays */
.safe-area {
  padding-top: env(safe-area-inset-top);
  padding-bottom: env(safe-area-inset-bottom);
  padding-left: env(safe-area-inset-left);
  padding-right: env(safe-area-inset-right);
}

/* Fixed bottom button */
.fixed-bottom {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  padding-bottom: env(safe-area-inset-bottom);
  background-color: #fff;
}
```

## Dynamic Responsive Layouts with JavaScript

Read the viewport dimensions and select a layout dynamically:

```javascript
// Convert the physical screen width reported by Lynx to CSS pixels.
const viewportWidth = SystemInfo.pixelWidth / SystemInfo.pixelRatio;

// Select a layout based on the viewport width
function getLayoutClass(width) {
  if (width >= 1024) return 'desktop-layout';
  if (width >= 768) return 'tablet-layout';
  return 'mobile-layout';
}

// Apply the result to the component
const layoutClass = getLayoutClass(viewportWidth);
const showSidebar = viewportWidth >= 768;
```

Combine this with conditional rendering:

```jsx
<view className={layoutClass}>
  {showSidebar ? <Sidebar /> : null}
  <MainContent />
</view>
```

```css
.desktop-layout {
  display: flex;
  flex-direction: row;
}

.tablet-layout {
  display: flex;
  flex-direction: row;
}

.mobile-layout {
  display: flex;
  flex-direction: column;
}
```

## Responsive Design Best Practices

1. **Start mobile-first**: Define mobile styles first, then adjust them dynamically with JavaScript.
2. **Use `rem`**: Prefer `rem` for responsive scaling, with `vw` defining the root font size.
3. **Use viewport units**: Use `vw` and `vh` to create fluid layouts.
4. **Choose breakpoints in JavaScript**:
   - 320px - Small phone
   - 375px - Standard iPhone
   - 414px - iPhone Plus
   - 768px - iPad in portrait orientation
   - 1024px - iPad in landscape orientation
5. **Test across devices**: Verify the layout on a range of device sizes.

## Common Pitfalls

### Do Not Use `@media`

```css
/* ❌ Unsupported: this rule has no effect */
@media (min-width: 768px) {
  .container {
    flex-direction: row;
  }
}
```

### Use Supported Alternatives

```css
/* ✅ Use rem and viewport units */
.container {
  display: flex;
  flex-direction: column;
  padding: 4vw;
}

/* Set the base font size on the root element */
page {
  font-size: calc(100vw / 23.4375); /* 1rem = 16px at a 375px viewport width */
}
```

```javascript
// ✅ Update the layout dynamically with JavaScript
const viewportWidth = SystemInfo.pixelWidth / SystemInfo.pixelRatio;
const isWide = viewportWidth >= 768;
```
