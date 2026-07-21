# Example: Card List

A common card-list layout that demonstrates several CSS techniques.

## Result

```
┌─────────────────────────┐
│ [IMG]  Title        >   │
│        Description      │
├─────────────────────────┤
│ [IMG]  Title        >   │
│        Description      │
└─────────────────────────┘
```

## Code

```jsx
// CardList.jsx
import { useState } from '@lynx-js/react';

export default function CardList() {
  const [items] = useState([
    {
      id: 1,
      title: 'Item 1',
      desc: 'Description 1',
      img: 'https://example.com/1.jpg',
    },
    {
      id: 2,
      title: 'Item 2',
      desc: 'Description 2',
      img: 'https://example.com/2.jpg',
    },
  ]);

  return (
    <view className="list">
      {items.map((item) => (
        <view className="card" key={item.id}>
          <image className="card-image" src={item.img} />
          <view className="card-content">
            <text className="card-title">{item.title}</text>
            <text className="card-desc">{item.desc}</text>
          </view>
          <text className="card-arrow">></text>
        </view>
      ))}
    </view>
  );
}
```

```css
/* CardList.css */
.list {
  padding: 16px;
  background-color: #f5f5f5;
}

.card {
  display: flex;
  flex-direction: row;
  align-items: center;
  padding: 12px;
  margin-bottom: 12px;
  background-color: #ffffff;
  border-radius: 8px;
}

.card-image {
  width: 60px;
  height: 60px;
  border-radius: 4px;
  margin-right: 12px;
}

.card-content {
  display: flex;
  flex-direction: column;
  flex: 1; /* Fill the remaining space */
}

.card-title {
  font-size: 16px;
  font-weight: bold;
  color: #333333;
  margin-bottom: 4px;
}

.card-desc {
  font-size: 14px;
  color: #666666;
  white-space: nowrap;
  text-overflow: ellipsis;
  overflow: hidden;
}

.card-arrow {
  font-size: 18px;
  color: #999999;
  margin-left: 8px;
}
```

## Key Points

1. **Flex layout**: Use `display: flex` to arrange items horizontally.
2. **`flex: 1`**: Let the middle content area fill the available width.
3. **Text truncation**: Combine `white-space: nowrap`, `text-overflow: ellipsis`, and `overflow: hidden`.
4. **Spacing**: Use `padding` for space inside an element and `margin` for space outside it.
5. **Rounded corners**: Use `border-radius` to create rounded cards.
