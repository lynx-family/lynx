// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.tasm.base.LLog;
import com.lynx.tasm.service.ILynxMonitorService;
import com.lynx.tasm.service.LynxServiceCenter;

/**
 * Util class invoking corresponding LynxService when Lynx info is updated
 */
public class LynxInfoReportHelper {
  private static final String TAG = "LynxInfoReportHelper";

  public static final String KEY_LAST_LYNX_URL = "last_lynx_url";

  // latest loaded async component url
  public static final String KEY_ASYNC_COMPONENT_URL = "last_lynx_async_component_url";

  private volatile ILynxMonitorService mMonitorService = null;

  /**
   * Report generic info via ILynxMonitorService for analyzing crash
   * @param tagName info tag
   * @param tagValue info value
   */
  public void reportLynxCrashContext(String tagName, String tagValue) {
    if (tagValue == null || tagName == null) {
      return;
    }

    // Synchronously report because crash may happen immediately
    try {
      if (mMonitorService == null) {
        synchronized (this) {
          if (mMonitorService == null) {
            mMonitorService = LynxServiceCenter.inst().getService(ILynxMonitorService.class);
          }
        }
      }
      if (mMonitorService == null) {
        LLog.e(TAG, "LynxMonitorService is null");
        return;
      }

      mMonitorService.reportCrashGlobalContextTag(tagName, tagValue);
    } catch (ClassCastException | NullPointerException e) {
      LLog.w(TAG, "Report Lynx Crash Context tag failed for LynxServiceCenter " + e.getMessage());
    }
  }
}
