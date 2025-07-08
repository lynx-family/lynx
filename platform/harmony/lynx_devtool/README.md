LynxDevtool
========

When encountering issues during Lynx page development, you can use [DevTool](https://lynxjs.org/guide/debugging/lynx-devtool.html) for debugging, and you need to integrate DevTool.


## Overview

The Lynx DevTool module provides a set of tools for debugging and inspecting Lynx applications on the harmony platform. It facilitates communication with remote debugging tools, manages inspector sessions, and provides an in-app LogBox for displaying errors and logs.


## Installation

```bash
ohpm install @lynx/lynx_devtool
```

## How to use

You can add dependency in oh-package.json5 like this:

```json5
{
  "dependencies": {
    "@lynx/lynx_devtool": "0.0.1-alpha.1",
  }
}
```
