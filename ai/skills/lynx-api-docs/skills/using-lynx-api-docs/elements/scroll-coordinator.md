# Lynx scroll-coordinator Element

`<scroll-coordinator>` is the public native element for coordinated header and slot scrolling. Use it when a page needs a collapsible header area plus a scrollable content slot that move together.

## Structure

Use the public child tags for the authored tree:

```xml
<scroll-coordinator bindoffset="onOffset">
  <scroll-coordinator-toolbar />
  <scroll-coordinator-header />
  <scroll-coordinator-slot>
    <scroll-view />
  </scroll-coordinator-slot>
  <scroll-coordinator-slot-drag />
</scroll-coordinator>
```

The header and slot are the structural children. Keep normal page content inside `<scroll-coordinator-slot>`.

## Core Props

| Prop | Platforms | Description |
| --- | --- | --- |
| `enable-scroll` | Android, iOS, Harmony | Enables or disables coordinator scrolling |
| `scroll-enable` | Android, iOS, Harmony | Alias for the scroll gate |
| `granularity` | Android, iOS, Harmony | Controls how often offset changes are emitted |
| `header-over-slot` | Android, iOS, Harmony | Places the header over the slot area |

## Child Props

| Prop | Child | Platforms | Description |
| --- | --- | --- | --- |
| `enable-drag` | `scroll-coordinator-slot-drag` | Android, iOS, Harmony | Enables the drag affordance |
| `android-consume-gesture` | `scroll-coordinator-header` | Android | Consumes gestures on the Android header path |

## Platform Props

Android:

- `compat-container-popup`
- `tab-movable-enable`
- `toolbar-interaction-enable`
- `header-scrollview-enable`
- `experimental-header-slot-overflow-hit-test`
- `android-enable-touch-stop-fling`
- `android-header-over-slot`
- `android-nested-scroll-as-child`
- `android-header-tap-slop`

iOS:

- `bounces`
- `allow-vertical-bounce`
- `enable-scroll-bar`
- `scroll-bar-enable`
- `refresh-mode`
- `ios-force-scroll-detach`
- `ios-scroll-view-filter`
- `ios-scroll-exclude`
- `ios-top-padding-for-native`
- `experimental-header-slot-overflow-hit-test`
- `ios-scrolls-to-top`

Harmony supports the core props and the public child tags. Set `header-over-slot` before first render on Harmony.

## Methods

| Method | Platforms | Description |
| --- | --- | --- |
| `setFoldExpanded` | Android, iOS, Harmony | Expands or collapses the coordinator header |
| `scrollBy` | Android, iOS, Harmony | Scrolls by a relative offset |
| `getScrollInfo` | Android, iOS | Returns current scroll information through the callback |

## Events

| Event | Platforms | Payload |
| --- | --- | --- |
| `offset` | Android, iOS, Harmony | `offset`, `height` |

`offset` reports the current coordinator offset. `height` reports the coordinated header height.

## Usage Notes

- Use `<scroll-view>` for a plain scroll area; use `<scroll-coordinator>` when the header and slot need coordinated movement.
- Keep `scroll-coordinator-*` children in their expected roles so platform child dispatch remains predictable.
- Treat Android tuning props as Android-only unless another platform is explicitly listed.
- `getScrollInfo` is not a Harmony method in the current native implementation.
