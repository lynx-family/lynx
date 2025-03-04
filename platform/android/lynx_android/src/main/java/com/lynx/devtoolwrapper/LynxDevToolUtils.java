// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.HashSet;
import java.util.Set;

public class LynxDevToolUtils {
  private static final ILynxDevToolService DEVTOOL_SERVICE =
      LynxServiceCenter.inst().getService(ILynxDevToolService.class);
  private static final String TAG = "LynxDevToolUtils";

  // set custom native loader for devtool.
  // the loader will be used to load v8 and devtool library.
  // the method must be used before devtool initialization.
  // the method is optional, if user does not set devtool loader,
  // devtool will load v8 and devtool native library by default way.
  static public void setDevToolLibraryLoader(INativeLibraryLoader loader) {
    if (DEVTOOL_SERVICE != null) {
      DEVTOOL_SERVICE.devtoolEnvSetDevToolLibraryLoader(loader);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }

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
