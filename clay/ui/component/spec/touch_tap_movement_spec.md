# Page-level touch tap movement compatibility

`clay_ios_allow_touch_tap_after_movement` defaults to true on iOS and false on
other platforms. Non-iOS production code does not enable this policy even when
the service returns true. Setting false restores the existing page tap drift
threshold. The setting is sampled on touch down for each sequence.

## Scope

- Only the isolated page-level primary touch tap recognizer may bypass drift.
- Component recognizers, mouse, stylus, double tap, long press, drag thresholds,
  and arena priority are unchanged.
- This policy does not create a separate click event or claim UIKit tap parity.
- The normal page threshold still applies when disabled. Its effective value
  can be supplied by page configuration; recognizer fallback is not the same
  as every page's effective threshold.

## Safety contract

When movement exceeds the original tap tolerance, the newly eligible tap must
retain the same attached weak target identity from down through dispatch. A
reported touchmove outside the down target rejects the click unless a later
reported move returns to that target's response chain. To match native iOS
event sampling, touchend does not independently hit-test its terminal
coordinate: down followed directly by an outside up still dispatches to the
down target. Numeric view ID reuse cannot restore eligibility. Small movement
within the original threshold keeps the existing event targeting and bubbling
behavior.

An accepted drag suppresses extended tap after movement beyond its default
threshold in the accepted axis. A sole recognizer's default acceptance on down
does not by itself suppress a stationary tap. Custom or negative drag thresholds,
gesture API handlers, native mediation, and consume-slide/intercept paths retain
the original tap behavior instead of bypassing drift. These exclusions are
checked again at extended tap dispatch if the target or ancestors changed.

Existing scroll-direction, fling-stop, system cancellation, and long-press
guards remain active. A second touch cannot become an extended primary tap.
The down target's lifetime and current gesture policy are validated after raw
touch-end and overlay callbacks, immediately before dispatch to the event
delegate.

With the policy enabled, touch batches complete the main, raw, isolated, and
cleanup pipeline per event before processing the next event. Splitting happens
before physical-coordinate conversion. Disabled batches retain existing order.

## Validation boundaries

Low-level tests cover enabled/disabled behavior, down-time setting snapshots,
small child seams, cross-target movement, outside up without a move,
deletion/ID reuse, touch-end tree mutation, sequential batches, multi-touch,
cancel, mouse, long press, main arena loss, default/custom drag thresholds,
scrolling, and fling-stop suppression.
Source/static validation is not device interaction evidence; iOS host scrolling,
embedded native views, and event-through overlays still require device testing.
