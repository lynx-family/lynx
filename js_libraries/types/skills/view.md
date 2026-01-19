---
name: view
description: Provide information about the `<view>` element. It is a container element that can be used to group other elements. This document details the basic usage and best practices for the `<view>` element.
---

# Lynx UI View SKILL.md

The `<view>` element is a versatile container element similar to HTML's `<div>`. It can hold other elements and serves as the foundation for building layouts. In Lynx, other elements share the same base attributes, events, and methods provided by `<view>`.

## 1. Core Capabilities

- **Layout Container**: Compose layouts with CSS (e.g. flexbox, spacing, sizing).
- **Drawing Container**: Render backgrounds, borders, transforms, and clipping on a container node.
- **Node Targeting**: Identify nodes via `id` and `name` for JS-side or native-side operations.
- **Exposure Instrumentation**: Configure exposure/disexposure detection for analytics and lazy-rendering.
- **Accessibility Metadata**: Provide accessible semantics for assistive technologies.

## 2. Basic Usage

`<view>` element usage example:

```tsx
function MyPage() {
  return (
    <view
      className='card'
      style={{
        width: '100%',
        padding: '12px',
        display: 'flex',
        flexDirection: 'row',
        alignItems: 'center',
        justifyContent: 'space-between',
      }}
    >
      <text>Hello</text>
      <view
        style={{
          width: '40px',
          height: '40px',
          borderRadius: '20px',
          backgroundColor: '#3b82f6',
        }}
      />
    </view>
  )
}
```

```css
.card {
  border-radius: 8px;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}
```

An example using identity and exposure-related attributes:

```tsx
function TrackableBlock() {
  return (
    <view
      id='hero'
      name='heroContainer'
      data-trace-id='home_hero'
      exposure-id='home_hero'
      exposure-scene='home'
      style='width:100%;height:200px;background:#111;'
    >
      <text style='color:white;'>Hero</text>
    </view>
  )
}
```

## 3. Attributes

### Identity & Styling

- `name?: string`: Used to name the element, commonly for native to find the corresponding node via `findViewByName`.
- `id?: string`: Unique identity of the element, used by frontend APIs to find and operate on the node (e.g. `invoke`).
- `style?: string`: Inline styles.
- `class?: string`: CSS class name(s).
- `className?: string`: In ReactLynx, use `className` to set CSS class names, equivalent to `class`.
- `data-*?: any`: Extra data that can be retrieved from event objects.

### Platform Specific

- `flatten?: boolean` (Android only): Force specific nodes to create corresponding Android Views.

### Interaction Control

- `native-interaction-enabled?: boolean`: Whether native-side interaction is enabled on this node. The native gesture interaction (e.g. scroll on `<list>` and `<scroll-view>`) is disabled when set to `false`.
- `user-interaction-enabled?: boolean`: Whether user interaction is enabled on this node. `bindtap`, `bindlongpress`, `bindtouchstart`, `bindtouchmove`, `bindtouchend`, `bindtouchcancel`, etc. are disabled when set to `false`.

### Exposure Detection

- `exposure-id?: string`: Whether this node should listen to exposure/disexposure events.
- `exposure-scene?: string`: Exposure scene used together with `exposure-id` to uniquely identify the monitored node.
- `exposure-ui-margin-top/right/bottom/left?: string`: Boundary scaling value of the target node itself in exposure detection. On Android/iOS, set `enable-exposure-ui-margin` on the node for it to take effect.
- `exposure-screen-margin-top/right/bottom/left?: string`: Screen boundary scaling value referenced by the target node in exposure detection. On Android/iOS, it only takes effect with `enable-exposure-ui-margin`; positive values scale the node boundary, negative values scale the screen boundary.
- `exposure-area?: string`: Viewport intersection ratio that triggers exposure; default is `0%`.
- `enable-exposure-ui-margin?: boolean`: Enables `exposure-ui-margin-*` support.
- `enable-exposure-ui-clip?: boolean`: Whether exposure detection considers the viewport clipping of the parent node.

### Accessibility

- `accessibility-element?: boolean`: Whether the node supports accessibility (image/text nodes default to `true`, other nodes default to `false`).
- `accessibility-label?: string`: Accessible label.

## 4. Common Events & Methods

### Tap

`<view>` commonly uses `bindtap` for click/tap handling:

```tsx
function MyPage() {
  return (
    <view
      bindtap={() => {
        console.log('tap')
      }}
      style='width:100px;height:40px;background:#eee;'
    />
  )
}
```

### Listen to Layout Change Event

When the layout of a node changes, you can listen to `bindlayoutchange`. In performance-sensitive cases, use the main-thread handler form.

```tsx

function MyPage() {
  const handleLayoutChange = (e: LayoutChangeEvent) => {
    'main thread'
    const { width, height, x, y } = e.detail
    console.log('layout', { width, height, x, y })
  }

  return (
    <view
      main-thread:bindlayoutchange={handleLayoutChange}
      style='width:100%;height:max-content;'
    />
  )
}
```

### Find and Call Node Methods

You can find a node by selector and call its methods via `invoke`:

```tsx
function MyPage() {
  return (
    <view
      bindtap={() => {
        lynx
          .createSelectorQuery()
          .select('#target')
          .invoke({
            method: 'someMethod',
            params: { any: 'params' },
          })
          .exec()
      }}
    >
      <view id='target' />
    </view>
  )
}
```

## 5. Get Bounding Client Rect

If you need to measure a node’s on-screen rectangle, use `boundingClientRect` on a selector query.

```tsx
function MyPage() {
  const measure = () => {
    lynx
      .createSelectorQuery()
      .select('#box')
      .boundingClientRect((rect: unknown) => {
        console.log('rect', rect)
      })
      .exec()
  }

  return (
    <view>
      <view id='box' style='width:100px;height:100px;background:#f00;' />
      <view bindtap={measure} style='margin-top:12px;'>
        <text>Measure</text>
      </view>
    </view>
  )
}
```

## 6. Critical Development Advices

- **MUST**: In ReactLynx, use `className` instead of `class`.
- **MUST**: On Android and iOS, set `enable-exposure-ui-margin={true}` if you need `exposure-ui-margin-*` to take effect.
- **MUST**: On Android and iOS, understand that enabling `enable-exposure-ui-margin` changes `exposure-screen-margin-*` behavior and can cause lazy loading in scroll containers to fail.
- **Notice**: By default, exposure detection mainly considers clipping from scrollable parent nodes; set `enable-exposure-ui-clip` to get stricter clipping behavior for other sized parents.
- **Notice**: `flatten` is Android-only; avoid relying on it for cross-platform layout correctness.
