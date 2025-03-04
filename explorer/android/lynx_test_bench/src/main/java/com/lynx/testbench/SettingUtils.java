// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.testbench;

import android.content.Context;
import android.content.SharedPreferences;
import com.lynx.tasm.base.LLog;

public class SettingUtils {
  private static final String TAG = "SettingUtils";
  private static final String SP_SETTINGS_KEY = "lynx_example_sp";
  private static final String ENABLE_LYNX_SERVICE_KEY = "lynx_service_enabled";

  private SharedPreferences mSP = null;
  private boolean lynxServiceEnabled = false;
  private static volatile SettingUtils sInstance;

  public static SettingUtils inst() {
    if (sInstance == null) {
      synchronized (SettingUtils.class) {
        if (sInstance == null) {
          sInstance = new SettingUtils();
        }
      }
    }
    return sInstance;
  }

  private SharedPreferences initIfNot(Context context) {
    synchronized (this) {
      return mSP != null ? mSP
                         : context.getSharedPreferences(SP_SETTINGS_KEY, Context.MODE_PRIVATE);
    }
  }

  public void initSettings(Context context) {
    synchronized (this) {
      if (context != null) {
        mSP = initIfNot(context);
        lynxServiceEnabled = mSP.getBoolean(ENABLE_LYNX_SERVICE_KEY, false);
      }
    }
  }

  public boolean isLynxServiceEnabled() {
    synchronized (this) {
      if (mSP == null) {
        LLog.e(TAG, "please call initSettings first");
      }
      return lynxServiceEnabled;
    }
  }

  public void setLynxServiceEnabled(boolean enabled) {
    synchronized (this) {
      if (mSP != null) {
        mSP.edit().putBoolean(ENABLE_LYNX_SERVICE_KEY, enabled).commit();
      }
    }
  }
}
