# Page-level click tracker

`clay_ios_allow_touch_tap_after_movement` defaults to true on iOS and false on
other platforms. Non-iOS production code does not enable the tracker even when
the service returns true. Setting false restores the existing page tap drift
threshold. The setting is sampled on touch down for each sequence.

## Scope

- Only the isolated page-level primary touch tap recognizer is affected.
- Component recognizers, mouse, stylus, double tap, long press, drag
  thresholds, and arena priority are unchanged.
- The tracker does not create a separate click event or claim UIKit tap
  parity. It gates whether the existing `tap` event dispatches after movement.
- The normal page threshold still applies when the tracker is disabled.

## Design

A touch down binds to the nearest clickable ancestor on the hit chain, the
"owner". Clickable means the view has a `tap` event binding or an isolated
tap gesture recognizer. If no clickable ancestor exists the tracker is not
created and the legacy slop reject applies as before.

Movement does not reject the recognizer or close the sequence. Up-time runs
the owner through a policy recheck plus an "inside" check against the current
dispatch target, then dispatches the tap event to the owner.

### Owner selection on down

- Device must be `kTouch`.
- Owner is the first non-anonymous hit-chain entry that has a tap event or an
  isolated tap gesture recognizer.
- A hit-chain entry that passes events to native, intercepts gestures,
  consumes slide events, has a drag recognizer with a non-default slop, or a
  non-empty gesture detector map disables the tracker for this sequence.
- The same disable list is also walked up the owner's ancestor chain, so an
  ancestor becoming native-mediated prevents the tracker from forming.
- A second concurrent touch marks any existing tracker as cancelled and
  prevents a tracker from forming for the new pointer in multi-touch
  sequences. Only the `active_touch_pointer_id_` sequence may track.

### Movement

- The tracker records axis-specific drag acceptance via the gesture-accepted
  listener. Default-threshold drag acceptance on down alone does not cancel;
  movement past the default slop in the accepted axis does.
- Long-press acceptance cancels immediately.
- Explicit `kCancel` events cancel.
- Cancellation is sticky for the remainder of the sequence.

### Dispatch

At tap-up, the recognizer calls `ReportTopViewEvent(event, kClayEventTypeTap)`.
`ReportTopViewEvent` then:

1. If `HasClickTracker(pointer_id)` is false, keeps the legacy top-view based
   dispatch unchanged.
2. If the tracker is cancelled, returns without dispatching.
3. If the owner is detached, fails `CanAcceptEvent`, or any ancestor now
   claims the pointer (native, consume-slide, intercept, non-default drag),
   returns without dispatching.
4. If the current dispatch top view is not the owner or a descendant,
   returns without dispatching. Returning inside the owner later in the
   sequence restores eligibility.
5. Otherwise, dispatches `tap` to the owner, with position transformed into
   owner-relative coordinates.

### Pointer sequence boundaries

With the tracker enabled, the dispatcher completes the main / raw / isolated
/ cleanup pipeline per event before processing the next event. Splitting
happens before physical-coordinate conversion. The disabled path keeps the
previous batch ordering.

## Validation boundaries

Unit tests cover enabled/disabled behavior, down-time setting snapshots, inner
child swaps inside the same owner, outside-then-back-inside returning the tap,
owner deletion / ID reuse, raw touch-end tree mutation, sequential batches,
multi-touch, cancel, mouse, long press, main arena loss, default/custom drag
thresholds, scrolling, and fling-stop suppression. Source/static validation
is not device interaction evidence; iOS host scrolling, embedded native
views, and event-through overlays still require device testing.
