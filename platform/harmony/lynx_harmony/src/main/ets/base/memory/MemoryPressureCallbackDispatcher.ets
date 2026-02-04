// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export type MemoryPressureCallback = (pressure: number) => void;

export class MemoryPressureCallbackDispatcher {
  private static readonly instance = new MemoryPressureCallbackDispatcher();
  private readonly callbacks: MemoryPressureCallback[] = [];

  static getInstance(): MemoryPressureCallbackDispatcher {
    return MemoryPressureCallbackDispatcher.instance;
  }

  addCallback(callback: MemoryPressureCallback) {
    if (this.callbacks.indexOf(callback) === -1) {
      this.callbacks.push(callback);
    }
  }

  removeCallback(callback: MemoryPressureCallback) {
    const index = this.callbacks.indexOf(callback);
    if (index !== -1) {
      this.callbacks.splice(index, 1);
    }
  }

  notifyMemoryPressure(pressure: number) {
    const callbacks = this.callbacks.slice();
    for (const callback of callbacks) {
      callback(pressure);
    }
  }
}

