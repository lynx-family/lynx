// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.base.memory;

import android.os.Build;
import android.os.Debug;
import android.util.Log;
import com.lynx.base.CalledByNative;
import java.util.Map;

public final class ProcessMemoryInfo {
  private static final String TAG = "ProcessMemoryInfo";

  private ProcessMemoryInfo() {}

  @CalledByNative
  private static String[] getMemoryStats() {
    try {
      Debug.MemoryInfo memoryInfo = new Debug.MemoryInfo();
      Debug.getMemoryInfo(memoryInfo);

      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
        return new String[] {"summary.total-pss", Integer.toString(memoryInfo.getTotalPss())};
      }

      Map<String, String> stats = memoryInfo.getMemoryStats();
      String[] memoryStats = new String[stats.size() * 2];
      int index = 0;
      for (Map.Entry<String, String> entry : stats.entrySet()) {
        memoryStats[index++] = entry.getKey();
        memoryStats[index++] = entry.getValue();
      }
      return memoryStats;
    } catch (Exception e) {
      Log.w(TAG, "Failed to get memory stats.", e);
      return new String[0];
    }
  }
}
