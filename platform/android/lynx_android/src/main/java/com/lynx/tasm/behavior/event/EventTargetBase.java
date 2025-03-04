// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.event;

import com.lynx.react.bridge.ReadableMap;

public interface EventTargetBase {
  public EventTargetBase parentResponder();
  public ReadableMap getDataset();
}
