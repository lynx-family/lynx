# Lynx refresh Element

This guide covers the public `<refresh>` element and its `<refresh-header>` child.

## When to Use `<refresh>`

Use `<refresh>` when you need a native pull-down refresh container around scrollable content.

Do not use `<refresh>` when you need footer loadmore behavior, multiple arbitrary content slots, or exact cross-platform ordering between release and refresh events.

## Quick Start

```tsx
<refresh
  id="feedRefresh"
  enable-refresh={true}
  bindstartrefresh={handleRefresh}
  bindheaderoffset={handleHeaderOffset}
  bindrefreshstatechange={handleStateChange}
  style={{ width: '100%', height: '100%' }}
>
  <refresh-header>
    <view style={{ width: '100%', height: '64px' }} />
  </refresh-header>

  <list id="feedList" />
</refresh>
```

Finish refresh after data reload:

```ts
this.getNodeRef('#feedRefresh').invoke({
  method: 'finishRefresh',
});
```

## Child Structure

Use one `<refresh-header>` plus one scrollable content subtree.

```ttml
<refresh>
  <refresh-header>...</refresh-header>
  <list />
</refresh>
```

Footer loadmore is not part of the public `<refresh>` contract.

## Properties

| Prop | Type | Platforms | What It Does |
| --- | --- | --- | --- |
| `enable-refresh` | `boolean` | Android, iOS, Harmony | Enables or disables pull-down refresh |
| `harmony-pull-down-ratio` | `number` | Harmony | Tunes Harmony pull-down sensitivity |

## Events

| Event | Detail | Notes |
| --- | --- | --- |
| `bindstartrefresh` | `isManual` | Main refresh trigger |
| `bindheaderoffset` | `isDragging`, `offsetPercent` | Preferred header progress event |
| `bindheadershow` | `isDragging`, `offsetPercent` | Legacy header progress alias |
| `bindheaderreleased` | none | Gesture compatibility signal; timing differs by platform |
| `bindrefreshstatechange` | `state` | Refresh state change event |

Refresh state values:

| State | Meaning |
| --- | --- |
| `0` | idle |
| `1` | over-drag release |
| `2` | refreshing |

## UI Methods

| Method | Platforms | Notes |
| --- | --- | --- |
| `finishRefresh` | Android, iOS, Harmony | Ends the refreshing state |
| `autoStartRefresh` | Android, iOS, Harmony | Programmatically enters refresh |

## Common Pattern

```ts
const handleRefresh = async () => {
  try {
    await reloadFeed();
  } finally {
    this.getNodeRef('#feedRefresh').invoke({ method: 'finishRefresh' });
  }
};
```

## Safe Guidance

- use `bindstartrefresh` as the business event
- use `bindheaderoffset` for custom header animation
- call `finishRefresh` when the reload completes
- treat `bindheaderreleased` and `bindheadershow` as compatibility signals
- keep loadmore and footer behavior out of public `<refresh>` guidance
