// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling;

import android.content.Context;
import android.util.Log;
import com.lynx.explorer.routes.LaunchDescriptor;

final class SparklingRouteLauncherImpl {
  private static final String TAG = "SparklingRouteLauncher";
  private SparklingRouteLauncherImpl() {}
  static boolean open(Context context, LaunchDescriptor descriptor) {
    Log.e(TAG, "Sparkling route requested in withoutSparkling build: " + descriptor.originalUrl);
    return false;
  }
}
