# Lynx textarea Element

This guide covers the current native Android, iOS, and Harmony `<textarea>` element behavior.

## When to Use `<textarea>`

### Use `<textarea>` when:

- you need a native multiline text editor
- the content can span multiple lines
- you need line counting or `maxlines` control
- you need focus, blur, value, or selection to be controlled programmatically
- you want submit or confirm behavior on a multiline field without falling back to the single-line `<input>` path

### Do not use `<textarea>` when:

- you need a true single-line field
- you need `<input>`-style single-line truncation or single-line native text-field behavior
- you need a cross-platform `beforeinput` event
- you need Harmony-safe password textarea behavior

### Choose `<input>` instead when:

- the value should stay on one line
- pressing return should always behave like a single-line confirm action
- exact single-line keyboard semantics matter more than multiline editing

> **Cross-platform guardrail:** If Harmony is in scope, do not rely on `type="password"` for `<textarea>`.
>
> **iOS `maxlength` guardrail:** When pasted text is longer than the remaining capacity, current native iOS `<textarea>` clips the pasted segment instead of accepting the full clipboard string.
>
> **Line-count guardrail:** Treat `line: -1` as an iOS or Android overflow signal only. Harmony `line` reports the current line count instead.
>
> **Keyboard-avoidance guardrail:** Use a string unit such as `"12px"` for `avoid-keyboard-spacing` when Android is in scope.

## Quick Start

### Basic multiline textarea

```tsx
<textarea
  placeholder="Write your comment"
  maxlength={200}
  maxlines={6}
/>
```

### Textarea with confirm and line tracking

```tsx
<textarea
  id="feedback"
  placeholder="Tell us what happened"
  confirm-type="done"
  maxlines={4}
  bindinput={handleInput}
  bindline={handleLine}
  bindconfirm={handleConfirm}
/>
```

## Properties

### Core props

| Prop | Type | What It Does |
| --- | --- | --- |
| `placeholder` | `string` | Sets placeholder text |
| `type` | `"text" \| "number" \| "digit" \| "tel" \| "email"` | Chooses the textarea input type and keyboard behavior |
| `confirm-type` | `"done" \| "next" \| "search" \| "send" \| "go"` | Chooses the submit or confirm action |
| `maxlength` | `number` | Limits the text length |
| `maxlines` | `number` | Limits the number of visible or accepted lines |
| `input-filter` | `string` | Filters accepted characters using regex or native filtering |
| `disabled` | `boolean` | Fully disables interaction |
| `readonly` | `boolean` | Prevents edits without always behaving exactly like `disabled` |
| `show-soft-input-on-focus` | `boolean` | Controls whether the system keyboard appears on focus |
| `confirm-enter` | `boolean` | Keeps the textarea focused after confirm instead of auto-blurring |
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
| `enable-scroll-bar` | iOS, Android | Shows the internal vertical scroll bar |
| `bounces` | iOS | Enables or disables textarea bounce behavior |
| `line-spacing` | iOS, Android, Harmony | Adjusts line spacing |
| `line-height` | iOS | Adjusts line height; do not assume Android or Harmony parity |
| `letter-spacing` | iOS, Android | Adjusts character spacing |
| `direction` | iOS, Android | Controls writing direction |
| `ios-auto-correct` | iOS | Toggles autocorrect |
| `ios-spell-check` | iOS | Toggles spell checking |
| `ios-send-composing-input` | iOS | Controls whether composing input updates stay in the `input` payload |
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
| `line` | All | Current line count; iOS and Android may emit `-1` when an edit would exceed `maxlines` |
| `height` | All | Keyboard height payload |
| `show` | iOS | Keyboard visibility flag |
| `keyboardHeight` | iOS | Keyboard height on show |
| `safeAreaBottom` | iOS | iOS safe-area inset on show |

### Frontend events

| Event | Platforms | When It Fires | Payload Highlights |
| --- | --- | --- | --- |
| `input` | All | Text value changes | `value`, `selectionStart`, `selectionEnd`, and usually `isComposing` |
| `line` | All | Line count changes; iOS and Android also use it for `maxlines` overflow signaling | `line` |
| `selection` | All | Selection changes | `selectionStart`, `selectionEnd` |
| `confirm` | All | Submit or confirm action occurs instead of an ordinary newline path | `value` |
| `focus` | All | Textarea gains focus | `value` |
| `blur` | All | Textarea loses focus | `value` |
| `beforeinput` | iOS | Before native text replacement is applied | `value`, `cursor`, `length`, `replace`, `isComposing` |
| `keyboardheightchange` | iOS, Android, Harmony | Keyboard height changes | `height` |
| `keyboard` | iOS | iOS keyboard show or hide lifecycle | `show`, and on show also `keyboardHeight`, `safeAreaBottom` |

### Standard binding syntax

```tsx
<textarea
  bindinput={handleInput}
  bindline={handleLine}
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
  .select('#feedback')
  .invoke({ method: 'focus' })
  .exec();
```

```tsx
this.getNodeRef('#feedback').invoke({
  method: 'setValue',
  params: { value: 'hello' },
});
```

| Method | Parameters | Notes |
| --- | --- | --- |
| `focus` | none | Focuses the textarea |
| `blur` | none | Blurs the textarea |
| `getValue` | none | Returns `value`, `selectionStart`, `selectionEnd`, and `isComposing` |
| `setValue` | `value: string`, `cursor?: number` | `cursor` is supported on iOS and Android; do not rely on it for Harmony |
| `setSelectionRange` | `selectionStart: number`, `selectionEnd: number` | Keep the range inside the current value length |

## Common Patterns

### Feedback box with line tracking

```tsx
<textarea
  id="feedback"
  placeholder="What should we improve?"
  maxlength={300}
  maxlines={6}
  bindline={(event) => {
    const { line } = event.detail;
    if (line === -1) {
      console.log('maxlines overflow on iOS/Android');
    }
  }}
/>
```

### Keep focus after confirm

```tsx
<textarea
  placeholder="Add more details"
  confirm-type="done"
  confirm-enter={true}
  maxlines={4}
/>
```

## Platform Availability Matrix

| Feature | iOS | Android | Harmony |
| --- | --- | --- | --- |
| Multiline editing | Yes | Yes | Yes |
| `maxlines` | Yes | Yes | Yes |
| `line` event | Yes | Yes | Yes |
| `line: -1` overflow signal | Yes | Yes | No |
| `beforeinput` | Yes | No | No |
| `keyboardheightchange` | Yes | Yes | Yes |
| Extra `keyboard` event | Yes | No | No |
| `setValue({ cursor })` | Yes | Yes | No |
| `type="password"` | Yes | Yes | No local mapping |
| `enable-scroll-bar` | Yes | Yes | No local handler |
| `bounces` | Yes | No | No local handler |

## Safe Guidance

- prefer `<textarea>` only for multiline entry
- set `maxlength` explicitly
- on current native iOS `<textarea>`, over-limit paste is clipped to the remaining capacity
- treat `line: -1` as an iOS or Android overflow signal only
- do not rely on Harmony-safe `type="password"` behavior for `<textarea>`
