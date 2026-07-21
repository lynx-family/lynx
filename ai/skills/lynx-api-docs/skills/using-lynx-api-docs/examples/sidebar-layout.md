# Example: Sidebar and Main Content Layout

A classic layout with a sidebar and main content area.

## Code

```jsx
// SidebarLayout.jsx
export default function SidebarLayout() {
  return (
    <view className="layout">
      <view className="sidebar">
        <view className="logo">
          <text>Logo</text>
        </view>
        <view className="menu">
          <view className="menu-item active">
            <text>Dashboard</text>
          </view>
          <view className="menu-item">
            <text>Settings</text>
          </view>
          <view className="menu-item">
            <text>Profile</text>
          </view>
        </view>
      </view>

      <view className="main">
        <view className="header">
          <text className="header-title">Dashboard</text>
        </view>
        <view className="content">
          <text>Main content goes here...</text>
        </view>
      </view>
    </view>
  );
}
```

```css
.layout {
  display: flex;
  flex-direction: row;
  height: 100vh;
}

.sidebar {
  width: 250px;
  background-color: #1a1a2e;
  display: flex;
  flex-direction: column;
}

.logo {
  height: 60px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.logo text {
  color: #ffffff;
  font-size: 20px;
  font-weight: bold;
}

.menu {
  flex: 1;
  padding: 16px 0;
}

.menu-item {
  padding: 12px 24px;
  margin: 4px 16px;
  border-radius: 8px;
}

.menu-item text {
  color: #a0a0a0;
  font-size: 14px;
}

.menu-item.active {
  background-color: rgba(255, 255, 255, 0.1);
}

.menu-item.active text {
  color: #ffffff;
}

.main {
  flex: 1;
  display: flex;
  flex-direction: column;
  background-color: #f5f5f5;
}

.header {
  height: 60px;
  background-color: #ffffff;
  display: flex;
  align-items: center;
  padding: 0 24px;
  border-bottom: 1px solid #e0e0e0;
}

.header-title {
  font-size: 18px;
  font-weight: bold;
  color: #333333;
}

.content {
  flex: 1;
  padding: 24px;
}
```

## Key Points

1. **Fixed-width sidebar**: Set `width: 250px`.
2. **Flexible main content**: Use `flex: 1` to fill the remaining width.
3. **Column-oriented Flex layout**: Use `flex-direction: column` inside both the sidebar and the main content area.
4. **Row-oriented Flex layout**: Use `flex-direction: row` for the overall layout.
5. **Full viewport height**: Set `height: 100vh`.
