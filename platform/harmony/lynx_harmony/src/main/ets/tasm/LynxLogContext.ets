// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const UNAVAILABLE_ENTITY_ID = -1;

export class LynxLogContext {
  readonly viewId: number;
  readonly engineId: number;
  readonly runtimeId: number;

  constructor(viewId: number, engineId: number, runtimeId: number) {
    this.viewId = viewId;
    this.engineId = engineId;
    this.runtimeId = runtimeId;
  }

  static forView(viewId: number): LynxLogContext {
    return new LynxLogContext(viewId, UNAVAILABLE_ENTITY_ID, UNAVAILABLE_ENTITY_ID);
  }

  toString(): string {
    return `[${this.viewId},${this.engineId},${this.runtimeId}]`;
  }
}
