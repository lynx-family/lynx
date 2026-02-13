// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import androidx.annotation.RestrictTo;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.HashSet;
import java.util.Set;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class LynxDevToolEnvUtils {
  private static final ILynxDevToolService DEVTOOL_SERVICE =
      LynxServiceCenter.inst().getService(ILynxDevToolService.class);
  private static final String TAG = "LynxDevToolEnvUtils";

  static public void setDevtoolEnv(String groupKey, Set<String> newGroupValues) {
    if (DEVTOOL_SERVICE != null) {
      DEVTOOL_SERVICE.setDevtoolGroupEnv(groupKey, newGroupValues);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }

  static public Set<String> getDevtoolEnv(String groupKey) {
    Set<String> ret = null;
    if (DEVTOOL_SERVICE != null) {
      ret = DEVTOOL_SERVICE.getDevtoolGroupEnv(groupKey);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
    return ret != null ? ret : new HashSet<String>();
  }
}
