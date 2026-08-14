# Computed-style CSS text

Lynx's CSS-text computed-style path uses a fixed getter allowlist rather than
serializing every registered property like Web `getComputedStyle()`.

| Declaration or query | Result in this package baseline |
| --- | --- |
| `pointer-events` | Hit testing works, but no CSS-text getter is registered |
| `mask-image` | Painting data can be applied, but no CSS-text getter is registered |
| `text-shadow` | Rendering data can be applied, but no CSS-text getter is registered |
| `text-decoration` | Rendering data can be applied, but no canonical CSS-text getter is registered |
| `backdrop-filter` | No value because the property is not registered |
| `text-decoration-line` or `text-decoration-style` | No value because the longhands are not registered |

`filter` has a getter and serializes its default as `none`. It serializes
`blur()` and `grayscale()` accurately, but applied `brightness()`,
`contrast()`, and `saturate()` values currently serialize as `none`.

An empty or fallback result is not a feature-detection signal. Verify property
registration and the value-specific serializer, then verify behavior. Keep
authored state in application data instead of round-tripping it through this
query.
