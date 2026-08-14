# Background and border painting

## Background origin, clip, and size

Lynx follows the CSS distinction between the background positioning area and
the background painting area. `background-origin` selects the box used to size
and position an image; `background-clip` selects the area in which it may be
painted.

The Web initial value of `background-origin` is `padding-box`. A `background`
shorthand that omits box values therefore sizes `cover` against the padding
box while the initial `background-clip: border-box` allows painting into the
border area. That result is standard CSS behavior, not a Lynx incompatibility.

When an image must be sized against the border box, set both properties
explicitly:

```css
.target {
  border: 25px solid transparent;
  background: url(image.png) 0 0 / cover no-repeat;
  background-origin: border-box;
  background-clip: border-box;
}
```

This package baseline implements `background-origin: border-box` in the
Android, iOS, Harmony, Fragment Layer, and Clay painting paths. Use nested
views only when the desired composition cannot be expressed with explicit
origin and clip, or when a separately verified host-specific painting issue
remains.

See CSS Backgrounds and Borders Level 3 sections
[2.7](https://drafts.csswg.org/css-backgrounds-3/#the-background-clip),
[2.8](https://drafts.csswg.org/css-backgrounds-3/#the-background-origin), and
[2.9](https://drafts.csswg.org/css-backgrounds-3/#the-background-size).

## Repetition

`background-repeat: repeat-y` maps to no-repeat on the horizontal axis and
repeat on the vertical axis. If repeated-image bounds match a Web reference
but surrounding text wraps differently, investigate text metrics rather than
attributing the difference to `repeat-y`.

## Patterned borders

Patterned border geometry remains backend-specific. Android implements dotted
and dashed borders with a dash path effect whose stroke does not request round
caps. A thick dotted border can therefore appear as rectangular segments
instead of Web-style circular dots. Prefer `solid` for exact geometry or draw
decorative dots explicitly.
