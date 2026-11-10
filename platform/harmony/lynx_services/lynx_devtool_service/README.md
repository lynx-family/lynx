Lynx DevTool Service
====

Lynx DevTool Service for the harmony platform.

## Overview

The `LynxDevToolService` acts as a bridge to connect Lynx applications with the Lynx developer tools. It implements the `ILynxDevToolService` interface and dynamically loads the `@lynx/lynx_devtool` module to provide debugging and inspection capabilities.


## Usage

The service is exposed as `LynxDevToolService.instance`. Registering it loads the
DevTool module asynchronously. Debugging remains disabled until the host explicitly
calls `LynxEnv.initializeDevTool()` after initializing `LynxEnv`.


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

For online development, register the service, initialize Lynx, then initialize DevTool:

```ts
import { LynxEnv, LynxServiceCenter, LynxServiceType } from '@lynx/lynx';
import { LynxDevToolService } from '@lynx/lynx_devtool_service';

LynxServiceCenter.registerService(LynxServiceType.DevTool, LynxDevToolService.instance);
LynxEnv.initialize(context);
await LynxEnv.initializeDevTool();
```

For offline development, call `LynxEnv.prepareDevToolForDevelopmentAfterInit()`
between `LynxEnv.initialize(context)` and `LynxEnv.initializeDevTool()`.
Preparation requests all DebugRouter sessions.

```ts
LynxEnv.initialize(context);
LynxEnv.prepareDevToolForDevelopmentAfterInit();
await LynxEnv.initializeDevTool();
```

`initializeDevTool()` waits for the module to load, then initializes DevTool and
applies any pending session request. Repeated calls are safe. Handle a rejected
Promise if initialization order is incorrect, the service is missing, or module
loading fails. Custom services that load their module asynchronously should
implement `waitForDevToolModuleLoaded()`.

For production, initialize Lynx without either DevTool call.
