# Migration Checklist and Common Pitfalls

## Quick Migration Checklist

When migrating from the Web to Lynx, check the following:

- [ ] Replace `display: inline` and `display: inline-block`; prefer an explicit Lynx layout mode over the `display: block` compatibility value.
- [ ] Wrap all text in `<text>` elements.
- [ ] Remove every `float` and `clear`; use Flexbox instead.
- [ ] Replace `position: static` with `relative`.
- [ ] Replace `min-content` for intrinsic width or height; `flex-basis: min-content` degrades to `0px`, while `max-content` is supported.
- [ ] Treat every explicit `z-index`, including `0`, as creating a stacking context even without `position`; check ancestor contexts instead of automatically adding `position`.
- [ ] Check whether `z-index` in a scroll container such as `scroll-view` or `scroll-coordinator` prevents an element from moving with scrolling.
- [ ] If so, add `z-index: 0` to the parent to establish the same stacking context.
- [ ] Remove every use of `::before` and `::after`; they are not supported.
- [ ] Check for margin-collapsing dependencies.
- [ ] Replace Web `scale3d()` and `rotate3d()` calls with Lynx-supported `scale()`, `rotateX/Y/Z()`, or `matrix3d()` calls.
- [ ] Add the step-position argument to `steps(n)`, for example `steps(n, end)`.
- [ ] Test every `white-space` setting.

## Common Pitfalls

### Pitfall 1: Omitting the `text` Element

```jsx
// ❌ Incorrect
<view>Text content</view>

// ✅ Correct
<view>
  <text>Text content</text>
</view>
```

### Pitfall 2: Relying on Margin Collapsing

```css
/* Web: the margins collapse to 20px. */
.top {
  margin-bottom: 20px;
}
.bottom {
  margin-top: 20px;
}

/* Lynx: the margins do not collapse, so this produces 40px; adjust them. */
.top {
  margin-bottom: 10px;
}
.bottom {
  margin-top: 10px;
}
/* Or: */
.top {
  margin-bottom: 20px;
}
.bottom {
  margin-top: 0;
}
```

### Pitfall 3: Using Unsupported or Partially Supported Units

```css
/* ❌ Not supported */
width: 1in;

/* ⚠️ Partially supported */
width: 100vmin;

/* ✅ Use an alternative. */
width: auto;
width: 100%;
```

### Pitfall 4: Omitting `white-space`

```css
/* Text does not wrap by default. */
text {
  white-space: nowrap;
}

/* Explicitly enable wrapping when required. */
text {
  white-space: normal;
}
```

### Pitfall 5: Failing to Copy `body` Styles to the `page` Element

Web user-agent style sheets commonly give `body` a default margin, while authored styles may add padding or replace that margin. The Lynx root element, `page`, has neither margin nor padding by default.

```css
/* Web: a common user-agent default is margin: 8px; padding is authored here. */
body {
  margin: 8px;
  padding: 0 8px;
}

/* Lynx: reproduce the computed or authored body spacing explicitly. */
page {
  margin: 8px;
  padding: 0 8px;
}
```

To preserve the layout during migration, inspect the Web `body` element's computed and authored styles, then reproduce the relevant spacing on `page` rather than assuming a fixed browser default.

### Pitfall 6: Mapping Web `html` and `body` Backgrounds to Ordinary Containers

On the Web, `html` is the document root and `body` is the page-content container. They follow special canvas background-propagation rules. Lynx has no equivalent elements with those names. `page` is both the page root and the surface for the full-page background.

```css
/* Web: html provides the page color; body provides the content-area background. */
html {
  background-color: #f5f5f5;
}

body {
  margin: 8px;
  background-color: white;
}
```

When migrating to Lynx, first determine the purpose of each Web background:

- Set `page` when a full-page background is required.
- Set the root `view` when a content-container background is required.
- When the Web version uses separate backgrounds for both `html` and `body`, normally map the `html` background to `page` and the `body` background to the root `view`.

```css
/* Lynx */
page {
  background-color: #f5f5f5;
}

.root {
  margin: 8px;
  background-color: white;
}
```

If the Web page relies only on the `body` background filling the viewport, setting `page` directly in Lynx produces the closest full-page result:

```css
/* Web */
body {
  background-color: green;
}

/* Lynx */
page {
  background-color: green;
}
```

Do not treat an ordinary `view` named `.body` as equivalent to the Web `body`. An ordinary `view` paints its background only within its own layout bounds; `page` is the Lynx root and full-page background surface.

### Pitfall 7: Migrating a `background` Shorthand That Contains `background-attachment`

The Web `background` shorthand may include the `background-attachment` slot with values such as `fixed` or `scroll`. The current supported-property list does not include `background-attachment`. When migrating, expand the shorthand into the background properties Lynx supports and separately determine whether the attachment behavior must be preserved:

```css
/* Web */
.box {
  background: url('bg.png') 0 0 / cover no-repeat fixed #fff;
}
```

```css
/* Lynx */
.box {
  background-image: url('bg.png');
  background-position: 0 0;
  background-size: cover;
  background-repeat: no-repeat;
  background-color: #fff;
}
```

```css
/* For a solid background, use background-color directly. */
.box {
  background-color: green;
}
```
