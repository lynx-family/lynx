# Canvas radius conversion extension

## [CanvasRoundRectRadii]

[WHATWG CanvasPath.roundRect](https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-roundrect)
accepts an optional number, DOMPointInit, or sequence mixing both forms.
A number r supplies circular radii (r, r); DOMPointInit.x/y supply elliptical
radii, with zero defaults. WebIDL sequences also accept arbitrary iterables.

This argument attribute selects a converter for that union because the
existing generator does not handle its numeric/dictionary/sequence members
together. Native and command-buffer calls receive consecutive x/y double
pairs, rather than dictionary objects or a new transport type.

WebIDL conversion reads dictionary members in lexicographic order, including
w/z even though only x/y affect geometry. Getters and numeric conversions may
throw. An item conversion failure closes the iterator, allowing a generator's
finally block to run; the original exception is preserved. These are argument
conversion requirements, and apply before path-level count, sign and finite
value checks. They do not add drawing behavior to the renderer.

Use this attribute only on the radii argument of CanvasPath.roundRect.
