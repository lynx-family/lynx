// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.basedevtool;

import android.util.Log;

public class BaseDevToolLoadSoUtils {
  private static boolean isLoaded = false;
  private static final String TAG = "BaseDevToolLoadSoUtils";

  public static void loadSo() {
    if (isLoaded) {
      return;
    }
    try {
      System.loadLibrary("basedevtool");
      isLoaded = true;
    } catch (UnsatisfiedLinkError e) {
      Log.e(TAG, "load basedevtool so error: " + e.getMessage());
    }
  }
}
