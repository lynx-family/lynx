Lynx DevTool Service
====

Lynx DevTool Service for the harmony platform.

## Overview

The `LynxDevToolService` acts as a bridge to connect Lynx applications with the Lynx developer tools. It implements the `ILynxDevToolService` interface and dynamically loads the `@lynx/lynx_devtool` module to provide debugging and inspection capabilities.


## Usage

The service is exposed as a singleton instance, `LynxDevToolService`. It is used by the Lynx framework on HarmonyOS to enable developer features.


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
