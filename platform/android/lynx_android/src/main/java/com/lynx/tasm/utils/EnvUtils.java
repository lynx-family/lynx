// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import android.content.Context;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import java.io.File;

public class EnvUtils {
  private static final String TAG = "EnvUtils";

  @CalledByNative
  public static String getCacheDir() {
    Context appContext = LynxEnv.inst().getAppContext();
    if (appContext != null) {
      File cacheDir = appContext.getCacheDir();
      return cacheDir.getAbsolutePath();
    }
    return "";
  }
}
