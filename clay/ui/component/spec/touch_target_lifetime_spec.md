# Touch Target Lifetime

Raw touch delivery is owned by `PageView` and the event delegate. The contract
applies to every platform using the shared Clay component implementation.

- On touch start, capture weak references to the hit target and its ancestors.
- Subsequent raw events use the original target while it remains attached.
  Otherwise, use the nearest attached, non-anonymous ancestor from the captured
  path. Do not hit test again or select a replacement with the same view ID.
- Recompute local coordinates for the selected recipient; preserve page
  coordinates and the physical pointer ID.
- End and cancel remove the physical contact even if its original target has
  been detached or destroyed. The surviving recipient lets the delegate finish
  the corresponding upper-layer touch state.
- Removing one target does not synthesize a stream-wide cancel or end another
  physical contact. Explicit cancellation retains the delegate's existing
  cancellation semantics.
- Single-touch compatibility keeps the first contact's target path until the
  last physical contact ends, including when the driving pointer changes.
- Embedded native views still receive raw events for their own live physical
  targets before single-touch compatibility coalesces delegate notifications.
- Captured paths do not keep views alive and are cleared when contacts end or
  the page is reset. The page itself remains a valid final ancestor during an
  active page session.
- A delegate that retains target IDs for cancellation must update them when raw
  delivery falls back to a surviving ancestor.
- Before dispatching the first stream-wide cancellation, refresh the retained
  targets of all active contacts from their captured paths. This must not depend
  on intervening move events or on the order in which contacts are cancelled.
  Shared surviving ancestors receive one cancellation, and subsequent physical
  terminal events only drain the already-cancelled stream.

Generated tap/long-press recognition is separate from raw touch completion;
this fallback does not change its target-validation policy.
