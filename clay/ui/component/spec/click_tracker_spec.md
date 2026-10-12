# Page-level click tracker

Default-on iOS Trail setting `clay_ios_allow_touch_tap_after_movement` gates
whether an isolated page tap stays eligible after moving past the usual
drift threshold. The setting is sampled on touch down and the behavior only
applies to primary single-touch sequences.

The tracker is compiled into iOS production builds and unit-test builds.
Other production platforms retain the original tap and gesture dispatch paths.

The first touch starts the sequence even when no clickable owner is found. A
concurrent touch cancels an existing tracker, and a new single-touch sequence
is eligible only after every touch in the current sequence ends.
At most one tracker exists, independently of the set of active touch pointers.

## Owner selection on down

The tracker binds to the deepest hit-chain view that has a `tap` event
binding or an isolated tap gesture recognizer. Icon-to-label swaps inside
the same clickable ancestor keep the same owner. If no clickable ancestor
exists the tracker is not created and the legacy page tap slop applies.

An owner whose ancestor chain mediates native, intercepts gestures, consumes
slide events, or declares a gesture detector keeps the legacy slop path.
The same exclusion applies to hit descendants between the pointer target and
the owner. The current hit target's ancestor chain is checked again at tap-up.

## Movement and cancellation

Move events for an eligible sequence do not reject the isolated tap recognizer.
Movement is handled by a page-specific recognizer; the shared tap recognizer
retains its original drift threshold.
A long-press recognition cancels the tracker.
The cancellation notification does not change native gesture mediation state,
including when the setting is disabled.
A second concurrent touch, raw cancel, or a tap-ineligible scroll/fling
reject through the existing isolated-gesture-detector path all end the
sequence. Cancellation is sticky for the remainder of the pointer.
Page reset cancels tracked isolated touch sequences before clearing trackers,
so an old pointer cannot dispatch a tap into the replacement view tree.

Touch batches are processed one event at a time while the setting is enabled
or a tracker exists. Each event completes main gesture handling, raw event
dispatch, isolated gesture handling, and cleanup before the next event starts.

When the main gesture arena accepts a recognizer for the tracked pointer, the
page tap itself and `kNone` keep the tracker. Long press and drag recognizers
cancel it permanently because they own the sequence, even if
the pointer later re-enters the owner.

## Dispatch

At tap-up `ReportTopViewEvent(kClayEventTypeTap)` checks the tracker. When
present it dispatches to the owner, with coordinates transformed into the
owner's local space, only if the owner is still attached, acceptable, has
no claiming ancestor, and the current dispatch top view is the owner or a
descendant. This reuses normal hit testing, including hit slop. A replacement
sibling or overlay does not retarget the click. Otherwise the tap is
suppressed.
Owner coordinates use the position converted by normal hit testing, including
overlay offsets for both overlay descendants and pass-through targets.

## Not covered

- iOS host scrolling, embedded native views and overlays with event-through
  still require device testing.
- An up outside the owner's current hit-tested subtree suppresses the tap.
