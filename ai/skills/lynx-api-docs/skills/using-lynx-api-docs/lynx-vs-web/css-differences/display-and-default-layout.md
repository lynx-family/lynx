# Display and Default Layout

## The `display` Property

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

## Default Layout Behavior (Important)

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
