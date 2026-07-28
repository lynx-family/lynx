// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

final class LynxLogContext {
  static final long UNAVAILABLE_ID = -1L;
  static final LynxLogContext UNAVAILABLE =
      new LynxLogContext(UNAVAILABLE_ID, UNAVAILABLE_ID, UNAVAILABLE_ID);

  final long viewId;
  final long engineId;
  final long runtimeId;

  LynxLogContext(long viewId, long engineId, long runtimeId) {
    this.viewId = viewId;
    this.engineId = engineId;
    this.runtimeId = runtimeId;
  }

  @Override
  public String toString() {
    return "[" + viewId + "," + engineId + "," + runtimeId + "]";
  }
}
