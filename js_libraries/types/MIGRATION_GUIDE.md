# Lynx CSS Types Migration Guide

## Overview

This guide helps you understand and adopt the new gradual type strengthening system for inline styles in Lynx. The new system provides a balance between type safety and backward compatibility.

## What Changed?

Previously, PR #1024 introduced strict type definitions that only allowed specific enum values. This caused breaking changes for existing code that relied on broader CSS types from `csstype`.

The new system provides **two modes**:

1. **Loose Mode** (`CSSProperties`) - Default, backward compatible
2. **Strict Mode** (`StrictCSSProperties`) - Opt-in, maximum type safety

## Type System Design

### CSSProperties (Loose Mode) - Default

This is the default export and is **backward compatible** with existing code.

```typescript
import { CSSProperties } from '@lynx-js/types';

const styles: CSSProperties = {
  // ✅ Lynx-specific values work
  position: 'absolute',
  display: 'flex',
  
  // ✅ Standard CSS values work
  position: 'static', // Even though not in Lynx enums
  display: 'inline-block', // Custom values allowed
  
  // ✅ Lynx-specific properties remain strict
  linearGravity: 'center', // Only specific values allowed
};
```

**Key Features:**
- Inherits from `csstype`'s `CSS.Properties`
- Standard CSS properties accept both Lynx-specific and broader CSS types
- Lynx-specific properties (e.g., `linearGravity`, `linearWeight`) remain strict
- Uses `(string & {})` pattern for open-ended types
- Best for gradual migration and existing projects

### StrictCSSProperties (Strict Mode) - Opt-in

Use this when you want maximum type safety and strict enum checking.

```typescript
import { StrictCSSProperties } from '@lynx-js/types';

const styles: StrictCSSProperties = {
  // ✅ Only Lynx-supported enum values allowed
  position: 'absolute',
  display: 'flex',
  
  // ❌ TypeScript error: Type 'static' not in enum
  position: 'static',
  
  // ✅ Lynx-specific properties work the same
  linearGravity: 'center',
};
```

**Key Features:**
- All properties have strict enum types
- Better autocomplete and IntelliSense
- Catches invalid values at compile time
- May require code changes for existing projects
- Best for new projects or when migrating to full type safety

## Property Categories

### Standard CSS Properties (Loose in Default Mode)

Properties like `position`, `display`, `flexDirection`, etc. accept broader types in loose mode:

```typescript
// ✅ In CSSProperties (loose mode)
const loose: CSSProperties = {
  position: 'absolute' | 'relative' | 'fixed' | 'sticky' | (string & {})
};

// ✅ In StrictCSSProperties (strict mode)
const strict: StrictCSSProperties = {
  position: 'absolute' | 'relative' | 'fixed' | 'sticky'
};
```

### Lynx-Specific Properties (Always Strict)

Lynx-specific properties are strict in both modes to ensure correct usage:

```typescript
// Both modes require exact enum values
const styles: CSSProperties = {
  linearGravity: 'center', // ✅
  linearGravity: 'invalid', // ❌ TypeScript error in both modes
  
  linearWeight: 1, // ✅
  linearOrientation: 'horizontal', // ✅
};
```

**Lynx-specific properties include:**
- Properties starting with `linear*` (e.g., `linearGravity`, `linearDirection`)
- Properties starting with `relative*` (e.g., `relativeId`, `relativeCenter`)
- Properties with `-x-` prefix or starting with `X` (e.g., `XHandleSize`)
- Layout animation properties (e.g., `layoutAnimationCreateDuration`)
- Transition names (e.g., `enterTransitionName`, `exitTransitionName`)

## Migration Strategies

### Strategy 1: No Changes Required (Recommended for Most Projects)

If your code already works, **you don't need to make any changes**. The default `CSSProperties` type is backward compatible.

```typescript
// This continues to work without changes
import { CSSProperties } from '@lynx-js/types';

const MyComponent = () => {
  const style: CSSProperties = {
    position: 'sticky',
    display: 'flex',
    color: 'red',
  };
  
  return <div style={style}>Content</div>;
};
```

### Strategy 2: Gradual Migration to Strict Mode

For projects that want to adopt stricter types over time:

**Step 1:** Start with loose mode (no changes)
```typescript
import { CSSProperties } from '@lynx-js/types';

const styles: CSSProperties = {
  // Current code works as-is
};
```

**Step 2:** Identify critical components that would benefit from strict checking
```typescript
import { StrictCSSProperties } from '@lynx-js/types';

// Use strict types for new components or critical UI
const NavbarStyles: StrictCSSProperties = {
  position: 'fixed',
  display: 'flex',
  // TypeScript will catch any invalid values
};
```

**Step 3:** Gradually convert more components
```typescript
// You can mix both types in the same codebase
import { CSSProperties, StrictCSSProperties } from '@lynx-js/types';

// Legacy code uses loose mode
const legacyStyles: CSSProperties = { /* ... */ };

// New code uses strict mode
const newStyles: StrictCSSProperties = { /* ... */ };
```

### Strategy 3: Full Strict Mode Adoption

For new projects or teams ready to embrace full type safety:

```typescript
import { StrictCSSProperties } from '@lynx-js/types';

// Use StrictCSSProperties everywhere
const styles: StrictCSSProperties = {
  position: 'absolute',
  display: 'flex',
  linearGravity: 'center',
};
```

**Benefits:**
- Maximum type safety
- Better IntelliSense and autocomplete
- Catch errors at compile time
- Consistent codebase

**Considerations:**
- May require updating existing code
- Need to ensure all values are Lynx-supported
- Team must be aligned on strict typing

## Common Patterns

### Pattern 1: Dynamic Styles with Loose Mode

```typescript
import { CSSProperties } from '@lynx-js/types';

function getStyles(theme: Theme): CSSProperties {
  return {
    // Mix of Lynx-specific and custom values
    display: 'flex',
    position: theme.isFixed ? 'fixed' : 'relative',
    backgroundColor: theme.primaryColor, // Any color value works
  };
}
```

### Pattern 2: Type-Safe Styling with Strict Mode

```typescript
import { StrictCSSProperties } from '@lynx-js/types';

type DisplayValue = 'none' | 'flex' | 'grid' | 'linear';

function createLayout(display: DisplayValue): StrictCSSProperties {
  return {
    display, // Type-checked at compile time
    position: 'relative',
    linearGravity: 'center',
  };
}
```

### Pattern 3: Combining csstype and Lynx Types

```typescript
import { CSSProperties } from '@lynx-js/types';
import * as CSS from 'csstype';

// CSSProperties already extends CSS.Properties
const styles: CSSProperties = {
  // Standard CSS properties work
  color: 'red',
  fontSize: '16px',
  
  // Lynx-specific properties work
  linearGravity: 'center',
  display: 'linear',
};
```

### Pattern 4: Utility Types

```typescript
import { 
  CSSProperties, 
  CSSPropertiesWithShorthands,
  CSSPropertiesWithLonghands 
} from '@lynx-js/types';

// Only shorthand properties (e.g., border, padding, margin)
const shorthands: CSSPropertiesWithShorthands = {
  border: '1px solid red',
  padding: '10px',
};

// Only longhand properties (e.g., borderLeft, paddingTop)
const longhands: CSSPropertiesWithLonghands = {
  borderLeft: '1px solid red',
  paddingTop: '10px',
};
```

## Best Practices

### 1. Start with Loose Mode

For existing projects, stick with the default `CSSProperties` to avoid breaking changes.

```typescript
import { CSSProperties } from '@lynx-js/types';
```

### 2. Use Strict Mode for New Components

When building new features, consider using `StrictCSSProperties` for better type safety.

```typescript
import { StrictCSSProperties } from '@lynx-js/types';
```

### 3. Be Consistent Within a Module

Don't mix loose and strict modes within the same component or module unless there's a good reason.

```typescript
// Good: Consistent within module
import { CSSProperties } from '@lynx-js/types';

const style1: CSSProperties = { /* ... */ };
const style2: CSSProperties = { /* ... */ };

// Avoid: Mixing modes unnecessarily
const style3: CSSProperties = { /* ... */ };
const style4: StrictCSSProperties = { /* ... */ };
```

### 4. Leverage TypeScript IntelliSense

Both modes provide excellent autocomplete support. Use it to discover available property values:

```typescript
const styles: StrictCSSProperties = {
  position: // Ctrl+Space shows: 'absolute' | 'relative' | 'fixed' | 'sticky'
};
```

### 5. Document Your Choice

If you choose strict mode for a project, document this decision so your team understands why certain values might not work.

```typescript
/**
 * Navbar styles using StrictCSSProperties for maximum type safety.
 * Only Lynx-supported CSS values are allowed.
 */
const navbarStyles: StrictCSSProperties = { /* ... */ };
```

## Troubleshooting

### Issue: Property value not allowed in Strict Mode

**Problem:**
```typescript
const styles: StrictCSSProperties = {
  position: 'static', // Error: Type 'static' is not assignable
};
```

**Solution:**
Either use a Lynx-supported value or switch to loose mode:
```typescript
// Option 1: Use Lynx-supported value
const styles: StrictCSSProperties = {
  position: 'relative', // ✅
};

// Option 2: Use loose mode
const styles: CSSProperties = {
  position: 'static', // ✅
};
```

### Issue: Lynx-specific property value invalid

**Problem:**
```typescript
const styles: CSSProperties = {
  linearGravity: 'invalid', // Error even in loose mode
};
```

**Solution:**
Lynx-specific properties are strict in both modes. Use a valid enum value:
```typescript
const styles: CSSProperties = {
  linearGravity: 'center', // ✅ Valid values: 'none' | 'top' | 'bottom' | 'left' | 'right' | 'center' | ...
};
```

### Issue: Can't use custom CSS values

**Problem:**
```typescript
const styles: StrictCSSProperties = {
  display: 'table', // Error: Not in Lynx enum
};
```

**Solution:**
Use loose mode if you need custom CSS values:
```typescript
const styles: CSSProperties = {
  display: 'table', // ✅ Works in loose mode
};
```

## TypeScript Configuration

For best results, ensure your `tsconfig.json` has:

```json
{
  "compilerOptions": {
    "strict": true,
    "strictNullChecks": true,
    "noImplicitAny": true
  }
}
```

## Summary

| Feature | Loose Mode (`CSSProperties`) | Strict Mode (`StrictCSSProperties`) |
|---------|------------------------------|-------------------------------------|
| **Default** | ✅ Yes | ❌ No (opt-in) |
| **Backward Compatible** | ✅ Yes | ⚠️ May break existing code |
| **Standard CSS Props** | Both Lynx + broader types | Only Lynx enum values |
| **Lynx-Specific Props** | Strict enum only | Strict enum only |
| **Autocomplete** | ✅ Good | ✅ Excellent |
| **Type Safety** | ⚠️ Moderate | ✅ Maximum |
| **Best For** | Existing projects, gradual migration | New projects, maximum safety |

## Next Steps

1. **No Action Required:** If your code works, continue using `CSSProperties`
2. **Explore Strict Mode:** Try `StrictCSSProperties` on new components
3. **Provide Feedback:** Report any issues or suggestions
4. **Gradual Migration:** Plan a migration strategy if desired

## Support

For questions or issues:
- Open an issue in the [Lynx repository](https://github.com/lynx-family/lynx/issues)
- Check the [CSS Types documentation](https://lynxjs.org/docs/types)
- Review the [changelog](./CHANGELOG.md) for updates
