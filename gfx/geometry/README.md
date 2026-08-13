# Transform Operations

`gfx/geometry` owns the platform-neutral transform representation, matrix
calculation, decomposition, and interpolation. It must not depend on CSS,
Starlight, TASM, DOM elements, or platform matrix types.

## CSS and GFX representations

`starlight::TransformType` describes parsed CSS syntax, so it distinguishes
function spellings such as `translateX()`, `translateY()`, and
`translate3d()`. `gfx::TransformOperation::Type` describes normalized graphics
operations. Variants that can be represented by different parameters of the
same operation intentionally share one GFX type.

The Lynx CSS adapter in
`core/renderer/css/transforms/transform_operations_helper.*` performs the
normalization:

| Starlight input | GFX operation | Normalization |
| --- | --- | --- |
| `translate`, `translateX`, `translateY`, `translateZ`, `translate3d` | `kTranslate` | Missing axes are zero. |
| `rotate`, `rotateZ` | `kRotateZ` | CSS `rotate()` rotates around the Z axis. |
| `rotateX`, `rotateY` | `kRotateX`, `kRotateY` | The axis is preserved. |
| `scale`, `scaleX`, `scaleY` | `kScale` | Missing axes are one. |
| `skew`, `skewX`, `skewY` | `kSkew` | Missing angles are zero. |
| `matrix`, `matrix3d` | `kMatrix`, `kMatrix3d` | The matrix dimensionality is preserved. |
| `none` | No operation | An empty operation list is the identity transform. |

This conversion preserves rendering and interpolation semantics for the
currently supported Lynx operation set, but does not preserve the original CSS
spelling. Code that needs CSS parsing, serialization, or unit resolution
belongs in the core adapter rather than in GFX. In particular, `calc()` values
are resolved by the core adapter using the current reference size and are
resolved again after a relevant size or unit notification.

## Clay integration

Clay resolves its parsed transform lengths at its style boundary and then uses
the same `gfx::TransformOperations` representation for visual transforms.
Direct GFX input is likewise resolved against the current content size and
resolved again when that size changes. Clay's `translateZ` behavior remains a
rendering policy: explicit Z translation controls sibling stacking order while
Clay's primitive visual translation is two-dimensional. The Clay adapter
therefore stores that value separately and keeps the corresponding visual
translate operation at Z zero. An explicit `matrix3d()` remains an opaque
visual 4x4 matrix and is not interpreted as a stacking-order value.

The Clay `perspective` property is also kept outside the operation list because
it is a property applied to the element's transform as a whole, not a parsed
transform function. Matrix decomposition and interpolation in GFX preserve the
full 4x4 result, including perspective, Z translation, three-axis scale, skew,
and quaternion rotation components when a matrix fallback is required.

The current shared operation enum covers the transform function spellings
accepted by both Lynx and Clay. If either renderer later exposes new operations,
such as arbitrary-axis rotation or an explicit three-axis scale function, the
GFX data model, matrix calculation, blending behavior, adapters, and tests must
be extended together. Unsupported operations must not be converted through a
lossy fallback merely to fit the current enum.
