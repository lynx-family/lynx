// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.testbench;

import android.content.Context;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;

public class TestBenchOpenUrlModule extends LynxModule {
  private static final String TAG = "TestBenchOpenUrlModule";

  public TestBenchOpenUrlModule(Context context) {
    super(context);
  }

  @LynxMethod
  public void openSchema(ReadableMap params) {
    LLog.e(TAG, params.toString());
    TestBenchPageManager.getInstance().replayPageFromOpenSchema(params);
  }
}
