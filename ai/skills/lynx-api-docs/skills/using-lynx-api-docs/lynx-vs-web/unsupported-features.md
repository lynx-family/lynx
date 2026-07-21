# Unsupported CSS Features

The following Web CSS features are either **unsupported** or **partially supported** in Lynx.

## ⚠️ Important: Selectors That Parse but Have No Effect

The CSS parser recognizes the following selectors without reporting an error, but the **selector matcher does not implement them**. They never match an element, so their styles never apply. This is a common source of migration issues.

### Structural Pseudo-classes: Parsed but Never Matched

- ⚠️ `:nth-child()` / `:nth-last-child()`
- ⚠️ `:nth-of-type()` / `:nth-last-of-type()`
- ⚠️ `:first-child` / `:last-child`
- ⚠️ `:only-child`
- ⚠️ `:first-of-type` / `:last-of-type`
- ⚠️ `:only-of-type`
- ⚠️ `:empty`

### Functional Pseudo-classes: Parsed but Never Matched

- ⚠️ `:is()`
- ⚠️ `:where()`
- ⚠️ `:has()` - Relational pseudo-class
- ⚠️ `:lang()`
- ⚠️ `:dir()`

### Link and State Pseudo-classes: Parsed but Never Matched

- ⚠️ `:link`
- ⚠️ `:visited`
- ⚠️ `:checked` - Checked state for form controls; parsed but never matched
- ⚠️ `:enabled` / `:disabled` - Form-control states; parsed but never matched
- ⚠️ `:default`

### Pseudo-elements: Parsed but Never Matched

- ⚠️ `::before`
- ⚠️ `::after`
- ⚠️ `::first-line`
- ⚠️ `::first-letter`
- ⚠️ `::backdrop`

### Selectors That Work: Seven in Total

| Selector          | Status              | Description                         |
| ----------------- | ------------------- | ----------------------------------- |
| `:not()`          | ✅ Fully supported  | Matches the nested selector         |
| `:hover`          | ⚠️ Platform-dependent | Hover state; support varies by platform |
| `:active`         | ✅ Fully supported  | Active state                        |
| `:focus`          | ✅ Fully supported  | Focus state                         |
| `:root`           | ✅ Fully supported  | Matches the root element, `page`    |
| `::placeholder`   | ✅ Fully supported  | Placeholder text styling            |
| `::selection`     | ✅ Fully supported  | Selected text styling               |

> **Pitfall**: A selector such as `:first-child` or `::before` does not produce a parse error, but its styles never take effect. Use another approach to reproduce the intended result.

## Completely Unsupported Features

### Display

- ❌ `display: inline` - Use the `<text>` element
- ❌ `display: inline-block` - Use Flexbox
- ❌ `display: table*` - All table-related values
- ❌ `display: list-item`
- ❌ `display: run-in`
- ❌ `display: contents`

### Float and Clear

- ❌ `float: left`
- ❌ `float: right`
- ❌ `float: none`
- ❌ `clear: left`
- ❌ `clear: right`
- ❌ `clear: both`
- ❌ `clear: none`

### Positioning

- ❌ `position: static` - The default is `relative`

### Length Values

- ❌ `stretch`

### Units

- ❌ `cm` - Centimeters
- ❌ `mm` - Millimeters
- ❌ `in` - Inches
- ❌ `pt` - Points
- ❌ `pc` - Picas
- ❌ `ch` - Character width
- ❌ `ex` - x-height

### Pseudo-classes

- ❌ `:is()` - Newer selector
- ❌ `:where()` - Newer selector
- ❌ `:has()` - Relational pseudo-class
- ❌ `:target` - Target pseudo-class
- ❌ `:lang()`
- ❌ `:dir()`
- ❌ `:read-only`
- ❌ `:read-write`
- ❌ `:placeholder-shown`
- ❌ `:default`
- ❌ `:checked` - Parsed but never matched
- ❌ `:indeterminate`
- ❌ `:valid`
- ❌ `:invalid`
- ❌ `:in-range`
- ❌ `:out-of-range`
- ❌ `:required`
- ❌ `:optional`
- ❌ `:blank`

### Pseudo-elements

- ❌ `::before` - Completely unsupported
- ❌ `::after` - Completely unsupported
- ❌ `::first-line` - First line
- ❌ `::first-letter` - First letter
- ❌ `::marker` - List marker
- ❌ `::backdrop`
- ❌ `::cue`
- ❌ `::part()`
- ❌ `::slotted()`

### Generated Content for Pseudo-elements

- ❌ `content: "text"` - Generated text is unsupported
- ❌ `content: url()` - Generated images are unsupported
- ❌ `content: attr()` - Unsupported

### At-rules

#### Media Queries: `@media` ⚠️

**Important**: Lynx does **not support** the CSS `@media` rule.

- ❌ **Completely unsupported**: `@media (min-width: ...)` and `@media (max-width: ...)` have no effect at runtime
- ❌ **Unsupported**: `@media print`
- ❌ **Unsupported**: Other media types such as `@media speech` and `@media tty`

**Alternatives**:

1. Use **`rem` with `vw`** for responsive adaptation; this is **recommended**.
2. Use **viewport units**, `vw` and `vh`, for fluid layouts.
3. Adjust styles dynamically with **JavaScript**.

```css
/* Recommended: use rem with vw instead of media queries. */
page {
  font-size: calc(100vw / 23.4375); /* 1rem = 16px at a width of 375px */
}

.container {
  width: 100vw;
  padding: 4vw; /* Adjusts automatically to the viewport width. */
}

.title {
  font-size: 2.25rem; /* Approximately 36px at 375px; scales with screen width. */
}

/* Alternative: use vw directly. */
.subtitle {
  font-size: 4.8vw; /* Approximately 18px at a width of 375px */
}
```

```javascript
// Adjust styles dynamically with JavaScript.
const viewportWidth = /* Obtain the viewport width. */;
if (viewportWidth >= 768) {
  // Apply styles for a large screen.
}
```

```css
/* Use viewport units instead of media queries. */
.container {
  width: 100vw;
  padding: 4vw; /* Adjusts automatically to the viewport width. */
}
```

#### Other At-rules

- ❌ `@layer` - Cascade layers
- ❌ `@supports` - Feature queries
- ❌ `@charset`
- ❌ `@namespace`

### Properties

- ❌ `content` - Generates content with `::before` or `::after`
- ❌ `quotes`
- ❌ `counter-*`
- ❌ `list-style-type`
- ❌ `list-style-image`
- ❌ `list-style-position`
- ❌ `caption-side`
- ❌ `table-layout`
- ❌ `border-collapse`
- ❌ `border-spacing`
- ❌ `empty-cells`
- ❌ `text-transform`
- ❌ `word-spacing`
- ❌ `word-wrap` / `overflow-wrap`
- ❌ `hyphens`
- ❌ `tab-size`
- ❌ `writing-mode`
- ❌ `text-orientation`
- ❌ `unicode-bidi`
- ❌ `white-space: pre | pre-wrap | pre-line | break-spaces` - Only `normal` and `nowrap` are supported
- ❌ `cursor`
- ❌ `resize`
- ❌ `scroll-behavior`
- ❌ `scroll-snap-*`
- ❌ `overscroll-behavior`
- ❌ `transform-style`

### The `env()` Function

- ✅ **Supported**: `env(safe-area-inset-top)`, `env(safe-area-inset-right)`, `env(safe-area-inset-bottom)`, and `env(safe-area-inset-left)` for safe-area adaptation
- ❌ **Unsupported**: Every other `env()` value, such as `env(titlebar-area-x)` and `env(keyboard-inset-height)`

### Functions

- ❌ `min()`
- ❌ `max()`
- ❌ `clamp()`

## Partially Supported Features

### Units

- ⚠️ `vmin` - The smaller viewport dimension
- ⚠️ `vmax` - The larger viewport dimension

### At-rules

- ⚠️ `@import`
- ⚠️ `@font-face`

### Grid Layout

- ❌ Named grid lines: `grid-template-columns: [start] 1fr [end]`
- ⚠️ The `grid-column` shorthand requires Lynx 3.9+ with `enableGridPlacementShorthands: true`; prefer `grid-column-start` and `grid-column-end` for compatibility
- ⚠️ The `grid-row` shorthand requires Lynx 3.9+ with `enableGridPlacementShorthands: true`; prefer `grid-row-start` and `grid-row-end` for compatibility
- ❌ The `grid-area` shorthand
- ⚠️ `minmax()`: Basic usage is supported, but complex cases may behave differently from the Web

### Flexbox

- ⚠️ `flex-basis: min-content` - Treated as `0px`
- ⚠️ `flex-basis: max-content` - Treated as `auto`

### Selectors

- ⚠️ Attribute selectors: `[attr^="val"]`, `[attr$="val"]`, and `[attr*="val"]`

### Text

- ⚠️ `text-overflow: fade` - Only `clip` and `ellipsis` are supported
- ⚠️ `vertical-align`
- ⚠️ `text-indent`
- ⚠️ `letter-spacing`
- ⚠️ `user-select`

### Values

- ⚠️ `currentColor`

### Transforms

- ⚠️ `perspective`

## Alternatives

| Unsupported feature  | Alternative                              |
| -------------------- | ---------------------------------------- |
| `float`              | `display: flex`                          |
| `display: inline`    | The `<text>` element                     |
| `min-content`        | A fixed value or `auto`                  |
| `writing-mode`       | Rotate with `transform`                  |
| `user-select`        | Use a platform-specific API              |
