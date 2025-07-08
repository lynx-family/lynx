Lynx
=======

Lynx is a _family_ of open-source technologies empowering developers to use their existing web skills to create truly
native UIs for both mobile and web from a single codebase, featuring performance at scale and velocity.
The official website is: https://lynxjs.org/


## Overview

This module includes the core components for rendering, event handling, native module communication, and service management.


## Installation

```bash
ohpm install @lynx/lynx
```

## Usage

To get started with Lynx for harmony, you can embed a `LynxView` component in your application's UI layout and provide it with a bundle to render.

```typescript
import { LynxView } from '@lynx/lynx';

@Entry
@Component
struct Index {
  build() {
    Column() {
      LynxView({
        url: 'main.lynx.bundle'
      }).width('100%').height('100%')
    }
  }
}
```

You can add dependency in oh-package.json5 like this:

```json5
{
  "dependencies": {
    "@lynx/lynx": "0.0.1-alpha.1",
  }
}
```





