# Theming with CSS Custom Properties

Use CSS custom properties to switch between themes.

## Define CSS Custom Properties

```css
/* Default theme (light) */
:root {
  /* Colors */
  --color-primary: #ff351a;
  --color-secondary: #00ebeb;
  --color-background: #ffffff;
  --color-surface: #f5f5f5;
  --color-text: #333333;
  --color-text-secondary: #666666;
  --color-border: #e0e0e0;

  /* Spacing */
  --spacing-xs: 4px;
  --spacing-sm: 8px;
  --spacing-md: 16px;
  --spacing-lg: 24px;
  --spacing-xl: 32px;

  /* Font sizes */
  --font-size-xs: 10px;
  --font-size-sm: 12px;
  --font-size-md: 14px;
  --font-size-lg: 16px;
  --font-size-xl: 18px;
  --font-size-xxl: 24px;

  /* Border radii */
  --radius-sm: 4px;
  --radius-md: 8px;
  --radius-lg: 12px;
  --radius-round: 50%;

  /* Shadows */
  --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.05);
  --shadow-md: 0 2px 4px rgba(0, 0, 0, 0.1);
  --shadow-lg: 0 4px 8px rgba(0, 0, 0, 0.15);
}
```

## Dark Theme

```css
/* Dark theme */
.theme-wrapper.dark {
  --color-primary: #ff5c45;
  --color-secondary: #33eeee;
  --color-background: #1a1a2e;
  --color-surface: #252540;
  --color-text: #ffffff;
  --color-text-secondary: #a0a0a0;
  --color-border: #33334d;

  --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.3);
  --shadow-md: 0 2px 4px rgba(0, 0, 0, 0.4);
  --shadow-lg: 0 4px 8px rgba(0, 0, 0, 0.5);
}
```

## Use CSS Custom Properties

```css
/* Component styles */
.button {
  padding: var(--spacing-sm) var(--spacing-md);
  background-color: var(--color-primary);
  color: #ffffff;
  border-radius: var(--radius-md);
  font-size: var(--font-size-md);
  box-shadow: var(--shadow-sm);
}

.button-secondary {
  background-color: var(--color-secondary);
}

.card {
  background-color: var(--color-surface);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--spacing-md);
}

.title {
  color: var(--color-text);
  font-size: var(--font-size-xl);
}

.description {
  color: var(--color-text-secondary);
  font-size: var(--font-size-md);
}
```

## Switch Themes in Lynx

```jsx
// ThemeProvider.jsx
import { useState } from '@lynx-js/react';

export function ThemeProvider({ children }) {
  const [theme, setTheme] = useState('light');

  const toggleTheme = () => {
    setTheme((prev) => (prev === 'light' ? 'dark' : 'light'));
  };

  return (
    <view className={`theme-wrapper ${theme}`}>
      {children}
      <view className="theme-toggle" bindtap={toggleTheme}>
        <text>{theme === 'light' ? '🌙' : '☀️'}</text>
      </view>
    </view>
  );
}
```

## Best Practices

1. **Follow a naming convention**: Use the `--category-property` format.
2. **Group tokens by category**: Define colors, spacing, and typography separately.
3. **Use semantic names**: Prefer `--color-primary` to `--color-red`.
4. **Provide fallbacks**: Always specify a fallback with `var(--property, fallback)`.

```css
.button {
  /* Provide fallback values */
  color: var(--button-color, var(--color-primary));
  padding: var(--button-padding, var(--spacing-md));
}
```
