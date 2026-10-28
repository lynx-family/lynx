# Transforms and Animations

## Subset of 3D Transform Functions

Web CSS supports `scale3d(x, y, z)` and `rotate3d(x, y, z, angle)`, but the Lynx transform parser does not currently recognize either function name. A declaration that contains an unsupported transform function is parsed as invalid. If it appears in `@keyframes`, the corresponding `transform` keyframe is not added to the animation curve.

| Web syntax | Lynx status | Migration guidance |
| --- | --- | --- |
| `scale3d(x, y, z)` | ❌ Not supported | Use `scale(x, y)` when only XY scaling is required; use `matrix3d(...)` to express Z-axis scaling explicitly |
| `rotate3d(x, y, z, angle)` | ❌ Not supported | Use `rotateX(...)`, `rotateY(...)`, or `rotateZ(...)` for single-axis rotation; use `matrix3d(...)` for arbitrary-axis rotation |
| `translate3d(x, y, z)` | ✅ Supported | Use directly |
| `matrix3d(...)` | ✅ Supported | Use as an explicit compatibility form for complex 3D transforms |

```css
/* ❌ Web syntax: Lynx does not recognize scale3d or rotate3d. */
.card {
  transform: scale3d(0.6, 0.6, 1) rotate3d(0, 1, 0, 180deg);
}

/* ✅ Lynx: split the transform into supported functions. */
.card {
  transform: scale(0.6, 0.6) rotateY(180deg);
}
```

Take particular care when migrating animations:

```css
/* ❌ The transform keyframe fails to parse in Lynx. */
@keyframes zoom {
  0% { transform: scale3d(0.6, 0.6, 1); }
  100% { transform: scale3d(1, 1, 1); }
}

/* ✅ Use scale(x, y). */
@keyframes zoom {
  0% { transform: scale(0.6, 0.6); }
  100% { transform: scale(1, 1); }
}
```

## The `steps(...)` Animation Timing Function

On the Web, the second argument to `steps(n)` may be omitted; the default is equivalent to `steps(n, jump-end)`. In Lynx, `steps(...)` requires an explicit step-position argument. Forms such as `steps(1)` and `steps(4)` are invalid.

| Web syntax | Lynx status | Lynx syntax |
| --- | --- | --- |
| `steps(4)` | ❌ Not supported | `steps(4, end)` or `steps(4, jump-end)` |
| `steps(4, start)` | ✅ Supported | Use directly |
| `steps(4, end)` | ✅ Supported | Use directly |
| `steps(4, jump-start)` | ✅ Supported | Use directly |
| `steps(4, jump-end)` | ✅ Supported | Use directly |
| `steps(4, jump-none)` | ✅ Supported | Use directly |
| `steps(4, jump-both)` | ✅ Supported | Use directly |
| `step-start` | ✅ Supported | Equivalent to `steps(1, start)` |
| `step-end` | ✅ Supported | Equivalent to `steps(1, end)` |

```css
/* ❌ Valid on the Web, invalid in Lynx. */
animation-timing-function: steps(4);

/* ✅ Lynx requires an explicit step position. */
animation-timing-function: steps(4, end);
```

As on the Web, `animation-timing-function` inside a keyframe applies to the interval from the current keyframe to the next, rather than to the current point itself:

```css
@keyframes fade-out-after-hold {
  0% {
    opacity: 1;
  }
  80% {
    opacity: 1;
    animation-timing-function: step-end;
  }
  100% {
    opacity: 0;
  }
}
```

Here, `step-end` controls the interval from `80%` to `100%`, so the opacity remains unchanged until it jumps at the end of the interval.
