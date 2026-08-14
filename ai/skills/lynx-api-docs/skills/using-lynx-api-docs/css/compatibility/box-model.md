# Box model and compatibility mode

## Authoring guidance

The default `box-sizing` value is `auto`. In this package baseline, `auto` is
used as border-box behavior in the default Lynx compatibility mode and as
content-box behavior when W3C alignment is enabled.

Set `box-sizing: border-box` or `box-sizing: content-box` explicitly when a
component must not depend on its host page mode.

```css
.stable-component {
  box-sizing: border-box;
}
```

## Web migration

Web CSS defaults to `content-box`. Choosing the same explicit box model is only
one migration step: floats, the browser's default body margin and line height,
absolute-position containing blocks, and min/max-size clamping can still
produce different geometry.
