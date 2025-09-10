// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export const nativeInitLynxLogWriteFunction: (logWriteFunctionAddress: number) => void;

export const nativeInitLynxLog: (isPrintLogToAllChannel: boolean) => void;

export const nativeUseSysLog: (open: boolean) => void;

export const nativeInternalLog: (level: number, tag: string, message: string) => void;

export const nativeSetMinLogLevel: (level: number) => void;

export const nativeInitLynxBaseTrace: (address: number) => void;
