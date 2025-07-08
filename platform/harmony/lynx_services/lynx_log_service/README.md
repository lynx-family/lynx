Lynx Log Service
====

Lynx Log Service for the harmony platform.

## Overview

The Lynx Log Service provides a logging mechanism for Lynx applications running on HarmonyOS. It implements the `ILynxLogService` interface and utilizes the native `hilog` module for log output, supporting standard levels like `VERBOSE`, `DEBUG`, `INFO`, `WARN`, and `ERROR`.

## Usage

The service is exposed as a singleton instance, `LynxLogService`. It is used by the Lynx framework on HarmonyOS to enable logging functionalities.

## Installation

```bash
ohpm install @lynx/lynx_log_service
```

## How to use

You can add dependency in oh-package.json5 like this:

```json5
{
  "dependencies": {
    "@lynx/lynx_log_service": "0.0.1-alpha.1",
  }
}
```
