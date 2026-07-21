# Lynx scroll-view Element

This guide covers the default native Android, iOS, and Harmony `<scroll-view>` element behavior.

## When to Use `<scroll-view>`

### Use `<scroll-view>` when:

- you need a scrollable container for a known, bounded set of child elements
- content overflows its container and the user needs to scroll to see it
- you need programmatic scroll control such as `scrollTo`, `scrollBy`, or `autoScroll`
- you need threshold-based events such as "load more when near bottom" with small datasets
- you need edge events or bounce effects on the default native path

### Do not use `<scroll-view>` when:

- you have a large or dynamic list of items; use `<list>` instead for virtualized rendering
- you only need a static layout that fits within the viewport
- you need paging or swipe between full-screen pages; use a pager element instead

### `<scroll-view>` and Page-Level Scrolling

In Lynx, `<page>` defaults to `overflow: hidden` and does not scroll by itself. Therefore, when migrating a long Web page that relies on body scroll, you usually need to wrap the content in a `<scroll-view>` to achieve equivalent page-level scrolling.

Note that moving content into a `<scroll-view>` changes the scrolling context, which can affect the reference frame for `position: fixed` and the stacking behavior of `z-index`. If the original page contains fixed-position elements or complex overlay layers, reassess their behavior inside a `<scroll-view>`.

### Choosing between `<scroll-view>` and `<list>`

| Scenario | Use |
| --- | --- |
| Fewer than 50 items with mixed layouts | `<scroll-view>` |
| Hundreds or thousands of items | `<list>` |
| Known bounded child set | `<scroll-view>` |
| Infinite or virtualized feed | `<list>` |

## Quick Start

### Vertical scroll

```tsx
<scroll-view
  scroll-orientation="vertical"
  bindscroll={onScroll}
  style={{ height: '500px' }}
>
  {/* child content */}
</scroll-view>
```

### Horizontal scroll

```tsx
<scroll-view
  scroll-orientation="horizontal"
  style={{ width: '100%', height: '200px' }}
>
  {/* horizontal content */}
</scroll-view>
```

## Implementation Path Notes

The tables below describe the default `<scroll-view>` path.

Some hosts can pre-switch `<scroll-view>` to a current new-arch native path through the internal `scroll-view-new-arch` selector. Treat that as implementation routing, not as a second element tag to author directly.

Known current deltas when that switch is enabled:

- `bindscrollstatechange` becomes available.
- `bindscrollstart` also fires on iOS.
- `bindcontentsizechanged`, edge events, and bounce events stay on the default path and are not exposed on the current new-arch path.
- current new-arch registrations use alternate native prop names such as `enable-scroll-bar`, `forwards-nested-scroll`, `backwards-nested-scroll`, `initial-scroll-index`, and `scroll-event-throttle`.
- `getScrollInfo` and `scrollBy` callback payloads are not identical across the default path and the current new-arch path.
- `takeContentScreenshot` is implemented on both the default path and the current new-arch path.

## Properties

### Core props

| Prop | Type | Default | What It Does |
| --- | --- | --- | --- |
| `scroll-orientation` | `"vertical"` \| `"horizontal"` | `"vertical"` | Sets the scroll direction |
| `enable-scroll` | `boolean` | `true` | Enables or disables user scrolling |
| `scroll-bar-enable` | `boolean` | `false` | Shows scroll bar indicators on the default path |
| `upper-threshold` | `number` | `0` | Distance from the top or left edge to trigger `scrolltoupper` |
| `lower-threshold` | `number` | `0` | Distance from the bottom or right edge to trigger `scrolltolower` |
| `scroll-top` / `scroll-left` | `number` | — | Default-path prop form for programmatic scroll position |
| `scroll-to-index` | `number` | — | Default-path prop form for scrolling to the Nth child |
| `initial-scroll-offset` | `number` | `0` | One-time initial scroll position on first render |
| `initial-scroll-to-index` | `number` | — | One-time initial scroll to the Nth child on first render |
| `enable-nested-scroll` | `boolean` | `false` | Enables nested scroll coordination on the default path |

### Platform-specific props

| Prop | Platforms | What It Does |
| --- | --- | --- |
| `bounces` | iOS, Harmony | Enables elastic over-scroll spring effect |
| `fading-edge-length` | iOS, Android | Adds a fading gradient at scroll edges |
| `force-can-scroll` | iOS, Android | Forces scroll gesture consumption and blocks parent scrollables |
| `nested-scroll-forward-options` | Harmony | Nested mode: `selfOnly`, `selfFirst`, `parentFirst`, or `parallel` |
| `nested-scroll-backward-options` | Harmony | Same modes for backward direction |
| `scroll-x-reverse` / `scroll-y-reverse` | iOS | Reverses scroll direction and starts at the far end |
| `ios-block-gesture-class` | iOS | Used with `force-can-scroll` to specify a native class to block |
| `ios-recognized-view-tag` | iOS | Used with `force-can-scroll` to specify a UIView tag to block |
| `scroll-tap` | Android | Enables tap-to-stop during fling |
| `forbid-fling-focus-change` | Android | Prevents focus changes during fling |
| `block-descendant-focusability` | Android | Blocks child views from receiving focus |
| `enable-new-nested` | Android | Enables the newer nested scrolling implementation |
| `android-touch-slop` | Android | `"paging"` mode uses a larger touch threshold |
| `android-preference-consume-gesture` | Android | Prefers consuming gestures inside the scroll view |

> Android does not expose a portable spring-bounce API for the default `<scroll-view>` path, so `bounces` is not available there.

### Deprecated props

| Deprecated | Use Instead |
| --- | --- |
| `scroll-x` | `scroll-orientation="horizontal"` |
| `scroll-y` | `scroll-orientation="vertical"` |

## Events

### Portable payload fields

The stable `event.detail` fields you can safely depend on are:

| Field | Description |
| --- | --- |
| `scrollLeft` | Current horizontal offset |
| `scrollTop` | Current vertical offset |
| `scrollWidth` | Total content width |
| `scrollHeight` | Total content height |
| `deltaX` | Frame-over-frame horizontal change |
| `deltaY` | Frame-over-frame vertical change |

Path-specific diagnostic fields such as `isDragging` and `scrollState` can differ by runtime path and should not be treated as portable.

### Event bindings on the default path

| Event Binding | Platforms | When It Fires |
| --- | --- | --- |
| `bindscroll` | All | Continuously during scrolling |
| `bindscrolltoupper` | All | Scroll enters the upper or left threshold zone |
| `bindscrolltolower` | All | Scroll enters the lower or right threshold zone |
| `bindscrollend` | All | All scrolling motion, including inertia, has stopped |
| `bindscrollstart` | Android, Harmony | Scrolling begins |
| `bindcontentsizechanged` | All | Content dimensions change |
| `bindscrolltoupperedge` | All | At the very top or left edge |
| `bindscrolltoloweredge` | All | At the very bottom or right edge |
| `bindscrolltonormalstate` | All | Leaves the edge region |
| `bindscrolltobounce` | iOS, Harmony | Scroll exceeds bounds into the bounce area |

When the current new-arch path is enabled, native code also emits `bindscrollstart` and `bindscrollstatechange` on iOS, Android, and Harmony, but it does not expose `bindcontentsizechanged`, edge events, or bounce events there.

### Event example

```tsx
const onScroll = (event) => {
  const { scrollTop, deltaY } = event.detail;
  console.log(`position: ${scrollTop}, delta: ${deltaY}`);
};

const onReachBottom = () => {
  loadMore();
};

<scroll-view
  scroll-orientation="vertical"
  lower-threshold={100}
  bindscroll={onScroll}
  bindscrolltolower={onReachBottom}
>
  {/* content */}
</scroll-view>;
```

## UI Methods

### `scrollTo`

Scrolls to an absolute position or a specific child index.

| Parameter | Type | Description |
| --- | --- | --- |
| `offset` | `number` | Offset relative to the target |
| `index` | `number` | Target child index |
| `smooth` | `boolean` | Whether to animate the scroll |

### `scrollBy`

Scrolls by a relative delta from the current position.

| Parameter | Type | Description |
| --- | --- | --- |
| `offset` | `number` | Relative scroll distance |

On the default path, the callback returns `{ consumedX, consumedY, unconsumedX, unconsumedY }`. Do not assume the current new-arch path returns the same payload.

### `autoScroll`

Starts or stops continuous auto-scrolling.

| Parameter | Type | Description |
| --- | --- | --- |
| `rate` | `number` | Speed in px/sec; positive is forward, negative is backward |
| `start` | `boolean` | `true` to start, `false` to stop |
| `autoStop` | `boolean` | Harmony default path only; auto-stop at edge |

### `getScrollInfo`

Returns the current scroll position.

On the default path, the callback returns `{ scrollX, scrollY, scrollRange }`.

If your host enables the current new-arch path, native code returns `{ scrollLeft, scrollTop, scrollWidth, scrollHeight }` instead. Do not assume one universal shape across runtime paths.

### `takeContentScreenshot`

Captures the full scrollable content, not just the visible viewport, and returns `{ width, height, data }`.

| Parameter | Type | Description |
| --- | --- | --- |
| `format` | `"jpeg"` \| `"png"` | Output image format; defaults to `"jpeg"` |
| `scale` | `number` | Output scale; defaults to `1` |

## Common Patterns

### Infinite scroll

```tsx
<scroll-view
  scroll-orientation="vertical"
  lower-threshold={100}
  bindscrolltolower={() => loadMore()}
  style={{ height: '100%' }}
>
  {items.map((item) => <ItemCard key={item.id} data={item} />)}
</scroll-view>
```

### Scroll to top

```tsx
const scrollToTop = () => {
  scrollRef.current?.invoke({
    method: 'scrollTo',
    params: { offset: 0, index: 0, smooth: true },
  });
};
```

### Bounce-triggered refresh on the default path

```tsx
<scroll-view
  scroll-orientation="vertical"
  bounces={true}
  bindscrolltobounce={(event) => {
    if (event.detail.direction === 'top') {
      triggerRefresh();
    }
  }}
>
  {/* content */}
</scroll-view>
```

## Platform Availability Matrix

The table below describes the default path:

| Feature | iOS | Android | Harmony |
| --- | --- | --- | --- |
| `scroll-orientation` | Yes | Yes | Yes |
| `bounces` | Yes | No | Yes |
| `enable-scroll` | Yes | Yes | Yes |
| `enable-nested-scroll` | Yes | Yes | Yes |
| `nested-scroll-*-options` | No | No | Yes |
| `force-can-scroll` | Yes | Yes | No |
| `scrollTo` / `scrollBy` | Yes | Yes | Yes |
| `autoScroll` | Yes | Yes | Yes, plus `autoStop` |
| `bindscrollstart` | No | Yes | Yes |
| `bindscrollstatechange` | No | No | No |
| `bindcontentsizechanged` | Yes | Yes | Yes |
| `bindscrolltoupperedge` | Yes | Yes | Yes |
| `bindscrolltoloweredge` | Yes | Yes | Yes |
| `bindscrolltonormalstate` | Yes | Yes | Yes |
| `bindscrolltobounce` | Yes | No | Yes |

If your host enables the current new-arch path, re-check path-specific deltas before depending on `bindscrollstatechange`, edge events, bounce events, or callback payload shapes.
