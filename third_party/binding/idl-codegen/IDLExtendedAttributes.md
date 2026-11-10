# Canvas radius conversion extension

## [CanvasRoundRectRadii]

This argument attribute selects the CanvasPath roundRect radii converter. The
IDL retains its optional number, DOMPointInit, or sequence union, while the
native and command-buffer representation is a sequence of consecutive x/y
double pairs. Conversion follows WebIDL, including dictionary getter order
and iterable closing on conversion errors. Radius count, sign, and finite
value checks are performed by the path operation after argument conversion.

Use this attribute only on the radii argument of CanvasPath.roundRect.
