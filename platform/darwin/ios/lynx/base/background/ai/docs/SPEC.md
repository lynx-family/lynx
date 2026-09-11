---
id: ios-background-rendering
type: submodule-design
status: active
title: iOS background and border rendering
---

## Responsibility

Background rendering coordinates border and background painting on the view or managed sibling layers. Native CALayer painting avoids path/image rendering only when it preserves the resolved geometry and existing stacking behavior.

## Native rounded corners

Clipping and painting share a private eligibility calculation over resolved, overlap-normalized radii and untransformed bounds. On iOS 11 and later, square corners may be mixed with circular corners sharing one radius. A corner with either zero axis is square, matching the path renderer. Newly eligible partial radii larger than half the shorter bounds dimension retain path rendering; they must not be truncated to enable this optimization. Uniform radii retain the legacy availability and clamping behavior.

Radius resolution and overlap normalization retain their existing size-change or force-redraw lifecycle. A size-preserving property update can leave partial radii unnormalized, so native eligibility also rejects edge sums that exceed the bounds, including the nonzero axis of a square corner. Path rendering remains in use until the geometry qualifies.

The literal uniform-radius predicate remains distinct because image processing has different requirements. Radius and selected corners are updated together, including resets when returning to complex rendering. Changes in bounds or mask state can change renderer eligibility without changing CSS properties, so layer placement and renderer type are reconciled before reuse.

## Boundary

Simple borders retain the existing width, color and style constraints; newly eligible borders requiring edge-width normalization keep complex painting. Simple backgrounds exclude background clips, images/gradients and inset shadows. Partial-radius painting does not migrate onto a view with clip-path, a layer mask or mask drawables, because that would change the clipping of sibling paint.

LynxUI owns overflow clipping and clip-path precedence. Native partial-radius clipping must not replace externally owned masks or change scroll-mask viewport positioning. Sibling painting layers remain unclipped so their outlines and outer shadows survive. Background-image masks and shadow geometry keep their existing rendering paths.

When an external mask takes over native partial-radius clipping, the UI clears native clipping and reconciles view-hosted paint onto managed sibling layers before returning (including scroll views). Background effect application also initiates this cleanup when only border widths change or a redraw occurs without dirty radius/background flags. Subsequent scroll-mask updates do not repeat the paint reconciliation. Uniform-radius external-mask behavior remains unchanged. Background effect application suppresses reentry from its own UI mask-update callback; the outer application performs the pending paint reconciliation.
