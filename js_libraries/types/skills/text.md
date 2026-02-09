---
name: text
description: Provide information about the `<text>` element. This document focuses on plain text rendering, line truncation, vertical alignment, text selection, and related events and UI methods to complement the API reference.
website: https://lynxjs.org/api/elements/built-in/text
---

# Lynx UI Text SKILL.md

The `<text>` element is the core primitive for rendering text in Lynx. It provides attributes for controlling truncation, vertical alignment, visual effects, and text selection/interaction, while inheriting common layout and styling capabilities from `<view>`.

## 1. Core Capabilities

- **Plain text rendering**: Render single-line or multi-line text with CSS typography (font, color, line-height, etc.).
- **Single-line vertical alignment**: Use `text-single-line-vertical-align` to fine-tune how a single line of plain text sits within its line box when the default baseline alignment is not visually centered.
- **Tail color conversion**: Use `tail-color-convert` to control where the color of the truncation ellipsis (`...`) comes from when `text-maxline` causes truncation.
- **Emoji compatibility (Android)**: Use `android-emoji-compat` to enable Emoji2 adaptation on Android, improving rendering for newer emoji sets when the host app integrates `androidx.emoji2`.
- **Fake bold**: Use `text-fake-bold` to simulate bold text when the underlying font has no bold variant.
- **Text selection & custom behavior**:
  - `text-selection` enables built-in text selection and copy.
  - `custom-context-menu` lets you replace the default selection context menu (copy/select all, etc.).
  - `custom-text-selection` disables the built-in selection gesture logic so you can implement fully custom selection interactions.
- **Layout & selection events / methods**:
  - `bindlayout` reports text layout details such as line count and per-line ranges.
  - `bindselectionchange` reports changes to the selected range.
  - UI methods `setTextSelection`, `getTextBoundingRect`, and `getSelectedText` allow programmatic control and measurement of selection.

## 2. Basic Usage

A simple `<text>` example showing common styling and `text-maxline`:

```tsx
function ArticleSnippet() {
  return (
    <view
      style={{
        padding: '16px',
        borderWidth: '1px',
        borderStyle: 'solid',
        borderColor: '#eee',
        borderRadius: '8px',
      }}
    >
      <text
        style={{
          fontSize: '16px',
          color: '#333',
          lineHeight: '24px',
        }}
        text-maxline="3"
      >
        Lynx is a family of open-source technologies that lets you reuse your web
        skills to build truly native UIs for mobile and web from a single codebase.
      </text>
    </view>
  )
}
```

## 3. Attributes

In addition to the common attributes inherited from `<view>` (such as `id`, `name`, `style`, `className`, accessibility attributes, etc.), `<text>` defines the following text-specific attributes.

### Common Attributes

- `text-maxline?: string`
  - Maximum number of lines to display. When the text exceeds this line count, it is truncated and an ellipsis (`...`) is appended.
  - **Platforms**: Android, iOS, Harmony, PC

### Platform-specific Attributes

- `tail-color-convert?: boolean`
  - By default, when text is truncated, the ellipsis color is taken from the nearest inline text node where truncation happens.
  - When set to `true`, the ellipsis color is taken from the outermost `<text>` element instead, which can make truncation color more consistent across mixed inline styles.
  - **Default**: `false`
  - **Platforms**: Android, iOS
  - **Since**: 2.0

- `text-fake-bold?: boolean`
  - Enables fake bold rendering when the default bold font is not available for the current font family.
  - **Default**: `false`
  - **Platforms**: Android, iOS
  - **Since**: 2.13

- `include-font-padding?: boolean`
  - Adds extra padding above and below the text on Android. This is mainly used in high-language scenarios to avoid glyphs being visually cut off.
  - **Default**: `false`
  - **Platforms**: Android
  - **Since**: 1.0

- `android-emoji-compat?: boolean`
  - Enables Emoji2 adaptation on Android. This improves rendering of newer emoji sets when the host app integrates `androidx.emoji2`.
  - **Default**: `false`
  - **Platforms**: Android
  - **Since**: 2.9

### Selection-related Attributes

- `text-selection?: boolean`
  - Enables built-in text selection and copy behavior on this element.
  - **Default**: `false`
  - **Platforms**: Android, iOS
  - **Since**: 2.18

- `custom-context-menu?: boolean`
  - Enables a custom context menu after selecting and copying text. When this is `true`, the system default selection menu is not shown.
  - Takes effect only when `text-selection={true}`.
  - Typically used together with `bindselectionchange` and `getTextBoundingRect` to position and populate a custom menu.
  - **Default**: `false`
  - **Platforms**: Android, iOS
  - **Since**: 2.18

- `custom-text-selection?: boolean`
  - Enables custom text selection. When this is `true`, the `<text>` element no longer handles gesture logic related to selection and copy.
  - Takes effect only when `text-selection={true}`.
  - Intended for advanced scenarios where you implement your own selection gestures (for example, cross-element selection) and call selection UI methods manually.
  - **Default**: `false`
  - **Platforms**: Android, iOS
  - **Since**: 2.18

### Compatibility & Performance Attributes

- `text-single-line-vertical-align?: 'normal' | 'bottom' | 'center' | 'top'`
  - Controls how a **single line of plain text** is vertically aligned within its line box. Inline child elements are **not** supported.
  - Useful when the default font metrics do not visually center text as expected.
  - **Default**: `'normal'`
  - **Platforms**: iOS, Android, PC
  - **Since**: 2.12
  - **Performance**: Increases text measurement cost. Use only where necessary (for example, key titles or buttons that must look visually centered).


## 4. Events

### `bindlayout`

Triggered after the text has been laid out. Use this event when you need precise layout information such as line count or per-line character ranges.

Key fields on `event.detail` (type `TextLayoutEventDetail`):

- `lineCount: number` – total number of lines.
- `lines: TextLineInfo[]` – information for each line:
  - `start: number` – start index in the full text.
  - `end: number` – end index in the full text.
  - `ellipsisCount: number` – number of ellipsis characters appended on this line due to truncation.
- `size: { width: number; height: number }` – rendered width and height of the text box.

Example:

```tsx
function TextLayoutInfo() {
  const handleLayout = (e: LayoutEvent) => {
    'main thread'
    const { lineCount, lines, size } = e.detail
    console.log('text layout', { lineCount, size, lines })
  }

  return (
    <text
      bindlayout={handleLayout}
      style={{ width: '200px' }}
      text-maxline="2"
    >
      Lynx is a framework for building native UIs for mobile and web.
    </text>
  )
}
```

### `bindselectionchange`

Triggered when the text selection range changes. This is useful when you implement custom context menus or need to react to selection state.

Key fields on `event.detail` (type `TextSelectionChangeEventDetail`):

- `start: number` – start index of the selected text. `-1` when nothing is selected.
- `end: number` – end index of the selected text. `-1` when nothing is selected.
- `direction: 'forward' | 'backward'` – direction in which the selection was made.

Example:

```tsx
function TextSelectionWatcher() {
  const handleSelectionChange = (e: SelectionChangeEvent) => {
    'main thread'
    const { start, end, direction } = e.detail
    if (start !== -1) {
      console.log('selected range', { start, end, direction })
    } else {
      console.log('selection cleared')
    }
  }

  return (
    <text
      id='selectable-text'
      text-selection={true}
      bindselectionchange={handleSelectionChange}
    >
      Long press or drag on this text to change the selection.
    </text>
  )
}
```

## 5. UIMethods

You can call `<text>` UI methods via `lynx.createSelectorQuery().select('#id').invoke(...).exec()`.

### `setTextSelection`

Programmatically sets the selection range based on coordinates relative to the `<text>` element.

Signature (type `SetTextSelectionMethod`):

- `method: 'setTextSelection'`
- `params`:
  - `startX: number`, `startY: number` – coordinates of the selection start, relative to the element.
  - `endX: number`, `endY: number` – coordinates of the selection end, relative to the element.
  - `showStartHandle?: boolean` – whether to show the start handle (default `true`).
  - `showEndHandle?: boolean` – whether to show the end handle (default `true`).
- `success?: (result) => void` – callback invoked on success, where `result` contains:
  - `boundingRect: Rect` – overall bounding rectangle of the selected text.
  - `boxes: Rect[]` – rectangles for each fragment of the selected text.
  - `handles: Handle[]` – handle positions.

Example:

```tsx
function selectFirstWord() {
  lynx
    .createSelectorQuery()
    .select('#selectable-text')
    .invoke({
      method: 'setTextSelection',
      params: {
        startX: 0,
        startY: 5, // roughly on the first line
        endX: 40,  // covers the first word in this example
        endY: 5,
      },
      success: (res) => {
        console.log('selection set', res.boundingRect, res.handles)
      },
    })
    .exec()
}
```

> **Note**: Although `setTextSelection` can work independently, in practice you should enable `text-selection={true}` so the element has selection enabled and the user can also adjust it interactively.

### `getTextBoundingRect`

Gets the bounding rectangles for a specific character range.

Signature (type `GetTextBoundingRectMethod`):

- `method: 'getTextBoundingRect'`
- `params`:
  - `start: number` – start index in the text.
  - `end: number` – end index in the text.
- `success?: (result) => void` – callback invoked on success, where `result` contains:
  - `boundingRect: Rect` – overall rectangle covering the given range.
  - `boxes: Rect[]` – per-line rectangles for the range.

Example:

```tsx
function locateTextRange() {
  lynx
    .createSelectorQuery()
    .select('#selectable-text')
    .invoke({
      method: 'getTextBoundingRect',
      params: {
        start: 5,
        end: 10,
      },
      success: (res) => {
        console.log('range bounding rect', res.boundingRect)
        // You can position a tooltip or menu near res.boundingRect.left/top.
      },
    })
    .exec()
}
```

### `getSelectedText`

Gets the currently selected text content.

Signature (type `GetSelectedTextMethod`):

- `method: 'getSelectedText'`
- `success?: (result) => void`, where:
  - `result.selectedText: string` – the currently selected text, or an empty string when nothing is selected.

Example:

```tsx
function logSelectedText() {
  lynx
    .createSelectorQuery()
    .select('#selectable-text')
    .invoke({
      method: 'getSelectedText',
      success: (res) => {
        if (res.selectedText) {
          console.log('selected text:', res.selectedText)
        }
      },
    })
    .exec()
}
```

## 6. FAQ

**Q: How do I enable text selection?**
A: Set `text-selection={true}` on the `<text>` element. This enables built-in selection and copy. For advanced usage (custom menus or gestures), also consider `custom-context-menu` and `custom-text-selection` together with `bindselectionchange` and the UI methods.

---

**Q: How do I vertically center a single line of text?**
A: Use `text-single-line-vertical-align="center"` on a `<text>` that contains **only one line of plain text** and no inline children. For multi-line text or text with inline children, prefer layout-level solutions (for example, `alignItems: 'center'` on a flex container and matching `line-height`).

---

**Q: When should I use `tail-color-convert`?**
A: Use `tail-color-convert={true}` when you have mixed inline styles (for example, different colors for different spans) but want the truncation ellipsis to always use the outer `<text>` color. If you are fine with the ellipsis inheriting the color of the last inline segment, you can keep the default `false`.

---

**Q: Emoji look broken or incomplete on Android. What should I check?**
A: First, ensure that `android-emoji-compat={true}` is set on the `<text>` element. Then confirm that the host Android app has integrated the `androidx.emoji2` dependency correctly. Without this dependency, enabling the attribute alone cannot fix emoji rendering.

## 7. Critical Development Advices

- **MUST**: Enable `text-selection={true}` before depending on selection behavior (user gestures, `bindselectionchange`, or selection-related UI methods in real features).
- **MUST**: Only enable `text-selection` on elements that really need it. Selection creates additional rendering and state overhead, which can impact performance in large lists.
- **MUST**: When you rely on `custom-text-selection={true}`, implement your own touch/gesture logic and call `setTextSelection` / `getSelectedText` as needed; the built-in selection gestures will no longer work.
- **MUST**: When using `text-single-line-vertical-align`, restrict it to key texts that visually need adjustment (such as primary titles or button labels). Avoid enabling it globally, as it increases text measurement time.
- **Notice**: `include-font-padding` is Android-only and mainly for avoiding glyph clipping in specific languages. Do not enable it by default everywhere, as it changes the effective text height and can break vertical alignment.
- **Notice**: `android-emoji-compat` requires the host Android app to integrate `androidx.emoji2`. Treat this as a host capability flag; enabling it without the dependency has no effect.
- **Notice**: `text-maxlength` is deprecated. Prefer pre-processing text length in your business logic so you can provide better feedback (for example, input limits) and avoid silent truncation.
- **Notice**: `text-vertical-align` and `enable-font-scaling` are deprecated. For new code, use `text-single-line-vertical-align` for fine-grained single-line vertical adjustments and rely on global or app-level font scaling configurations instead of per-element overrides.

## 8. Anti-Patterns (Things to Avoid)

- **DO NOT** expect `bindselectionchange` to fire or `getSelectedText` to return meaningful data when `text-selection={false}` (the default). Explicitly enable selection before relying on these APIs.
- **DO NOT** blindly enable `text-single-line-vertical-align` on every `<text>` in performance-sensitive contexts such as large lists or frequently updated views. Overuse can noticeably increase layout cost.
- **DO NOT** rely on `getTextBoundingRect` results before layout has stabilized (for example, calling it immediately on first render while text or container size is still changing). Prefer calling it after a `bindlayout` event or after the layout is known to be stable.
- **DO NOT** mix deprecated attributes (`text-maxlength`, `text-vertical-align`, `enable-font-scaling`) into new code paths. Keep their usage isolated to legacy components that still require them.
