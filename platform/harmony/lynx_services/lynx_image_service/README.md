Lynx Image Service
====

Lynx Image Service for the harmony platform.

## Overview

The Lynx Image Service provides image loading and rendering support for Lynx applications running on HarmonyOS. It implements the `ILynxImageService` interface and initializes a native service backed by `@ohos/imageknifepro`.

## Usage

The service is exposed as a singleton instance, `LynxImageService`. It is used by the Lynx framework on HarmonyOS to enable image functionalities.

## Installation

```bash
ohpm install @lynx/lynx_image_service
```

## How to use

You can add dependency in oh-package.json5 like this:

```json5
{
  "dependencies": {
    "@lynx/lynx_image_service": "0.0.1-alpha.1",
  }
}
```
