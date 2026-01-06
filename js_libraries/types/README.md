# Introduction

@lynx-js/types is a type package of all public APIs officially provided by the Lynx team. Using this package can help you better use Lynx APIs to develop your applications.

# CSS Type System

This package provides a **gradual type strengthening system** for inline styles in Lynx:

## Type Modes

### CSSProperties (Default - Loose Mode)
Recommended for most use cases. Provides backward compatibility while offering good type safety.

```typescript
import { CSSProperties } from '@lynx-js/types';

const styles: CSSProperties = {
  // ✅ Lynx-specific values work
  position: 'absolute',
  display: 'flex',
  
  // ✅ Standard CSS values also work for backward compatibility
  position: 'static',
  display: 'inline-block',
  
  // ✅ Lynx-specific properties remain strict
  linearGravity: 'center',
};
```

### StrictCSSProperties (Strict Mode)
Opt-in strict type checking for maximum type safety.

```typescript
import { StrictCSSProperties } from '@lynx-js/types';

const styles: StrictCSSProperties = {
  // ✅ Only Lynx-supported enum values allowed
  position: 'absolute',
  display: 'flex',
  
  // ❌ TypeScript error: Value not in Lynx enum
  position: 'static',
};
```

**Key Differences:**
- **Loose Mode** (`CSSProperties`): Standard CSS properties accept both Lynx-specific and broader types - backward compatible
- **Strict Mode** (`StrictCSSProperties`): All properties have strict enum types - maximum type safety
- **Lynx-Specific Properties**: Always strict in both modes (e.g., `linearGravity`, `linearWeight`)

For detailed migration guide, see [MIGRATION_GUIDE.md](./MIGRATION_GUIDE.md).

# Implementation

There are three pieces of content in the entire package, namely:
 
1. background-thread
2. main-thread
3. common

These three sections contain all of Lynx's publicly available features:

1. The **background-thread** contains all the APIs that can be used in the background-thread runtime, including animation functions, the lynx family of APIs, NativeModules, and so on.
2. **main-thread** is the API that can only be called in the main thread, and contains worklet-related functions. Be careful when using this part of the API, as it is called in the main thread, and evaluate the performance impact carefully.
3. **common** are all the APIs common to **background-thread** and **main-thread**, such as setting attributes for element, event listening, and so on.

# Usage

## For Framework Developers

```json
"peerDependencies": {
  "@lynx-js/types": "latest"
}
```

## For Product Developers

```json
"devDependencies": {
  "@lynx-js/types": "latest"
}
```

After installing the dependencies, you can use them directly, for example:

```typescript
import { ListProps } from '@lynx-js/types';
let prop: ListProps;
```

If you need to extend the type, for example, GlobalProps, each business will be extended according to its own type, and can be extended like this:

```typescript
declare module '@lynx-js/types' {
  interface GlobalProps {
     foo: string;
     bar: number;
  }
}
```

Once extended, it can be used anywhere in the package.

