// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
export class CallbackManager {
  private id: number = 1;
  private callbacks: Map<number, Function>;
  private taskIdToCallbackIds: Map<number, number>;
  private recycledIds: number[];

  private static readonly MIN_ID = 1;
  private static readonly MAX_ID = Number.MAX_SAFE_INTEGER;

  constructor() {
    this.callbacks = new Map();
    this.taskIdToCallbackIds = new Map();
    this.recycledIds = [];
  }

  private nextId(): number | undefined {
    if (!this.callbacks) {
      return undefined;
    }
    if (this.recycledIds && this.recycledIds.length > 0) {
      while (this.recycledIds.length > 0) {
        const reused = this.recycledIds.pop();
        if (!this.callbacks.has(reused)) {
          return reused;
        }
      }
    }
    const start =
      this.id >= CallbackManager.MIN_ID ? this.id : CallbackManager.MIN_ID;
    let candidate = start;
    while (this.callbacks.has(candidate)) {
      candidate =
        candidate >= CallbackManager.MAX_ID
          ? CallbackManager.MIN_ID
          : candidate + 1;
      if (candidate === start) {
        return undefined;
      }
    }
    this.id =
      candidate >= CallbackManager.MAX_ID
        ? CallbackManager.MIN_ID
        : candidate + 1;
    return candidate;
  }

  addCallback(callback: Function): number | undefined {
    if (!this.callbacks) {
      return undefined;
    }
    const id = this.nextId();
    if (id === undefined) {
      return undefined;
    }
    this.callbacks.set(id, callback);
    return id;
  }

  invokeCallback(once: boolean, key: number, ...args: unknown[]) {
    if (!this.callbacks) {
      return;
    }
    const callback = this.callbacks.get(key);
    if (callback) {
      try {
        callback.apply(callback, args);
      } finally {
        if (once) {
          this.removeCallback(key);
        }
      }
    } else {
      console.warn(`callCallback: Callback with ID ${key} not found`);
    }
  }

  removeCallback(key: number) {
    if (this.callbacks) {
      if (typeof key !== 'number') {
        return;
      }
      const removed = this.callbacks.delete(key);
      if (removed && this.recycledIds) {
        this.recycledIds.push(key);
      }
    }
  }

  addTaskIdAndCallbackId(taskId: number, callbackId: number) {
    if (this.taskIdToCallbackIds) {
      this.taskIdToCallbackIds.set(taskId, callbackId);
    }
  }

  removeCallbackByTaskId(taskId: number) {
    if (this.taskIdToCallbackIds && this.callbacks) {
      const callbackId = this.taskIdToCallbackIds.get(taskId);
      this.taskIdToCallbackIds.delete(taskId);
      this.removeCallback(callbackId);
    }
  }
  removeTaskId(taskId: number | undefined) {
    if (this.taskIdToCallbackIds && taskId !== undefined) {
      this.taskIdToCallbackIds.delete(taskId);
    }
  }

  destroy() {
    this.callbacks = undefined;
    this.taskIdToCallbackIds = undefined;
    this.recycledIds = undefined;
  }
}
