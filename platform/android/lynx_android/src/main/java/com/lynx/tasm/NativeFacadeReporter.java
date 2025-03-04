// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;

@SuppressWarnings("JniMissingFunction")
public class NativeFacadeReporter {
  private LynxViewClientV2 mClient;

  public void setTemplateLoadClientV2(LynxViewClientV2 client) {
    mClient = client;
  }

  @CalledByNative
  private void onPerformanceEvent(ReadableMap entryMap) {
    if (mClient == null) {
      return;
    }
    mClient.onPerformanceEvent(PerformanceEntryConverter.makePerformanceEntry(entryMap));
  }
}
