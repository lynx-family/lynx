# Transforms and stacking contexts

## Transform function support

This package baseline recognizes `translate()`, `translateX()`, `translateY()`,
`translate3d()`, `scale()`, `scaleX()`, `scaleY()`, `rotate()`, `rotateX()`,
`rotateY()`, `rotateZ()`, `skew()`, `skewX()`, `skewY()`, `matrix()`, and
`matrix3d()`.

It does not recognize `scale3d()` or `rotate3d()`. A declaration containing an
unsupported transform function is invalid. Use two-dimensional scale,
single-axis rotation, or an explicit `matrix3d()` when those forms express the
required result.

## `transform: none`

In this package baseline, an explicit `transform: none` still leaves transform
data on the computed style. The element is consequently treated as a
stacking-context node.

Remove the `transform` declaration when descendants must remain in the
ancestor stacking context. Non-`none` transforms also create a stacking
context, as expected.
