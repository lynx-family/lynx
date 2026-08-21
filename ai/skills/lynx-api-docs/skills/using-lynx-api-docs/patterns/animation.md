# Animation

Implement animations and transitions in Lynx.

## Transitions

### Basic Transition

```css
.element {
  transition: all 0.3s ease;
  opacity: 1;
  transform: scale(1);
}

.element:hover {
  opacity: 0.8;
  transform: scale(1.05);
}
```

### Property-Specific Transitions

```css
.button {
  background-color: #ff351a;
  transition: background-color 0.3s ease, transform 0.2s ease;
}

.button:active {
  background-color: #e63016;
  transform: scale(0.98);
}
```

## Keyframe Animations

### Fade In

```css
@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

.fade-in {
  animation: fadeIn 0.3s ease-out;
}
```

### Slide In

```css
@keyframes slideIn {
  from {
    transform: translateX(-100%);
    opacity: 0;
  }
  to {
    transform: translateX(0);
    opacity: 1;
  }
}

.slide-in {
  animation: slideIn 0.3s ease-out;
}
```

### Bounce

```css
@keyframes bounce {
  0%,
  100% {
    transform: translateY(0);
  }
  50% {
    transform: translateY(-10px);
  }
}

.bounce {
  animation: bounce 0.6s ease infinite;
}
```

### Spin

```css
@keyframes spin {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}

.spin {
  animation: spin 1s linear infinite;
}
```

### Pulse

```css
@keyframes pulse {
  0%,
  100% {
    transform: scale(1);
    opacity: 1;
  }
  50% {
    transform: scale(1.05);
    opacity: 0.8;
  }
}

.pulse {
  animation: pulse 1.5s ease-in-out infinite;
}
```

## Examples

### Staggered List-Item Entrance

```jsx
function List({ items }) {
  return (
    <view className="list">
      {items.map((item, index) => (
        <view
          key={item.id}
          className="list-item"
          style={{ animationDelay: `${index * 0.1}s` }}
        >
          <text>{item.name}</text>
        </view>
      ))}
    </view>
  );
}
```

```css
.list-item {
  opacity: 0;
  animation: slideIn 0.3s ease-out forwards;
}

@keyframes slideIn {
  from {
    transform: translateX(-20px);
    opacity: 0;
  }
  to {
    transform: translateX(0);
    opacity: 1;
  }
}
```

### Loading Spinner

```css
.loading-spinner {
  width: 40px;
  height: 40px;
  border: 3px solid #f0f0f0;
  border-top-color: #ff351a;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}
```

### Skeleton Shimmer

```css
.skeleton {
  background: linear-gradient(90deg, #f0f0f0 25%, #e0e0e0 50%, #f0f0f0 75%);
  background-size: 200% 100%;
  animation: shimmer 1.5s infinite;
}

@keyframes shimmer {
  0% {
    background-position: 200% 0;
  }
  100% {
    background-position: -200% 0;
  }
}
```

## Performance Optimization

### Use `transform` and `opacity`

```css
/* ✅ Performance-friendly */
.optimized {
  transition: transform 0.3s ease, opacity 0.3s ease;
}

/* ❌ Avoid animating these properties */
.slow {
  transition: width 0.3s, height 0.3s, margin 0.3s;
}
```

### Hardware Acceleration

```css
.gpu-accelerated {
  transform: translateZ(0);
}

/* Alternative */
.gpu-accelerated-3d {
  transform: translate3d(0, 0, 0);
}
```

## Animation Best Practices

1. **Use `transform` and `opacity`**: These properties generally provide the best animation performance.
2. **Limit concurrent animations**: Avoid running too many animations at once.
3. **Clean up after an animation completes**: Remove animation classes that are no longer needed.
4. **Minimize layout recalculation**: Avoid animating properties such as `width`, `height`, and `margin`.
5. **Use `requestAnimationFrame` appropriately**: Control complex animations with JavaScript.
