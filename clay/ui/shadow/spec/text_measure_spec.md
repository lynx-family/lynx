# Clay Text Measurement Spec

## Scope

This document defines width precision and rounding for Clay `<text>`
measurement. It applies to `TextShadowNode`, `TextRender`,
`ShadowLayoutContextMeasure`, `InternalTextView`, and
`InnerTextShadowNode`.

## Input Lengths

CSS lengths are resolved before Clay text measurement. For example:

```text
resolved_px = rpx_value * viewport_width / 750
```

`BaseTextShadowNode::SetFontSize` must preserve the resolved floating-point
value. It must not snap font size to an integer layout unit or device pixel
before paragraph measurement. `line-height` must not force width rounding.

## Measure Constraints

All text measurement entry points use the same width policy:

```text
definite:   result.width = constraint.width
indefinite: layout_width = infinity
            result.width = measured_text_width
at-most:    layout_width = constraint.width
            result.width = min(measured_text_width, constraint.width)
```

Definite width is owned by the parent. Indefinite and at-most results must
preserve the measured fractional width.

## Layout Context Fields

The following values must remain floating point until the result boundary:

- width and height passed into `LayoutContextClay::MeasureImpl`
- `ShadowLayoutContextMeasure::{measured_width_, measured_height_}`
- `TextRender::{measured_width_, measured_height_}`
- equivalent `InternalTextView` and `InnerTextShadowNode` fields

## Width Source

```text
measured_width = paragraph->GetMaxIntrinsicWidth()
```

This is authoritative for paragraph backends that preserve fractional
intrinsic widths. With `CLAY_ENABLE_TTTEXT`, the intrinsic width may already be
rounded. Clay must recover precision from line metrics only when safe:

```text
longest_line_width = max(line_metrics[i].width)
use longest_line_width when:
  layout_width is infinite, or
  longest_line_width + tolerance < layout_width
otherwise keep GetMaxIntrinsicWidth()
```

The finite-width guard preserves wrapping behavior when a line fills the
available width. The helper applies consistently to normal text, inner text,
internal text views, `GetTextInfo`, and `MeasureText`.

## Rounding Policy

Clay must not apply `ceil(width)` in the text layer. Width remains floating
point through:

```text
TextRender::measured_width_
ShadowLayoutContextMeasure::measured_width_
MeasureResult.width
```

Only the common layout layer may snap the result to the physical pixel grid:

```text
ceil(width * physical_pixels_per_layout_unit) / physical_pixels_per_layout_unit
```

For example, 21.3333 remains 21.3333 at 3x but would incorrectly become 22
under text-layer `ceil`.

## Height Policy

Measured height remains floating point inside the text path. For non-definite
height constraints, Clay applies `ceil(measured_height)` only when assigning
`MeasureResult.height`, preserving existing external behavior.

## Second Layout

When `need_second_layout_` is set, `TextShadowNode::Measure` runs a
definite-width second pass for alignment or final line placement. This includes:

- indefinite width with non-left text alignment
- at-most width where measured text is narrower than the available width

The second pass must use the preserved fractional first-pass width.

## Non-goals

This policy intentionally does not:

- rewrite TTText layout internals
- force every paragraph backend to use line metrics
- bypass the common physical-pixel rounding step

The change preserves fractional text width inside Clay until shared layout
performs its normal final rounding.
