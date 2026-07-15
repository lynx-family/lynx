# Introduction

@lynx-js/types is a type package of all public APIs officially provided by the Lynx team. Using this package can help you better use Lynx APIs to develop your applications.

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

## Inline style types

Choose strict checking for one object, one style map, or an entire TypeScript project. The default remains backward compatible.

`CSSProperties` and its explicit alias `CompatibleCSSProperties` preserve values from `csstype` and the Lynx inline-style surface from 4.2. TypeScript compatibility does not guarantee that the Lynx runtime accepts every value.

`StrictCSSProperties` is the author-facing alias for the finite, generated `LynxCSSProperties` contract. Current Lynx CSS metadata does not encode every runtime value grammar, so the strict contract can reject values that the runtime accepts.

### Adopt one style object

Use `satisfies StrictCSSProperties` to check one object without changing its inferred type:

```typescript
import type {
  CSSProperties,
  StrictCSSProperties,
} from '@lynx-js/types';

const compatibleStyle: CSSProperties = {
  flex: 1,
  display: 'linear',
};

const checkedStyle = {
  display: 'linear',
  position: 'absolute',
} satisfies StrictCSSProperties;
```

### Adopt a style map

Use `StrictStyleSheet` to check every named style in a module:

```typescript
import type { StrictStyleSheet } from '@lynx-js/types';

export const styles = {
  root: {
    display: 'linear',
    position: 'absolute',
  },
  label: {
    color: '#fff',
    fontSize: '16px',
  },
} satisfies StrictStyleSheet;
```

This contract preserves the concrete `root` and `label` keys and each property's literal inference. It does not add a runtime wrapper.

### Adopt an entire project

Create a declaration file included by your `tsconfig.json`:

```typescript
// lynx-strict-styles.d.ts
import '@lynx-js/types/strict';
```

This type-only entry changes Lynx JSX and element `style` objects to `StrictCSSProperties` across that TypeScript project. Explicit `CSSProperties` annotations stay compatible, and string styles remain accepted without object checking.

Remove the declaration import to return the project to compatible JSX style objects.

## For Framework Developers

```json
"peerDependencies": {
  "@lynx-js/types": "latest"
}
```

Use `InlineStyleProperties` when a framework or component exposes a Lynx style prop. The type resolves to the compatible contract by default and follows a consumer's project-level strict entry:

```typescript
import type { InlineStyleProperties } from '@lynx-js/types';

interface CustomViewProps {
  style?: string | InlineStyleProperties;
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
