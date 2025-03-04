// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.utils.LynxViewBuilderProperty;
import java.util.Map;

public class LynxViewConfigProcessor {
  private static final String TAG = "LynxViewConfigProcessor";

  public static void parseForLynxViewBuilder(
      @Nullable Map<String, String> lynxViewConfig, @NonNull LynxViewBuilder lynxViewBuilder) {
    if (lynxViewConfig == null || lynxViewConfig.isEmpty()) {
      return;
    }
    // If we don't catch exceptions here, it's easy to crash due to schema misconfiguration
    try {
      String autoConcurrency =
          lynxViewConfig.get(LynxViewBuilderProperty.AUTO_CONCURRENCY.getKey());
      if (autoConcurrency != null) {
        lynxViewBuilder.setEnableAutoConcurrency(Integer.parseInt(autoConcurrency) == 1);
      }
    } catch (NumberFormatException e) {
      LLog.e(TAG, e.toString());
    }
  }
}
