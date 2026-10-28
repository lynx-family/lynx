# Positioning and Stacking

## Positioning

**`static` is not supported**:

```css
/* Web */
position: static; /* Default */

/* Lynx: use relative instead. */
position: relative; /* Default */
```

**`position: fixed` requires a definite inset on each axis**:

On the Web, a fixed-positioned element with `left: auto` or `top: auto` can be placed at its **static position**, its original position in normal flow. Lynx does not use this Web static-position fallback. Set at least one definite inset on each axis: `left` or `right` horizontally, and `top` or `bottom` vertically. A definite start inset wins when both sides are specified; when the start inset is `auto` and the end inset is definite, Lynx uses the end inset.

```css
/* ❌ Lynx: neither axis has a definite inset. */
#fixed-box {
  position: fixed;
}

/* ✅ Lynx: right and bottom provide definite insets on both axes. */
#fixed-box {
  position: fixed;
  right: 20px;
  bottom: 20px;
}
```

## Floats Are Not Supported

```css
/* ❌ Not supported */
float: left;
float: right;
clear: both;

/* ✅ Use Flexbox instead. */
display: flex;
flex-direction: row;
```

## `z-index`

**An explicit `z-index` creates a stacking context without requiring `position`**:

```css
/* Lynx: this element creates a stacking context without position. */
.content-layer {
  z-index: 0;
}
```

Do not add `position` solely to make `z-index` work. An ancestor's explicit `z-index`, including `0`, can constrain the stacking order of its descendants. When a child must participate in a higher-level stacking order, remove the unnecessary ancestor `z-index` or adjust the ancestor structure instead of adding unrelated positioning styles.

**Stacking contexts and compositing layers**:

In Lynx, `z-index` creates a new **stacking context** and may promote an element to a **compositing layer**. As a result, child elements in scroll containers such as `scroll-view` and `scroll-coordinator` may fail to move with scrolling.

```css
/* ❌ Problem: children with z-index may not move with the scroll. */
scroll-coordinator-header {
  /* Children in the header set z-index. */
}
.header-item {
  position: relative;
  z-index: 10; /* This element may be promoted and fail to move with the header. */
}

/* ✅ Solution: establish the same stacking context on the parent with z-index: 0. */
scroll-coordinator-header {
  position: relative;
  z-index: 0; /* Make the header itself a compositing layer. */
}
.header-item {
  position: relative;
  z-index: 10; /* Now moves correctly with scroll-coordinator-header. */
}
```

**How it works**:

- When a child sets `z-index`, it is promoted to an independent compositing layer.
- That layer's coordinate transform may not include the parent container's scroll offset.
- Adding `z-index: 0`, or any `z-index`, to the parent creates a stacking context so the children remain in the same layer and move with scrolling.

**Common scenarios**:

- Elements with `z-index` inside `scroll-coordinator-header` do not move with scrolling.
- Fixed-positioned elements inside `scroll-view` interact incorrectly with scrollable content in the stacking order.
- Overlay popups interact incorrectly with scrollable content in the stacking order.
