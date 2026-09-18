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

The literal uniform-radius predicate remains distinct because image processing has different requirements. Radius and selected corners are updated together, including resets when returning to complex rendering. Changes in bounds or mask state can change renderer eligibility without changing CSS properties, so layer placement and renderer type are reconciled before reuse. Border painting deferred at zero size remains pending until it can actually run. The first nonzero layout must apply it even if the layer arrangement already matches; subsequent size changes alone must not reapply unchanged native border widths and colors. Borderless child pages can share a frame element's backing view: their size-only updates must not write a zero-width border over the host's border. This includes W3C defaults with a positive nominal width but `none` style. Explicit border-property changes still repaint or remove the border.

## Common layout cost

Zero and uniform radii retain the inexpensive legacy route. Partial-corner eligibility and frame-specific policy must not add native corner writes to that common clipping route. State previously applied by partial-corner optimization must still be reset on transitions or view replacement. Painting reuses eligibility within an application rather than introducing a cross-layout geometry cache; external masks and bounds can change independently of CSS flags.

## Boundary

Frame views retain path-based clipping and managed painting for partial radii because their backing layer is shared with a child page whose own corner updates can overwrite native host geometry. This restriction applies to both clipping and painting eligibility; uniform-radius legacy behavior is unchanged. The shared-layer predicate lives on LynxUI as `hasSharedBackingLayer`: only frame elements and embedded page roots may return YES, because they are the only UIs whose view can be a frame view; every other UI keeps the default NO. A UI that newly hosts a frame view must override this predicate, or the restriction silently stops applying.

Simple borders retain the existing width, color and style constraints; newly eligible borders requiring edge-width normalization keep complex painting. Simple backgrounds exclude background clips, images/gradients and inset shadows. Partial-radius painting does not migrate onto a view with clip-path, a layer mask or mask drawables, because that would change the clipping of sibling paint.

LynxUI owns overflow clipping and clip-path precedence. Native partial-radius clipping must not replace externally owned masks or change scroll-mask viewport positioning. Sibling painting layers remain unclipped so their outlines and outer shadows survive. Background-image masks and shadow geometry keep their existing rendering paths.

When an external mask takes over native partial-radius state, the UI clears native clipping and reconciles view-hosted paint onto managed sibling layers before returning (including scroll views and visible overflow). The manager tracks the layer to which it applied native nonuniform-radius state, including zero-axis radii that render as square corners; this ownership record, not the selected corner bits, mutable `clipsToBounds` flag or native radius, determines whether painting needs reconciliation. External owners may reset either native clipping property before installing their mask. Background effect application also initiates this cleanup when only border widths change or a redraw occurs without dirty radius/background flags. Subsequent scroll-mask updates do not repeat the paint reconciliation. Uniform-radius external-mask behavior remains unchanged. Background effect application suppresses reentry from its own UI mask-update callback; the outer application performs the pending paint reconciliation.
