# Clay Text Measurement Spec

## Scope

This document describes the Clay text measurement path for `<text>` nodes, with
focus on width precision and rounding. It covers the code around
`TextShadowNode`, `TextRender`, `ShadowLayoutContextMeasure`, and the paragraph
backends used by Clay.

This spec does not require changes in TTText. When TTText returns a rounded
intrinsic width, Clay should recover subpixel precision from data already
exposed by the paragraph interface when it is safe to do so.

## Input Lengths

CSS lengths are resolved before text measurement reaches Clay.

For `rpx`, Lynx resolves the value from the viewport width:

```text
resolved_px = rpx_value * viewport_width / 750
```

For example, on an iPhone 16 Pro portrait viewport of 402 pt:

```text
20rpx = 20 * 402 / 750 = 10.72pt
```

Clay keeps the resolved `font-size` as a floating-point value in
`BaseTextShadowNode::SetFontSize`; it should not snap the font size to an
integer layout unit or to the device pixel grid before paragraph measurement.

For two full-em CJK glyphs, the expected width is about:

```text
10.72 * 2 = 21.44pt
```

This is the width that should be preserved by Clay text measurement. Rounding it
to a whole layout unit produces 22 pt. The common layout layer may still snap
the final layout result to the physical pixel grid.

`line-height` contributes to line box height and baseline placement. It should
not force integer rounding of the measured text width.

## Main Entry Points

Text measurement starts from:

```text
TextShadowNode::Measure(const MeasureConstraint& constraint)
```

The shadow node creates a `ShadowLayoutContextMeasure`, passes it into
`TextRender::Measure`, and maps the resulting layout context back to a
`MeasureResult`.

The main text render path is:

```text
TextRender::Measure
TextRender::BuildTextLayout
LayoutParagraph
txt::Paragraph::Layout
```

Static helper APIs follow the same width policy:

```text
TextRender::GetTextInfo
TextRender::MeasureText
```

## Measure Constraints

`TextShadowNode::Measure` interprets width constraints as follows.

For definite width:

```text
result.width = constraint.width
```

The parent has already decided the width, so text intrinsic width is not used as
the returned layout width.

For indefinite width:

```text
layout_width = infinity
result.width = measured_text_width
```

The paragraph is allowed to lay out without a width limit. The returned width is
the actual measured paragraph width and must preserve subpixel precision.

For at-most width:

```text
layout_width = constraint.width
result.width = min(measured_text_width, constraint.width)
```

The paragraph lays out with the available width, and the returned width is
clamped to that available width.

## Layout Context Fields

`ShadowLayoutContextMeasure::layout_width_` stores the width used to lay out the
paragraph.

`ShadowLayoutContextMeasure::measured_width_` stores the measured paragraph
width. It must be a floating-point value so Clay can preserve subpixel text
widths such as 21.3333.

`TextRender::measured_width_` follows the same rule. It must not be narrowed to
an integer before the result is returned to `TextShadowNode`.

`ShadowLayoutContextMeasure::measured_height_` stores the measured paragraph
height. It is also floating point, so intermediate text layout does not lose
precision before the final result boundary.

`TextRender::measured_height_` follows the same rule. Paragraph height and line
spacing adjustments should be stored without integer `ceil` in the text render
path.

## Paragraph Construction

`LayoutParagraph` builds a `TextParagraphBuilder` from the text shadow node
style and children. It applies:

- font style, size, weight, family, variation, and feature settings
- text color, decoration, shadow, and stroke
- text direction
- text alignment
- white-space and max-line settings
- max-length truncation
- text indent
- inline placeholders
- emoji and attachment handling

After construction, the paragraph is laid out with `paragraph->Layout(width)`.

For `white-space: nowrap` without ellipsis, Clay uses an infinite layout width.
That keeps the text on one line and makes the measured width the intrinsic line
width.

## Width Source

The default measured width source is:

```text
paragraph->GetMaxIntrinsicWidth()
```

For paragraph backends that preserve fractional intrinsic widths, this value is
the authoritative measured text width.

When `CLAY_ENABLE_TTTEXT` is enabled, `ParagraphTTText::GetMaxIntrinsicWidth()`
may be backed by TTText `LayoutRegion::GetLayoutedWidth()`. That value can
already be rounded before Clay receives it. Clay should not modify TTText for
this fix. Instead, Clay uses paragraph line metrics as a more precise width
source when that substitution is safe.

The Clay helper policy is:

```text
measured_width = paragraph->GetMaxIntrinsicWidth()

if CLAY_ENABLE_TTTEXT:
  longest_line_width = max(line_metrics[i].width)

  if layout_width is infinite:
    measured_width = longest_line_width
  else if longest_line_width + tolerance < layout_width:
    measured_width = longest_line_width
  else:
    keep paragraph->GetMaxIntrinsicWidth()
```

The finite-width guard matters for wrapping. If a line fills the available width,
the line metric can represent the shaped line width while the paragraph width
still needs to remain tied to the layout constraint. In that case Clay keeps the
paragraph intrinsic width instead of replacing it with the line metric.

For short unconstrained or non-wrapping text, such as a two-CJK-glyph pinned tag,
line metrics can preserve widths like 21.3333 even when TTText's intrinsic width
has already been rounded to 22.

## Rounding Policy

Clay text measurement should not round measured width to a whole layout unit.

The width should stay floating point through:

```text
TextRender::measured_width_
ShadowLayoutContextMeasure::measured_width_
MeasureResult.width
```

Final rounding should happen only in the common layout layer that snaps measured
sizes to the physical pixel grid:

```text
ceil(width * physical_pixels_per_layout_unit) / physical_pixels_per_layout_unit
```

On a 3x device, this preserves a width of 21.3333 as 21.3333 because it already
lands on a physical pixel boundary. A whole-unit `ceil(width)` in the text layer
would incorrectly turn the same value into 22.

If final layout still reports 22 for a 21.3333 text measurement, check
`physical_pixels_per_layout_unit`. If it is 1, the common layout rounding step
will snap to whole layout units.

## Height Policy

Clay text measurement also keeps measured height floating point inside the text
layout path.

The height should stay floating point through:

```text
TextRender::measured_height_
ShadowLayoutContextMeasure::measured_height_
```

`MeasureResult.height` remains the result boundary. For non-definite height
constraints, Clay returns `ceil(measured_height)` there to preserve existing
external behavior while avoiding premature integer conversion during paragraph
measurement.

## Second Layout

`TextRender` can request a second layout pass through `need_second_layout_`.

This is used when the first pass used a wider layout width than the final
measured width, and a second pass is needed for alignment or final line
placement. Typical cases include:

- indefinite width with non-left text alignment
- at-most width where the measured text width is sufficiently smaller than the
  available width

When a second pass is required, `TextShadowNode::Measure` runs a definite-width
measure using the first pass result width and height. The width passed into the
second pass should be the preserved floating-point width, not an integer-ceiled
width.

## Native iOS Comparison

The native iOS text path measures with TextKit and returns a `CGFloat` size from
the layout manager used rect. It does not round the measured width to a whole
layout unit in `LynxTextShadowNode`.

Clay should follow the same precision model at the text measurement boundary:
preserve the fractional text width first, then let shared layout rounding snap
the final result to the physical pixel grid.

## Debug Checklist

When investigating text width precision, log these values in order:

- resolved `font-size`
- `font-size` after `BaseTextShadowNode::SetFontSize`
- paragraph layout width
- `paragraph->GetMaxIntrinsicWidth()`
- `paragraph->GetMaxWidth()`
- each `LineMetrics::width`
- Clay selected measured width
- `TextShadowNode::Measure` result width
- `physical_pixels_per_layout_unit` in the common layout layer

This separates precision loss into one of three categories:

- input length or font-size conversion
- paragraph backend intrinsic width rounding
- final layout pixel-grid rounding

## Non-goals

This policy intentionally does not:

- rewrite TTText layout internals
- change height measurement behavior
- force every paragraph backend to use line metrics
- bypass the common physical-pixel rounding step

The fix is limited to preserving fractional text width inside Clay until the
shared layout layer performs its normal final rounding.
