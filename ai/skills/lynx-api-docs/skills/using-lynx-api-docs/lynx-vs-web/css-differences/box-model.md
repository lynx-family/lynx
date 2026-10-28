# Box Model

## Box Model

**Default `box-sizing`**:

```css
/* Web default */
box-sizing: content-box;

/* Lynx initial value: normally uses border-box sizing */
box-sizing: auto;
```

> **Note**: Lynx initially uses `auto`, which normally resolves to `border-box` sizing: `width` and `height` include padding and borders. Legacy Web-aligned configuration can instead resolve `auto` as `content-box`. Set `border-box` or `content-box` explicitly for configuration-independent sizing. The Web initial value is `content-box`, where `width` and `height` apply only to the content box.
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
