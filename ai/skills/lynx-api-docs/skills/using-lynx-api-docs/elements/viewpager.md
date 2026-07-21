# Lynx viewpager Element

This guide covers the public `<viewpager>` element and its `<viewpager-item>` child.

## When to Use `<viewpager>`

Use `<viewpager>` when you need a native horizontal pager with a small, known set of pages, page-change events, and programmatic page selection through `selectTab`.

Do not use `<viewpager>` when you need long scrolling content, virtualized lists, or platform-specific pager integrations that are not listed here.

## Quick Start

```tsx
<viewpager
  id="pager"
  initial-select-index={0}
  enable-scroll={true}
  bindwillchange={handleWillChange}
  bindchange={handleChange}
  bindoffsetchange={handleOffsetChange}
  style={{ width: '100%', height: '320px' }}
>
  <viewpager-item>
    <view style={{ width: '100%', height: '100%' }} />
  </viewpager-item>
  <viewpager-item>
    <view style={{ width: '100%', height: '100%' }} />
  </viewpager-item>
</viewpager>
```

Switch pages imperatively:

```ts
this.getNodeRef('#pager').invoke({
  method: 'selectTab',
  params: { index: 1, smooth: true },
});
```

## Child Structure

Wrap each page in `<viewpager-item>`.

```ttml
<viewpager>
  <viewpager-item>...</viewpager-item>
  <viewpager-item>...</viewpager-item>
</viewpager>
```

## Properties

### Core props

| Prop | Type | What It Does |
| --- | --- | --- |
| `initial-select-index` | `number` | Chooses the initial page on first render |
| `select-index` | `number` | Marks a page through prop updates; prefer `selectTab` for runtime page changes |
| `enable-scroll` | `boolean` | Enables or disables page dragging |
| `allow-horizontal-gesture` | `boolean` | Alternate horizontal gesture gate in current native code |
| `background` | `string` | Android public pager background color |

### Partial and platform-specific props

| Prop | Platforms | What It Does |
| --- | --- | --- |
| `bounces` | iOS, Harmony | Enables bounce or spring edge behavior |
| `single-page-touch` | Android, iOS | Restricts a drag to one page transition |
| `ios-single-page-touch` | iOS | iOS alias for single-page drag behavior |
| `keep-item-view` | Android, iOS | Keeps page views attached more aggressively |
| `enable-nested-scroll` | Harmony | Enables Harmony nested-scroll handling during drag |
| `page-change-animation` | Android, iOS | Android wires this to page changes; iOS stores it, so do not rely on it for portable no-animation behavior |
| `android-force-can-scroll` | Android | Keeps edge gestures from falling through to parents |
| `android-always-overscroll` | Android | Enables Android edge overscroll |
| `android-distinguish-swipe-tap` | Android | Avoids tap recognition after swipe motion |
| `ios-gesture-direction` | iOS | Enables simultaneous gesture handling with nested horizontal scroll views at edges |
| `ios-gesture-offset` | iOS | Requires the pan to begin beyond a leading-edge threshold |
| `ios-not-reload-items` | iOS | Avoids item reload after layout in keyboard-sensitive cases |
| `ios-recognized-gesture-class` / `ios-recognized-view-tag` | iOS | Allows a specific gesture or view tag to recognize simultaneously with the pager |
| `experimental-pager-change-epsilon` | iOS | Tunes the float epsilon used for change detection |

## Events

| Event | Platforms | Payload | Notes |
| --- | --- | --- | --- |
| `bindchange` | Android, iOS, Harmony | `index`, `isDragged` | Fires when the current page changes |
| `bindwillchange` | Android, iOS, Harmony | `index`, `isDragged` | Fires before the pager settles on a page |
| `bindoffsetchange` | Android, iOS, Harmony | `offset` | Stringified fractional scroll progress |

## UI Methods

### `selectTab`

Parameters:

- `index: number`
- `smooth?: boolean`
- `allowTempChanges?: boolean`, iOS only

Platforms:

- Android
- iOS
- Harmony

### `setDragGesture`

Parameters:

- `canDrag: boolean`

Platforms:

- Android
- iOS

## Platform Availability Matrix

| Feature | Android | iOS | Harmony |
| --- | --- | --- | --- |
| `change` / `willchange` / `offsetchange` | Yes | Yes | Yes |
| `selectTab` | Yes | Yes | Yes |
| `setDragGesture` | Yes | Yes | No |
| `bounces` | No | Yes | Yes |
| `single-page-touch` | Yes | Yes | No |
| `keep-item-view` | Yes | Yes | No |
| `enable-nested-scroll` | No | No | Yes |

## Safe Guidance

- use `selectTab` for runtime page changes
- prefer `enable-scroll` for portable code
- keep each page inside `<viewpager-item>`
- do not assume Android-only or iOS-only props are portable
- do not use `<viewpager>` as a replacement for large virtualized lists
