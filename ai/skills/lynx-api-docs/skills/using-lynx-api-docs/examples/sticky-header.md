# Example: Sticky Header

Keep a header pinned to the top while the content scrolls.

## Code

```jsx
// StickyHeader.jsx
export default function StickyHeader() {
  return (
    <view className="page">
      <scroll-view className="content" scroll-y>
        <view className="header">
          <text className="header-title">Sticky Header</text>
        </view>
        <view className="item-list">
          {Array.from({ length: 50 }).map((_, i) => (
            <view className="list-item" key={i}>
              <text>Item {i + 1}</text>
            </view>
          ))}
        </view>
      </scroll-view>
    </view>
  );
}
```

```css
.page {
  display: flex;
  flex-direction: column;
  height: 100vh;
}

.header {
  position: sticky; /* Enable sticky positioning */
  top: 0; /* Pin the header when it reaches the top edge */
  z-index: 100; /* Keep the header above the content */
  padding: 16px;
  background-color: #ffffff;
  border-bottom: 1px solid #e0e0e0;
}

.header-title {
  font-size: 18px;
  font-weight: bold;
  text-align: center;
}

.content {
  flex: 1; /* Fill the remaining space */
  overflow: scroll; /* Enable scrolling */
}

.list-item {
  padding: 16px;
  border-bottom: 1px solid #f0f0f0;
}
```

## Key Points

1. **Header placement**: Keep the sticky header inside the scroll container.
2. **`position: sticky`**: Enables sticky positioning.
3. **`top: 0`**: Pins the header when it reaches the top edge.
4. **`z-index`**: Keeps the header above the other content.
5. **`flex: 1`**: Lets the content area fill the remaining space.
6. **`overflow: scroll`**: Makes the content scrollable.
