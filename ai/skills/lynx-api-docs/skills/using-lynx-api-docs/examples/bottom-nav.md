# Example: Fixed Bottom Navigation

A navigation bar fixed to the bottom of the viewport.

## Code

```jsx
// BottomNav.jsx
export default function BottomNav({ activeTab, onTabChange }) {
  const tabs = [
    { id: 'home', icon: '🏠', label: 'Home' },
    { id: 'discover', icon: '🔍', label: 'Discover' },
    { id: 'profile', icon: '👤', label: 'Profile' },
  ];

  return (
    <view className="bottom-nav">
      {tabs.map((tab) => (
        <view
          key={tab.id}
          className={`nav-item ${activeTab === tab.id ? 'active' : ''}`}
          bindtap={() => onTabChange(tab.id)}
        >
          <text className="nav-icon">{tab.icon}</text>
          <text className="nav-label">{tab.label}</text>
        </view>
      ))}
    </view>
  );
}
```

```css
.bottom-nav {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  display: flex;
  flex-direction: row;
  justify-content: space-around;
  align-items: center;
  height: 60px;
  background-color: #ffffff;
  border-top: 1px solid #e0e0e0;
  padding-bottom: env(safe-area-inset-bottom); /* iPhone safe-area inset */
}

.nav-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  flex: 1;
  height: 100%;
}

.nav-item.active .nav-label {
  color: #ff351a;
}

.nav-icon {
  font-size: 24px;
  margin-bottom: 4px;
}

.nav-label {
  font-size: 12px;
  color: #666666;
}
```

## Key Points

1. **`position: fixed`**: Anchors the navigation bar to the viewport.
2. **`bottom: 0`**: Positions it at the bottom edge.
3. **`env(safe-area-inset-bottom)`**: Accounts for the iPhone bottom safe-area inset.
4. **`flex: 1`**: Gives each navigation item an equal share of the available width.
5. **Safe-area handling**: Preserves bottom spacing on devices with a bottom safe area, such as devices with a home indicator.
