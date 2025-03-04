// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface Console {
  debug(...args: any[]): void;
  error(...args: any[]): void;
  group(label?: string): void;
  groupEnd(): void;
  info(...args: any[]): void;
  log(...args: any[]): void;
  alog(...args: any[]): void;
  warn(...args: any[]): void;
}
