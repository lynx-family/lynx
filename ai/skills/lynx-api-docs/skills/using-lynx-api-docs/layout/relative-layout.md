# Relative Layout

Relative Layout is a Lynx-specific layout system. It positions child elements relative to the parent container or to sibling elements without introducing extra nesting. Typical use cases include cards with an avatar, text, and a button; chat messages; and complex form rows.

## Core Concepts

Within a `display: relative` container, each child that another element needs to reference is identified by a `relative-id`. A `relative-id` must be a nonzero integer. Sibling elements can use this ID to establish relative positioning constraints.

## Basic Usage

```css
.container {
  display: relative;
  width: 100%;
  min-height: 120px;
}

/* Avatar in the top-left corner */
.avatar {
  relative-id: 1;
  relative-align-left: parent;
  relative-align-top: parent;
  width: 80px;
  height: 80px;
}

/* Username to the right of the avatar, top-aligned with the avatar */
.username {
  relative-id: 2;
  relative-right-of: 1;
  relative-align-top: 1;
  margin-left: 8px;
}

/* Description below the username, left-aligned with the username */
.description {
  relative-id: 3;
  relative-bottom-of: 2;
  relative-align-left: 2;
  margin-top: 4px;
}

/* Close button in the top-right corner */
.close {
  relative-id: 4;
  relative-align-right: parent;
  relative-align-top: parent;
}
```

```jsx
<view className="container">
  <image className="avatar" src="avatar.png" />
  <text className="username">Username</text>
  <text className="description">Description here</text>
  <text className="close">×</text>
</view>
```

Use `margin` to add spacing. For example, `relative-right-of: 1` only states that the current element is to the right of element 1; `margin-left` controls the distance between them.

## Construction Steps

### Step 1: Apply `display: relative`

```css
.container {
  display: relative;
}
```

### Step 2: Assign IDs to Child Elements

```css
.avatar {
  relative-id: 1;
}

.username {
  relative-id: 2;
}
```

### Step 3: Align Edges

Edge-alignment properties align an edge of the current element with the corresponding edge of the parent container or a sibling element.

| Property | Description | Value |
| -------- | ----------- | ----- |
| `relative-align-top` | Aligns the top edges | `parent` or another element ID |
| `relative-align-right` | Aligns the right edges | `parent` or another element ID |
| `relative-align-bottom` | Aligns the bottom edges | `parent` or another element ID |
| `relative-align-left` | Aligns the left edges | `parent` or another element ID |
| `relative-align-inline-start` | Aligns the inline-start edges and adapts to RTL | `parent` or another element ID |
| `relative-align-inline-end` | Aligns the inline-end edges and adapts to RTL | `parent` or another element ID |

### Step 4: Set Relative Positions

Relative-positioning properties place the current element relative to a sibling element. For example, `relative-left-of: 1` places the current element so that its right edge touches the left edge of the sibling whose ID is 1.

| Property | Description | Value |
| -------- | ----------- | ----- |
| `relative-left-of` | Places the element to the left of the target | Another element ID |
| `relative-right-of` | Places the element to the right of the target | Another element ID |
| `relative-top-of` | Places the element above the target | Another element ID |
| `relative-bottom-of` | Places the element below the target | Another element ID |
| `relative-inline-start-of` | Places the element on the inline-start side of the target and adapts to RTL | Another element ID |
| `relative-inline-end-of` | Places the element on the inline-end side of the target and adapts to RTL | Another element ID |

### Step 5: Center an Element

```css
/* Center horizontally */
.center-horizontal {
  relative-id: 1;
  relative-center: horizontal;
}

/* Center vertically */
.center-vertical {
  relative-id: 2;
  relative-center: vertical;
}

/* Center on both axes */
.center-both {
  relative-id: 3;
  relative-center: both;
}
```

## `relative-center` Values

| Value        | Description                  |
| ------------ | ---------------------------- |
| `none`       | No centering (default)       |
| `vertical`   | Centers vertically           |
| `horizontal` | Centers horizontally         |
| `both`       | Centers horizontally and vertically |

## RTL Support

Prefer logical-direction properties when supporting right-to-left (RTL) languages.

| Physical Direction          | Logical Direction (Recommended)    |
| --------------------------- | ---------------------------------- |
| `relative-align-left`       | `relative-align-inline-start`      |
| `relative-align-right`      | `relative-align-inline-end`        |
| `relative-left-of`          | `relative-inline-start-of`         |
| `relative-right-of`         | `relative-inline-end-of`           |

```css
/* Recommended: use logical directions to adapt automatically to RTL */
.card {
  display: relative;
}

.avatar {
  relative-id: 1;
  relative-align-inline-start: parent; /* Left-aligned in LTR; right-aligned in RTL */
  relative-align-top: parent;
}

.username {
  relative-id: 2;
  relative-inline-end-of: 1; /* Right of the avatar in LTR; left of it in RTL */
  relative-align-top: 1;
  margin-inline-start: 8px;
}
```

## `relative-layout-once`

`relative-layout-once` is a layout optimization switch for Relative Layout and is enabled by default. Keep it enabled when sibling-relative positioning is involved.

```css
.container {
  relative-layout-once: true;
}
```

You usually do not need to set this property explicitly. Consider changing it only when diagnosing unusual dependency relationships.

## Best Practices

1. Positioning relative to the parent container does not affect performance and can be used freely.
2. Keep `relative-layout-once` enabled when positioning sibling elements relative to one another.
3. Avoid unnecessary circular dependencies, such as A depending on B horizontally while B depends on A vertically.
4. Do not create unsatisfiable dependency cycles; otherwise, layout will fail.
5. For internationalized interfaces, prefer `relative-align-inline-start`, `relative-align-inline-end`, `relative-inline-start-of`, and `relative-inline-end-of`.

## Practical Example: Chat Message

```css
.chat-message {
  display: relative;
  padding: 12px;
}

.avatar {
  relative-id: 1;
  relative-align-inline-start: parent;
  relative-align-top: parent;
  width: 40px;
  height: 40px;
}

.username {
  relative-id: 2;
  relative-inline-end-of: 1;
  relative-align-top: 1;
  margin-inline-start: 8px;
}

.message-bubble {
  relative-id: 3;
  relative-bottom-of: 2;
  relative-align-inline-start: 2;
  margin-top: 4px;
}
```
