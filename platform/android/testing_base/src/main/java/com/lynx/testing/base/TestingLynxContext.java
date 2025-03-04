// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.testing.base;

import android.content.Context;
import android.util.DisplayMetrics;
import com.lynx.tasm.behavior.LynxContext;

public class TestingLynxContext extends LynxContext {
  public TestingLynxContext(Context base, DisplayMetrics screenMetrics) {
    super(base, screenMetrics);
  }

  @Override
  public void handleException(Exception e) {}
}
