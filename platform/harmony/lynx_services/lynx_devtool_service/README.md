Lynx DevTool Service
====

Lynx DevTool Service for the harmony platform.

## Overview

The `LynxDevToolService` acts as a bridge to connect Lynx applications with the Lynx developer tools. It implements the `ILynxDevToolService` interface and dynamically loads the `@lynx/lynx_devtool` module to provide debugging and inspection capabilities.


## Usage

The service is exposed as `LynxDevToolService.instance`. Registering it loads the
DevTool module asynchronously. Debugging remains disabled until the host explicitly
prepares development DevTool support after initializing `LynxEnv`.


## Installation

```bash
ohpm install @lynx/lynx_devtool_service
```

## How to use

You can add dependency in oh-package.json5 like this:

```json5
{
  "dependencies": {
    "@lynx/lynx_devtool_service": "0.0.1-alpha.1",
  }
}
```

Register the service, initialize Lynx, then prepare DevTool:

```ts
import { LynxEnv, LynxServiceCenter, LynxServiceType } from '@lynx/lynx';
import { LynxDevToolService } from '@lynx/lynx_devtool_service';

LynxServiceCenter.registerService(LynxServiceType.DevTool, LynxDevToolService.instance);
LynxEnv.initialize(context);
await LynxEnv.prepareDevToolForDevelopmentAfterInit();
```

The preparation method waits for module loading, initializes DevTool, and enables
all DebugRouter sessions. It preserves persisted DevTool switches and is safe to
call repeatedly. Handle a rejected Promise if initialization order is incorrect,
the service is missing, or module loading fails. Custom services that load their
module asynchronously should implement `waitForDevToolModuleLoaded()`.
