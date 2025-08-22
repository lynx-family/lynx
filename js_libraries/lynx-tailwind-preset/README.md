# @lynx-js/tailwind-preset

A Tailwind CSS preset for the Lynx framework that provides utilities and components optimized for cross-platform development.

## Features

### Component Mapping
The preset provides semantic HTML component mappings for Lynx:

```tsx
// Before
<text className="text-gray-300 hover:text-white cursor-pointer transition-colors">
  Home
</text>

// After
<link className="text-gray-300 hover:text-white cursor-pointer transition-colors">
  Home
</link>
```

Supported components:
- `button` - Maps to `view` with button role
- `link`/`a` - Maps to `view` with link role
- `nav` - Maps to `view` with navigation role
- `header` - Maps to `view` with banner role
- `footer` - Maps to `view` with contentinfo role
- `main` - Maps to `view` with main role
- `article` - Maps to `view` with article role
- `section` - Maps to `view` with region role
- `aside` - Maps to `view` with complementary role

### Safe Area Utilities
Handle safe area insets across platforms:

```tsx
<view className="safe-top safe-bottom">
  Content
</view>
```

Available utilities:
- `.safe-top` - Adds top safe area padding
- `.safe-bottom` - Adds bottom safe area padding
- `.safe-left` - Adds left safe area padding
- `.safe-right` - Adds right safe area padding

### Scroll Behavior
Improved scroll behavior with proper indicators:

```tsx
<scroll-view className="overflow-y-auto">
  Content
</scroll-view>
```

Available utilities:
- `.overflow-y-auto` - Vertical scrolling with indicator
- `.overflow-x-auto` - Horizontal scrolling with indicator

### Interactive Elements
Enhanced touch feedback and states:

```tsx
<button className="bg-blue-500 hover:bg-blue-600 active:bg-blue-700">
  Click me
</button>
```

Features:
- Touch feedback transitions
- Hover and active states
- Focus states with rings
- Optimized gradient rendering

## Installation

```bash
npm install @lynx-js/tailwind-preset
# or
yarn add @lynx-js/tailwind-preset
# or
pnpm add @lynx-js/tailwind-preset
```

## Usage

Add the preset to your Tailwind configuration:

```js
// tailwind.config.js
module.exports = {
  presets: [require('@lynx-js/tailwind-preset')],
  // ... rest of your config
}
```

## Contributing

Please read our [Contributing Guidelines](https://github.com/lynx-family/lynx/blob/main/.cursorfile) before submitting a Pull Request.

## License

Apache License 2.0 