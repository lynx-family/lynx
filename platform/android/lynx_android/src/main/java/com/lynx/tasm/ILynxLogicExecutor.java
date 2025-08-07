// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.react.bridge.ReadableMap;

public interface ILynxLogicExecutor {
  // Trigger event callback, event contains two parameters, one is method, which represents the name
  // of the called method, and the other is args, which represents the event parameters.
  void onLynxEvent(LynxView lynxView, ReadableMap event);

  void destroy();
}
