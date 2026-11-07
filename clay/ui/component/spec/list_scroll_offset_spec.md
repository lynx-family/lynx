# List Scroll Offset Spec (Clay ListContainerView)

This document is the normative contract for how the Clay `ListContainerView`
(and its `ScrollView` / `Scrollable` base) tracks scroll position, overscroll
(bounce) displacement, and content-size updates driven by the cross-platform
ListElement layout manager.

It is a spec for the implemented behavior, not a historical design note.
Whenever the offset model, the content-resize base-offset rule, the overscroll
mapping, or the overflow/paint math changes, this document must be updated in
the same change.

Scope:

- the three distinct offset quantities a list scroll container tracks
- the end-to-end linkage from a platform scroll gesture to a ListElement
  relayout and back into Clay
- the base-offset rule inside `UpdateContentOffsetForListContainer` (the
  content-resize entry point) and why it is conditional
- overflow-rect / max-scroll-range semantics and paint-offset application

Out of scope:

- item recycling, anchor selection, and fill logic inside the core
  `ListLayoutManager` (cross-platform, lives under
  `lynx/core/renderer/ui_component/list/`)
- sticky header/footer transform math beyond its interaction with offsets
- snap-scroll target selection policy

## 1. Design Goal

A list scroll container must keep three things consistent at all times:

1. the logical scroll position the ListElement believes the viewport is at,
2. the extra rubber-band displacement produced when the user drags past an
   edge (overscroll / bounce), and
3. the paint offset actually applied to on-screen layers.

The invariant is:

```
normal scrolling:   scroll_offset_ == paint offset,             overscroll_offset_ == 0
during overscroll:  scroll_offset_ == clamped edge (unchanged),  paint offset == edge + rubber-band(overscroll_offset_)
```

Content resizes (load-more, close item, item height change) must preserve this
invariant without adjusting the visible position twice.

## 2. Terms

- `scroll_offset_` (`Scrollable::scroll_offset_`)
  - The Clay logical scroll position. Always clamped by
    `NestedScrollable::DoScroll` to `[0, MaxScroll]` of the current overflow
    rect. This is the value reported back to the ListElement in `DidScroll`.
- `overscroll_offset_` / `clamped_overscroll_offset_` (`Scrollable`)
  - The raw and rubber-band-compressed displacement beyond an edge. Non-zero
    only while dragging past an edge or during a bounce animation.
    `clamped_overscroll_offset_ = RubberBandDistance(overscroll_offset_, ...)`.
- paint offset (`RenderScroll::ScrollLeft/ScrollTop`, read via
  `RenderBox::GetPaintOffsetForScroll`)
  - The offset actually applied when painting the scroll layer. During
    overscroll it is set by `OffsetOverscrollEffect::OnOverscroll` to
    `MaxScroll + clamped_overscroll` (lower edge) or `-clamped_overscroll`
    (upper edge), so it can exceed the logical `scroll_offset_`.
- `max_content_` (`ListContainerView`)
  - The list's content extent along the scroll axis, pushed down by the
    ListElement. Written into the render scroll's overflow rect by
    `SetMaxContent`.
- content-resize delta (`target_content_offset_x/y`)
  - A logical position delta the ListElement computes **relative to the scroll
    offset it last saw** (i.e. relative to the pre-resize offset), passed into
    `UpdateContentOffsetForListContainer`.

## 3. The Three Offset Models

### 3.1 ListElement logical content offset (core layer)

- Owned by `ListLayoutManager` in core; cross-platform.
- Decides which items are on-screen, anchor selection, fill/recycle, and edge
  events.
- Clamped to `[0, max(content_size - viewport_size, 0)]` in core before being
  flushed to the platform.
- Never includes platform bounce displacement.

### 3.2 Clay logical scroll offset (`scroll_offset_`)

- The platform-side mirror of 3.1.
- `NestedScrollable::DoScroll` (`nested_scrollable.cc:306`) always clamps it to
  the current `MaxScrollWidth/Height`.
- `ListContainerView::DidScroll` (`list_container_view.cc:826`) reports it back
  to the ListElement.

### 3.3 RenderScroll paint offset + overscroll

- `Scrollable::DoOverscroll` (`scrollable.cc:51`) stores unconsumed drag
  distance into `overscroll_offset_`; it does **not** move `scroll_offset_`.
- `OffsetOverscrollEffect::OnOverscroll` (`overscroll_effect.cc:9`) maps the
  clamped overscroll into the render scroll's paint offset:
  - lower edge: `paint = MaxScroll + clamped_overscroll`
  - upper edge: `paint = -clamped_overscroll`
- Therefore during overscroll `paint offset != scroll_offset_`. This gap is the
  root reason the content-resize base offset is conditional (§5).

## 4. End-to-End Linkage

### 4.1 Platform gesture to ListElement

```
drag / wheel / fling
  -> NestedScrollManager::DispatchScroll
  -> NestedScrollable::DoScroll        # update clamped scroll_offset_, return unconsumed delta
  -> Scrollable::DoOverscroll          # store overscroll_offset_; OffsetOverscrollEffect updates paint
  -> ListContainerView::DidScroll      # report scroll_offset_ to ListElement (blocked during content update)
  -> ... EngineProxy / ListElement::ScrollByListContainer
  -> ListLayoutManager::ScrollByPlatformContainer
```

### 4.2 ListElement relayout and content-size flush

```
relayout
  -> update on-screen children / anchor / fill / recycle
  -> compute new content_size_
  -> ListAnchorManager::AdjustContentOffsetWithAnchor
  -> flush (content_size, delta = new_logical - pre_resize_logical) to platform
  -> ListContainerView::UpdateContentOffsetForListContainer   # <-- the fix point
```

### 4.3 Clay applies content-size + delta (`UpdateContentOffsetForListContainer`)

Two steps, order is load-bearing:

1. `SetMaxContent(content_size)` (`list_container_view.cc:748`) updates the
   render scroll overflow rect (new max range), then — only if the max
   changed — runs `CorrectScrollOffset`, `StopAnimation`, and
   `ClearOverscrollState`.
2. `OnScrollUpdate(base_offset + delta)` applies the ListElement delta. The
   result is clamped again by `DoScroll` against the freshly-updated max range.

`should_block_did_scroll_` is held true across this method so the intermediate
`DidScroll` calls are not reported to the ListElement as user scrolls.
If growth consumes visible stretch and the final logical offset differs from
the clamped logical offset requested by layout, report that final offset once
to the ListElement. Its fill range otherwise remains at the old logical
position, leaving newly exposed cells blank until another gesture. The
correction carries logical coordinates only; normal growth, shrink, and
same-size updates do not echo layout's own offset. The follow-up same-size
layout must not generate another correction.

## 5. Content-Resize Base-Offset Rule (Normative)

`SetMaxContent` must run **before** `OnScrollUpdate`. Rationale: `DoScroll`
clamps to the *current* overflow rect (`render_box.cc:64` derives max from
`overflow_rect.height() - ClientHeight`). If `OnScrollUpdate` ran first, a
target position that only becomes valid after the content grows (e.g. top
prepend / positive anchor delta) would be clamped against the old, smaller max
and permanently lose that displacement.

Under the three-offset model the logical offset and the overscroll displacement
are tracked separately, so the base offset is composed **explicitly** from
component state rather than read back from the paint offset. The base depends
only on the growth direction:

```cpp
// Snapshot before SetMaxContent(): it overwrites max_content_ and clears the
// overscroll state.
const bool content_is_growing = content_size > max_content_;
const float logical_before  = scroll_offset_.<axis>();
const float overscroll_before = ClampedOverscrollOffset().<axis>();

SetMaxContent(content_size);

// Growing folds the pending overscroll displacement into the base (it becomes
// valid scroll room); shrinking/steady uses the logical offset only. The final
// OnScrollUpdate() -> DoScroll() clamps to the freshly-updated max range.
const float base_offset =
    content_is_growing ? logical_before + overscroll_before : logical_before;
OnScrollUpdate(base_offset + target_content_offset_<axis>);
```

The four-quadrant contract (viewport = 100 examples):

| Overscroll? | Content | Base offset | Why |
|-------------|---------|-------------|-----|
| no  | grow   | logical | nothing to consume (overscroll = 0) |
| no  | shrink | logical | avoid platform clamp + negative delta double-counting |
| yes | grow   | logical + overscroll | fold the pending rubber-band displacement into valid scroll room (load-more must not jump back) |
| yes | shrink | logical | delta is still relative to the pre-resize logical offset; folding overscroll would over-shoot |

Worked cases:

- **Close item (shrink), no overscroll**: content 300->250, max 200->150,
  delta -50. Base = logical 200; 200 + (-50) = **150** (flush to new bottom).
  Folding a clamp to 150 first, then -50, would give 100 and leave a 50px blank
  strip.
- **Load-more (grow) during overscroll**: at bottom (logical 200), pull past
  edge until the visible rubber-band displacement is 50. New data grows content 300->400 (max 300),
  delta 0. Base = logical 200 + overscroll 50 = 250, clamped to new max 300 ->
  **250**, keeping the consumed distance. Using logical only (200) would snap
  back and jump.


`OverscrollOffset()` is the raw gesture distance. The visible stretch is
`ClampedOverscrollOffset()`, after rubber-band compression; during bounce the
animator updates that visible value directly. Only the visible displacement can
become logical scroll room without a jump. For a 100px viewport and a 60px pull,
the visible displacement is about 24.81px, so growth from a 200px logical offset
continues at about 224.81px, not 260px. If less space was added, clamp at the new
edge. Growth at the upper edge must not create a negative logical offset.

During overscroll, an unchanged content extent with a zero platform delta is a no-op for logical,
paint, and overscroll state. Reassert the overflow rectangle, then return without
`OnScrollUpdate`: a zero-delta `DoScroll` still reads the render offset, which
would contaminate the logical offset during a held pull or bounce. A nonzero
anchor adjustment at the same extent retires the old stretch and bounce before
applying the logical delta, so subsequent gestures cannot revive stale stretch.

Anti-patterns (must not be used):

- Always pre-resize offset: regresses load-more during overscroll (jump back).
- Always post-resize offset: regresses close-item (double clamp, blank strip).
- Swapping `SetMaxContent` / `OnScrollUpdate` order: loses growth deltas clamped
  against the stale max (see §5 first paragraph).
- Deleting `CorrectScrollOffset` from `SetMaxContent`: reintroduces
  out-of-range / paint-logical desync after content grows.
- Feeding paint offset back to the ListElement: pollutes logical offset, edge
  events, anchor, and visibility.

## 6. Overflow Rect and Max Scroll Range

- `SetMaxContent` writes the overflow rect as `(0, 0, width, max_content_)`
  (vertical) or `(0, 0, max_content_, height)` (horizontal).
- `RenderBox::MaxScrollHeight/Width` (`render_box.cc:64`) = overflow extent -
  client extent, floored at 0. This is the clamp bound used everywhere.
- `ScrollView::CalculateOverFlow` (`scroll_view.cc:197`) recomputes
  `content_size_` from children for a plain ScrollView, but `ListContainerView`
  overrides it (`list_container_view.cc:743`) to re-assert `max_content_`, so
  the list's content extent is authoritative and equals the ListElement value,
  never expanded by a bounce region.
- Bounce is realized purely through the paint offset / layer translate; the
  list does not add a `BounceView` child, so no rect expansion is needed for
  the overscroll area.

## 7. Invariants to Preserve on Change

1. `SetMaxContent` runs before `OnScrollUpdate` in the content-resize path.
2. Overscroll/`max_content_` inputs are read before `SetMaxContent`.
3. Growth consumes the pre-resize **visible** stretch, never raw finger
   distance or a post-resize paint readback.
4. `scroll_offset_` reported to the ListElement never carries bounce
   displacement.
5. `content_size_` / overflow extent equals the ListElement content size and
   excludes the overscroll region.

## 8. Test Coverage

`list_container_view_unittests.cc` covers:

- vertical/horizontal shrink with a negative delta, and zero-delta clamping;
- exact visible-stretch consumption on both axes, including animated bounce;
- growth smaller than the stretch, and upper-edge growth clamped to zero;
- final logical-position notification after growth consumes stretch, including
  both axes, clamping, and suppression of resize/steady feedback loops;
- repeated same-size/zero-delta updates preserving separate logical and paint
  offsets while the finger holds a pull;
- shrink clearing stretch, with paint equal to the resulting logical offset;
- direction changes resetting both offset axes, clearing overscroll, stopping
  bounce, and synchronizing the component and RenderScroll direction;
- setting the same direction without disturbing the current pull.

Use explicit logical and paint assertions. A loose `old < new <= max` check alone
does not distinguish raw drag distance from visible rubber-band displacement.

## 9. Direction Changes

The list uses `scroll-orientation="vertical" | "horizontal"` to configure both
its core layout manager and platform scroll container. A layout-only legacy
attribute is insufficient to select the Clay gesture axis.

When the direction actually changes, `Scrollable::SetScrollDirection` stops
animations, clears overscroll, resets logical and paint offsets on both axes,
and updates both component and RenderScroll directions before rebuilding drag
recognizers. Reapplying the same direction is a no-op. The next list content
update establishes the range for the new axis; old-axis offsets must not leak
into its gestures or raster scrolling.

## 10. Bounce Property

`ListContainerView` delegates `bounces` and its `bounce` alias to `ScrollView`.
The inherited platform policy applies: ScrollView ignores these attributes on
Android and handles them on other platforms. This offset model does not change
that policy.
