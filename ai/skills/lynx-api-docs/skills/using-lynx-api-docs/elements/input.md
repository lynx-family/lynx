# Lynx input Element

This guide covers the current native Android, iOS, and Harmony `<input>` element behavior.

## When to Use `<input>`

### Use `<input>` when:

- you need a native single-line text field
- you need text entry for search, account, phone, email, or password flows
- you need focus or blur control from JS
- you need value or selection to be read or updated programmatically
- you need confirm actions such as `done`, `search`, `send`, or `next`

### Do not use `<input>` when:

- you need multiline editing
- you need line-count behavior or textarea-style scrolling
- you need a cross-platform `beforeinput` event

### Choose `<textarea>` instead when:

- the content can span multiple lines
- return or enter should insert line breaks instead of behaving like a single-line confirm action

> **Cross-platform guardrail:** Set `maxlength` explicitly. Harmony currently defaults native `<input>` to `140` when no `maxlength` is provided.
>
> **iOS `maxlength` guardrail:** When pasted text is longer than the remaining capacity, current native iOS `<input>` clips the pasted segment instead of accepting the full clipboard string.
>
> **Keyboard-avoidance guardrail:** Use a string unit value such as `"12px"` for `avoid-keyboard-spacing` when Android is in scope. Android only parses string units there.

## Quick Start

### Basic text input

```tsx
<input
  placeholder="Enter your username"
  type="text"
  confirm-type="done"
  maxlength={32}
/>
```

### Email input with confirm

```tsx
<input
  placeholder="name@example.com"
  type="email"
  confirm-type="send"
  maxlength={64}
/>
```

## Properties

### Core props

| Prop | Type | What It Does |
| --- | --- | --- |
| `placeholder` | `string` | Sets placeholder text |
| `type` | `"text" \| "number" \| "digit" \| "password" \| "tel" \| "email"` | Chooses the input type and keyboard behavior |
| `confirm-type` | `"done" \| "next" \| "search" \| "send" \| "go"` | Chooses the return or submit action |
| `maxlength` | `number` | Limits input length |
| `input-filter` | `string` | Filters accepted characters using regex or native filtering |
| `disabled` | `boolean` | Fully disables interaction |
| `readonly` | `boolean` | Prevents edits without always behaving exactly like `disabled` |
| `show-soft-input-on-focus` | `boolean` | Controls whether the system keyboard appears on focus |
| `confirm-enter` | `boolean` | Keeps the field focused after confirm instead of auto-blurring |
| `avoid-keyboard` | `boolean` | Moves the Lynx root or view when keyboard overlap would occur |
| `avoid-keyboard-spacing` | `string \| number` | Adds extra keyboard-avoidance spacing; prefer string units for Android-safe behavior |
| `hold-keyboard` | `boolean` | Prevents automatic keyboard dismissal in some focus transitions |

### Text and placeholder styling

| Prop | Platforms | What It Does |
| --- | --- | --- |
| `font-size` | iOS, Android, Harmony | Controls base text size |
| `font-weight` / `font-style` / `font-family` | iOS, Android, Harmony | Controls text font appearance |
| `placeholder-font-size` / `-x-placeholder-font-size` | iOS, Android, Harmony | Controls placeholder font size |
| `placeholder-font-weight` / `-x-placeholder-font-weight` / `placeholder-font-style` / `-x-placeholder-font-style` / `placeholder-font-family` / `-x-placeholder-font-family` | iOS, Android, Harmony | Controls placeholder font appearance |
| `color` | iOS, Android, Harmony | Controls text color |
| `placeholder-color` / `-x-placeholder-color` | iOS, Android, Harmony | Controls placeholder color |
| `caret-color` | iOS, Android, Harmony | Controls cursor color |
| `text-align` | iOS, Android, Harmony | Controls horizontal alignment |

### Partial and platform-specific props

| Prop | Platforms | What It Does |
| --- | --- | --- |
| `line-spacing` | iOS, Android | Adjusts internal line spacing |
| `line-height` | iOS | Adjusts line height; do not assume Android or Harmony parity |
| `letter-spacing` | iOS, Android | Adjusts character spacing |
| `direction` | iOS, Android | Controls writing or text direction |
| `ios-auto-correct` | iOS | Toggles autocorrect |
| `ios-spell-check` | iOS | Toggles spell checking |
| `ios-send-composing-input` | iOS | Controls whether composing input updates are emitted continuously |
| `android-fullscreen-mode` | Android | Controls IME fullscreen extract mode |
| `android-set-soft-input-mode` | Android | Requests the host window soft-input adjustment mode |

## Events

### Event payload fields

| Field | Available On | Description |
| --- | --- | --- |
| `value` | All | Current text value |
| `selectionStart` | All | Current selection start, often `-1` when unfocused |
| `selectionEnd` | All | Current selection end, often `-1` when unfocused |
| `isComposing` | iOS, Android, Harmony | Real composing state on iOS and Android; Harmony currently reports `false`; iOS `input` omits this field when `ios-send-composing-input={false}` |
| `cursor` | iOS | Replacement start index for `beforeinput` |
| `length` | iOS | Replacement length for `beforeinput` |
| `replace` | iOS | Replacement text for `beforeinput` |
| `height` | All | Keyboard height payload |
| `show` | iOS | Keyboard visibility flag |
| `keyboardHeight` | iOS | Keyboard height on show |
| `safeAreaBottom` | iOS | iOS safe-area inset on show |

### Frontend events

| Event | Platforms | When It Fires | Payload Highlights |
| --- | --- | --- | --- |
| `input` | All | Text value changes | `value`, `selectionStart`, `selectionEnd`, and usually `isComposing`; iOS omits `isComposing` when `ios-send-composing-input={false}` |
| `selection` | All | Selection changes | `selectionStart`, `selectionEnd` |
| `confirm` | All | Return or submit action occurs | `value` |
| `focus` | All | Input gains focus | `value` |
| `blur` | All | Input loses focus | `value` |
| `beforeinput` | iOS | Before native text replacement is applied | `value`, `cursor`, `length`, `replace`, `isComposing` |
| `keyboardheightchange` | iOS, Android, Harmony | Keyboard height changes | `height` |
| `keyboard` | iOS | iOS keyboard show or hide lifecycle | `show`, and on show also `keyboardHeight`, `safeAreaBottom` |

### Standard binding syntax

```tsx
<input
  bindinput={handleInput}
  bindconfirm={handleConfirm}
  bindfocus={handleFocus}
  bindblur={handleBlur}
/>
```

## UI Methods

### Standard invocation syntax

```tsx
lynx
  .createSelectorQuery()
  .select('#search-input')
  .invoke({ method: 'focus' })
  .exec();
```

```tsx
this.getNodeRef('#search-input').invoke({
  method: 'setValue',
  params: { value: 'hello' },
});
```

| Method | Parameters | Notes |
| --- | --- | --- |
| `focus` | none | Focuses the input |
| `blur` | none | Blurs the input |
| `getValue` | none | Returns `value`, `selectionStart`, `selectionEnd`, and `isComposing` |
| `setValue` | `value: string`, `cursor?: number` | `cursor` is supported on iOS and Android; do not rely on it for Harmony |
| `setSelectionRange` | `selectionStart: number`, `selectionEnd: number` | Keep the range inside the current value length |

## Common Patterns

### Search box

```tsx
<input
  placeholder="Search keyword"
  type="text"
  confirm-type="search"
  maxlength={50}
/>
```

### Password field

```tsx
<input
  placeholder="Password"
  type="password"
  maxlength={32}
/>
```

### Numeric code field

```tsx
<input
  placeholder="123456"
  type="number"
  maxlength={6}
  input-filter="[0-9]"
  confirm-type="done"
/>
```

## Platform Availability Matrix

| Feature | iOS | Android | Harmony |
| --- | --- | --- | --- |
| Single-line native input | Yes | Yes | Yes |
| `beforeinput` | Yes | No | No |
| `selection` event | Yes | Yes | Yes |
| `keyboard` event | Yes | No | No |
| `keyboardheightchange` event | Yes | Yes | Yes |
| `setValue({ cursor })` | Yes | Yes | No |
| Real composing-state reporting | Yes | Yes | Partial |
| Default unlimited length | Yes | Yes | No |

## Safe Guidance

- prefer `<input>` only for single-line entry
- set `maxlength` explicitly
- on current native iOS `<input>`, over-limit paste is clipped to the remaining capacity
- use `disabled` when you want the field fully inert; `readonly` has different focus behavior across platforms
- if exact numeric acceptance rules matter, pair `type` with explicit filtering
- treat `beforeinput` and `keyboard` as iOS-only frontend events; `keyboardheightchange` is the cross-platform height event
- do not rely on `line-height`, `line-spacing`, `letter-spacing`, or `direction` as if they were uniformly supported on all three native paths
