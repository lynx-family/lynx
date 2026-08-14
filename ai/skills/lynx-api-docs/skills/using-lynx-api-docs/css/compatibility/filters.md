# Filters

This package baseline accepts one of the following values for `filter`:

- `none`
- `grayscale()`
- `blur()`
- `brightness()`
- `contrast()`
- `saturate()`

Multiple chained functions are not supported. `opacity()`, `drop-shadow()`,
`hue-rotate()`, `invert()`, and `sepia()` are not recognized, and
`backdrop-filter` is not a registered Lynx property.

Use the ordinary `opacity` property instead of `filter: opacity(...)`. Use
`box-shadow` or `text-shadow` when that matches the desired shadow; Lynx has no
general replacement for `filter: drop-shadow(...)`.

The computed-style serializer has additional limitations for supported filter
functions. See [Computed-style CSS text](./computed-style.md) before using a
serialized filter value for state or feature detection.
