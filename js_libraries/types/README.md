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

