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

## 5. Content-Resize Base-Offset Rule (Normative)

`SetMaxContent` must run **before** `OnScrollUpdate`. Rationale: `DoScroll`
clamps to the *current* overflow rect (`render_box.cc:64` derives max from
`overflow_rect.height() - ClientHeight`). If `OnScrollUpdate` ran first, a
target position that only becomes valid after the content grows (e.g. top
prepend / positive anchor delta) would be clamped against the old, smaller max
and permanently lose that displacement. `CorrectScrollOffset` cannot recover it
because it only reads the paint offset, and the lost delta never became paint.

The delta base offset is **conditional** on overscroll state and growth
direction. `UpdateContentOffsetForListContainer` snapshots the inputs *before*
`SetMaxContent` (which clears overscroll state and overwrites `max_content_`):

```cpp
const bool was_under_overscroll = IsUnderOverscroll();
const bool content_is_growing   = content_size > max_content_;   // max_content_ still old here
const FloatPoint logical_offset_before_resize = scroll_offset_;

SetMaxContent(content_size);

const FloatPoint offset_for_delta =
    (was_under_overscroll && content_is_growing)
        ? scroll_offset_                 // post-resize
        : logical_offset_before_resize;  // pre-resize

OnScrollUpdate(offset_for_delta.<axis>() + target_content_offset_<axis>);
```

The four-quadrant contract (viewport = 100 examples):

| Overscroll? | Content | Base offset | Why |
|-------------|---------|-------------|-----|
| no  | grow   | pre-resize  | nothing to consume; pre/post usually equal |
| no  | shrink | pre-resize  | avoid platform clamp + negative delta double-counting |
| yes | grow   | **post-resize** | `CorrectScrollOffset` promoted the out-of-range paint displacement into a valid `scroll_offset_`; must keep it (load-more must not jump back) |
| yes | shrink | pre-resize  | delta is still relative to the pre-resize logical offset; using the clamped value would double-count |

Worked cases:

- **Close item (shrink), no overscroll**: content 300->250, max 200->150,
  delta -50. Pre-resize base 200 + (-50) = **150** (flush to new bottom).
  Using post-resize (already clamped to 150) + (-50) = 100 would leave a 50px
  blank strip. -> pre-resize.
- **Load-more (grow) during overscroll**: at bottom (scroll 200), pull past
  edge by 50 (paint ~= 221 after rubber band). New data grows content 300->400
  (max 300), delta 0. `SetMaxContent` -> `CorrectScrollOffset` promotes the
  paint displacement into `scroll_offset_` (> 200). Post-resize base + 0 keeps
  the consumed distance; pre-resize base (200) + 0 would snap back and jump.
  -> post-resize.

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
3. Only the `overscroll && growth` quadrant uses the post-resize offset.
4. `scroll_offset_` reported to the ListElement never carries bounce
   displacement.
5. `content_size_` / overflow extent equals the ListElement content size and
   excludes the overscroll region.

## 8. Test Coverage

`list_container_view_unittests.cc` covers the four-quadrant contract:

- vertical/horizontal shrink + negative delta (no double clamp)
- shrink + zero delta (clamp still applies)
- lower-overscroll + growth + zero delta (load-more consumes overscroll, does
  not jump back)
- lower-overscroll + shrink + negative delta (no double clamp under overscroll)

Overscroll is simulated with `SetOverscrollEnabled(true)` +
`SetOverscrollOffset(...)`. Because rubber-band compression is non-linear,
overscroll-growth assertions check the consumed direction (result strictly
greater than the pre-resize offset and within the new max), not an exact pixel.
