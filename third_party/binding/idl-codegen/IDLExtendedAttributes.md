# Canvas radius conversion extension

## [CanvasRoundRectRadii]

[WHATWG CanvasPath.roundRect](https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-roundrect)
accepts an optional number, DOMPointInit, or sequence mixing both forms.
A number r supplies circular radii (r, r); DOMPointInit.x/y supply elliptical
radii, with zero defaults. WebIDL sequences also accept arbitrary iterables.

This is a local binding extension, not a WHATWG Web IDL attribute. The
standard roundRect declaration does not need it, and upstream Blink and
WebKit CanvasPath IDL do not use it. The public argument types and conversion
semantics still follow the standard; this annotation selects their local
native and command-buffer representation.

The current generator does not support this combination of numeric,
dictionary and sequence union members end to end. Supporting the standard
declaration without this annotation would also require general nested-union
conversion, iterable handling and buffered encoding support. For this change,
the dedicated converter keeps that broader binding work out of scope and
reuses consecutive x/y double pairs for native and command-buffer calls.
Buffered calls convert before encoding so getters, iterators and exceptions
remain observable during the original call. Remove this extension when the
general binding path can preserve those semantics across both call paths.

WebIDL conversion reads dictionary members in lexicographic order, including
w/z even though only x/y affect geometry. Getters and numeric conversions may
throw. An item conversion failure closes the iterator, allowing a generator's
finally block to run; the original exception is preserved. These are argument
conversion requirements, and apply before path-level count, sign and finite
value checks. They do not add drawing behavior to the renderer.

Use this attribute only on the radii argument of CanvasPath.roundRect.
