# Pseudo-classes and pseudo-elements

Pseudo-classes and pseudo-elements supported by Lynx.

## Pseudo-classes

### State pseudo-classes

#### :active

Matches an element while it is being activated.

```css
.button:active {
  opacity: 0.8;
  transform: scale(0.98);
}
```

#### :focus

Matches an element that has focus.

```css
input:focus {
  border-color: #ff351a;
}
```

#### :hover

Matches an element while the pointer is hovering over it. Support varies by platform.

```css
.button:hover {
  background-color: #e63016;
}
```

> **Note:** Although the CSS parser recognizes `:disabled` and `:enabled`, the Lynx DOM layer does not implement matching for these pseudo-classes. They therefore have no effect in practice. Apply disabled-state styles by adding and removing a class instead.
>
> ```css
> /* Recommended */
> .button.disabled {
>   opacity: 0.5;
>   background-color: #ccc;
> }
> ```

### Negation pseudo-class

#### :not()

Excludes elements that match the specified selector.

```css
/* Exclude elements with a specific class */
.item:not(.exclude) {
  color: black;
}
```

## Pseudo-elements

> **Note:** `::before` and `::after` are **not supported**. Although the CSS parser recognizes these pseudo-elements, neither the selector matcher nor the rendering engine implements them, so using them has no effect.

### ::placeholder

Styles placeholder text in an input element.

```css
input::placeholder {
  color: #999;
  font-size: 14px;
}
```

### ::selection

Styles selected text.

```css
::selection {
  background-color: #1890ff;
  color: #fff;
}
```

## Examples

### Button states

```css
.button {
  background-color: #ff351a;
  color: #fff;
  padding: 12px 24px;
}

/* Pressed state */
.button:active {
  background-color: #e63016;
  transform: scale(0.98);
}

/* Disabled state implemented with a class */
.button.disabled {
  background-color: #ccc;
  opacity: 0.6;
}
```

### Form input

```css
.input {
  border: 1px solid #ddd;
  padding: 8px 12px;
}

/* Focused state */
.input:focus {
  border-color: #ff351a;
  outline: none;
}

/* Placeholder styles */
.input::placeholder {
  color: #999;
}
```

## Unsupported pseudo-classes and pseudo-elements

- ❌ `:first-child` / `:last-child` / `:nth-child()` - Structural pseudo-classes
- ❌ `:nth-of-type()` / `:only-child` / `:empty` - Structural pseudo-classes
- ❌ `:is()` - Newer selector syntax
- ❌ `:where()` - Newer selector syntax
- ❌ `:has()` - Relational pseudo-class
- ❌ `:target` - Target pseudo-class
- ❌ `:disabled` / `:enabled` - Form state pseudo-classes (parsed but not matched)
- ❌ `:checked` - Checked-state pseudo-class (parsed but not matched)
- ❌ `:valid`, `:invalid` - Form validation pseudo-classes
- ❌ `:required`, `:optional`
- ❌ `::before` / `::after` - Pseudo-elements
- ❌ `::first-line` - First-line pseudo-element
- ❌ `::first-letter` - First-letter pseudo-element

**Note:** All pseudo-classes and pseudo-elements marked with ❌ above are unsupported.
