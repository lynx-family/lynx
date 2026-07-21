# CSS selectors

Lynx supports most commonly used CSS selectors.

## Basic selectors

### Type selector

```css
view {
  background-color: #fff;
}

text {
  color: #333;
}

image {
  border-radius: 8px;
}
```

### Class selector

```css
.container {
  padding: 16px;
}

.button-primary {
  background-color: #ff351a;
  color: #fff;
}
```

### ID selector

```css
#header {
  position: sticky;
  top: 0;
}

#main-content {
  flex: 1;
}
```

### Universal selector

```css
* {
  box-sizing: border-box;
}
```

## Combinators and selector lists

### Descendant combinator

```css
.container .item {
  /* Select every .item descendant of .container */
  margin-bottom: 12px;
}
```

### Child combinator

```css
.list > .list-item {
  /* Select direct children */
  border-bottom: 1px solid #eee;
}
```

### Next-sibling combinator

```css
.title + .subtitle {
  /* Select the .subtitle immediately following .title */
  margin-top: 8px;
}
```

### Subsequent-sibling combinator

```css
.header ~ .content {
  /* Select every .content sibling that follows .header */
  padding-top: 16px;
}
```

### Selector list

```css
h1,
h2,
h3 {
  font-weight: bold;
}
```

## Attribute selectors

```css
/* Elements with this attribute */
[disabled] {
  opacity: 0.5;
}

/* Exact match */
[type='text'] {
  border: 1px solid #ccc;
}

/* Value contains a substring (partial support) */
[class*='active'] {
  color: red;
}
```

> **Note:** `[class~='val']`, which matches an item in a whitespace-separated list, is unsupported. Use `[class*='val']`, which performs substring matching, instead.

## Pseudo-classes

### State pseudo-classes

```css
/* Active state */
:active

/* Focused state */
:focus

/* Hover state (support varies by platform) */
:hover

/* Root element */
:root
```

> **Note:** Although the CSS parser recognizes `:disabled` and `:enabled`, the Lynx DOM layer does not implement matching for these pseudo-classes. They therefore have no effect in practice. Apply disabled-state styles by adding and removing a class instead.
>
> ```css
> /* Recommended */
> .button.disabled {
>   opacity: 0.5;
>   background-color: #ccc;
> }
> ```

### Negation pseudo-class

```css
/* Exclude certain elements */
:not(.exclude) {
  /* Select elements that do not have the .exclude class */
}

.item:not(.last-item) {
  /* Select every .item that is not explicitly marked as the last item */
  border-bottom: 1px solid #eee;
}
```

## Pseudo-elements

```css
/* Placeholder text in an input element */
input::placeholder {
  color: #999;
}

/* Selected text */
::selection {
  background-color: #1890ff;
  color: #fff;
}
```

> **Note:** `::before` and `::after` are **not supported**. Although the CSS parser recognizes these pseudo-elements, neither the selector matcher nor the rendering engine implements them, so using them has no effect.

## Selector specificity

Specificity precedence, from highest to lowest:

1. **Inline styles** - `style="..."` (highest)
2. **ID selectors** - `#id`
3. **Class, attribute, and pseudo-class selectors** - `.class`, `[attr]`, `:hover`
4. **Type selectors** - `tag`
5. **Universal selector** - `*` (lowest)

### Calculation examples

```css
/* 0-1-1-0 */
#nav .menu {
}

/* 0-0-2-0 */
.nav .menu-item {
}

/* 0-0-2-1 */
.nav a:hover {
}
```

## Selector limitations

The following selectors are unsupported:

- ❌ `:is()` - Newer selector syntax
- ❌ `:where()` - Newer selector syntax
- ❌ `:has()` - Newer selector syntax
- ❌ `:first-child` / `:last-child` / `:nth-child()` - Structural pseudo-classes
- ❌ `:nth-of-type()` / `:only-child` / `:empty` - Structural pseudo-classes
- ❌ `:disabled` / `:enabled` - Form state pseudo-classes
- ❌ `::before` / `::after` - Pseudo-elements
- ❌ Complex attribute selectors such as `[attr~="val"]` (whitespace-separated list matching)

**Note:** All selectors marked with ❌ above are unsupported.
