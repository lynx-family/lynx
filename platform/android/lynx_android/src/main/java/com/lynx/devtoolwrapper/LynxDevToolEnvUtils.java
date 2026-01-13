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

  static public void setDevtoolEnv(String key, Object value) {
    if (DEVTOOL_SERVICE != null) {
      DEVTOOL_SERVICE.setDevtoolEnv(key, value);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }

  static public void setDevtoolEnv(String groupKey, Set<String> newGroupValues) {
    if (DEVTOOL_SERVICE != null) {
      DEVTOOL_SERVICE.setDevtoolGroupEnv(groupKey, newGroupValues);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }

  static public Object getDevtoolEnv(String key, Object defaultValue) {
    Object ret = defaultValue;

    if (defaultValue instanceof Boolean) {
      if (DEVTOOL_SERVICE != null) {
        ret = DEVTOOL_SERVICE.getDevtoolBooleanEnv(key, (Boolean) defaultValue);
      } else {
        LLog.e(TAG, "failed to get DevToolService");
      }
    } else if (defaultValue instanceof Integer) {
      if (DEVTOOL_SERVICE != null) {
        ret = DEVTOOL_SERVICE.getDevtoolIntEnv(key, (Integer) defaultValue);
      } else {
        LLog.e(TAG, "failed to get DevToolService");
      }
    } else {
      LLog.e(TAG, "value type error! key: " + key + ", value: " + defaultValue.toString());
    }
    return ret;
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
