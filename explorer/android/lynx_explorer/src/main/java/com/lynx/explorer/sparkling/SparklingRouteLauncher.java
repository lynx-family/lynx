// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling;

import android.content.Context;
import com.lynx.explorer.routes.LaunchDescriptor;

public final class SparklingRouteLauncher {
  private SparklingRouteLauncher() {}

  public static boolean open(Context context, LaunchDescriptor descriptor) {
    return SparklingRouteLauncherImpl.open(context, descriptor);
  }
}
