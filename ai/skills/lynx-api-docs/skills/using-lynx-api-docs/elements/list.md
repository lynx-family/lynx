# Lynx list Element

This guide covers the common `<list>` element usage for large or frequently updated scrollable collections in Lynx.

## When to Use `<list>`

### Use `<list>` when:

- you need a high-performance scrollable list for many repeated items
- you need a feed, gallery, or data set that can grow over time
- you can model each row or tile as a `<list-item>`
- you want list-oriented scrolling instead of a generic bounded `<scroll-view>`

### Do not use `<list>` when:

- the child set is small and static; `<scroll-view>` is often simpler
- you do not need list-item structure
- the container only wraps a few blocks and does not benefit from list-oriented rendering

## Quick Start

```tsx
<list className="item-list" scroll-y>
  {items.map((item) => (
    <list-item key={item.id} item-key={item.id}>
      <view className="item">
        <text>{item.title}</text>
      </view>
    </list-item>
  ))}
</list>
```

```css
.item-list {
  height: 100vh;
}

list-item {
  height: 80px;
}

.item {
  padding: 16px;
  border-bottom: 1px solid #eeeeee;
}
```

## Common Props

| Prop | Type | What It Does |
| --- | --- | --- |
| `scroll-y` | `boolean` | Enables vertical scrolling |
| `scroll-x` | `boolean` | Enables horizontal scrolling |
| `upper-threshold` | `number` | Distance from the start edge for upper-threshold events |
| `lower-threshold` | `number` | Distance from the end edge for lower-threshold events |
| `scroll-top` | `number` | Sets vertical scroll position |
| `scroll-left` | `number` | Sets horizontal scroll position |
| `scroll-into-view` | `string` | Scrolls to a child with the matching id |
| `enable-back-to-top` | `boolean` | Enables tap-status-bar to top on supported iOS paths |
| `enable-flex` | `boolean` | Enables Flexbox behavior inside the list |

## Events

| Event | What It Does |
| --- | --- |
| `bindscroll` | Fires while the list scrolls |
| `bindscrolltoupper` | Fires when the list reaches the upper or left threshold |
| `bindscrolltolower` | Fires when the list reaches the lower or right threshold |

## Common Pattern

### Load more near the end

```tsx
<list className="item-list" scroll-y bindscrolltolower={handleLoadMore}>
  {items.map((item) => (
    <list-item key={item.id} item-key={item.id}>
      <view className="item">
        <text>{item.title}</text>
      </view>
    </list-item>
  ))}
</list>
```

## Guardrails

- give `<list>` a resolved height so the scroll area is measurable
- keep `key` and `item-key` stable and unique
- if possible, keep `list-item` height predictable for smoother rendering
- avoid nesting large `<list>` containers inside other large `<list>` containers
- use `<scroll-view>` instead when the child set is small and does not need list-oriented optimization
