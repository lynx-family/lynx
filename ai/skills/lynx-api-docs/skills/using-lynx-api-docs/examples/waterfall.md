# Example: Masonry-Style Grid

Use Grid to create a simple two-column layout with variable-height items. This approximates the visual style of masonry, but Grid still preserves row tracks and does not balance column heights.

## Code

```jsx
// Waterfall.jsx
export default function Waterfall() {
  const items = [
    { id: 1, height: 200, color: '#ff351a' },
    { id: 2, height: 300, color: '#00ebeb' },
    { id: 3, height: 150, color: '#ff00ff' },
    { id: 4, height: 250, color: '#ffff00' },
    { id: 5, height: 180, color: '#00ff00' },
    { id: 6, height: 220, color: '#0000ff' },
  ];

  return (
    <scroll-view className="waterfall" scroll-y>
      <view className="grid">
        {items.map((item) => (
          <view
            key={item.id}
            className="grid-item"
            style={{ height: item.height, backgroundColor: item.color }}
          >
            <text className="item-text">Item {item.id}</text>
          </view>
        ))}
      </view>
    </scroll-view>
  );
}
```

```css
.waterfall {
  height: 100vh;
  padding: 16px;
}

.grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.grid-item {
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.item-text {
  color: #ffffff;
  font-size: 18px;
  font-weight: bold;
}
```

## Key Points

1. **Two-column Grid**: Set `grid-template-columns: repeat(2, 1fr)`.
2. **Variable item heights**: Assign a different height to each item.
3. **Automatic placement**: Grid places items into cells in source order; it does not select the shortest column.
4. **Scroll container**: Wrap the grid in a `scroll-view` to enable scrolling.
5. **Visual differentiation**: Use distinct background colors to distinguish the items.

## Considerations

A true masonry layout requires more sophisticated placement logic. In production, you may need to:

- Calculate column heights dynamically.
- Place each new item in the shortest column.
- Account for height changes after images finish loading.
