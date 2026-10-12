# Page-level click tracker

Default-on iOS Trail setting `clay_ios_allow_touch_tap_after_movement` gates
whether an isolated page tap stays eligible after moving past the usual
drift threshold. The setting is sampled on touch down and the behavior only
applies to primary single-touch sequences.

`clay_ios_click_tracker_boundary_tolerance_pt` sets the permitted distance
outside each owner edge at tap-up, in Clay logical points. It defaults to
`2`, is clamped to zero when negative, and is sampled with the owner on touch
down. The tolerance has no effect when no tracker is created.

## Owner selection on down

The tracker binds to the deepest hit-chain view that has a `tap` event
binding or an isolated tap gesture recognizer. Icon-to-label swaps inside
the same clickable ancestor keep the same owner. If no clickable ancestor
exists the tracker is not created and the legacy page tap slop applies.

An owner whose ancestor chain mediates native, intercepts gestures, consumes
slide events, or declares a gesture detector keeps the legacy slop path.

## Movement and cancellation

Movement past the inherited tap drift tolerance is swallowed instead of
rejecting the recognizer. A long-press recognition cancels the tracker.
A second concurrent touch, raw cancel, or a tap-ineligible scroll/fling
reject through the existing isolated-gesture-detector path all end the
sequence. Cancellation is sticky for the remainder of the pointer.

## Dispatch

At tap-up `ReportTopViewEvent(kClayEventTypeTap)` checks the tracker. When
present it dispatches to the owner, with coordinates transformed into the
owner's local space, only if the owner is still attached, acceptable, has
no claiming ancestor, and either the current dispatch top view is the owner
or a descendant, or the up coordinate is within the configured tolerance of
the owner bounds while remaining outside the owner itself. A replacement
sibling or overlay inside the owner bounds does not use the tolerance to
retarget the click. Otherwise the tap is suppressed.

## Not covered

- iOS host scrolling, embedded native views and overlays with event-through
  still require device testing.
- Outside-up beyond the configured owner-boundary tolerance suppresses the tap.
