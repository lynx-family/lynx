// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;

/**
 * A centralized manager for DevTool user preferences and settings.
 * It is also responsible for taking care of their persistence via SharedPreferences
 * and synchronizing these values with the native layer while necessary.
 * <p>
 * It provides a strongly-typed API (e.g., `isDevToolEnabled()`) to ensure type safety and
 * better maintainability.
 */
public class DevToolSettings {
  private static final String TAG = "DevToolSettings";
  private static volatile DevToolSettings sInstance;
  private SharedPreferences mSharedPreferences;

  // TODO(mitchilling): use the same SP name as LynxEnv to share existing preferences
  private static final String SP_NAME = "lynx_env_config";

  // TODO(mitchilling): change these keys to private when encapsulated
  public static final String SP_KEY_ENABLE_DEVTOOL = "enable_devtool";

  public static DevToolSettings inst() {
    if (sInstance == null) {
      synchronized (DevToolSettings.class) {
        if (sInstance == null) {
          sInstance = new DevToolSettings();
        }
      }
    }
    return sInstance;
  }

  private DevToolSettings() {}

  public void init(Context context) {
    if (context != null) {
      mSharedPreferences = context.getSharedPreferences(SP_NAME, Context.MODE_PRIVATE);
    } else {
      LLog.e(TAG, "init with null context");
    }
  }

  private boolean getPersistedBoolean(String key, boolean defaultValue) {
    if (mSharedPreferences != null) {
      return mSharedPreferences.getBoolean(key, defaultValue);
    }
    return defaultValue;
  }

  private void setPersistedBoolean(String key, boolean value) {
    if (mSharedPreferences != null) {
      mSharedPreferences.edit().putBoolean(key, value).apply();
    }
  }

  private void syncBooleanToNative(@NonNull String key, boolean defaultValue) {
    if (!DevToolLifecycle.getInstance().isInitialized()) {
      return;
    }
    // FIXME(mitchilling): loop dependency between DevToolSettings and LynxEnv
    LynxEnv.inst().nativeSetLocalEnv(key, defaultValue ? "1" : "0");
  }

  private void syncIntToNative(@NonNull String key, int defaultValue) {
    if (!DevToolLifecycle.getInstance().isInitialized()) {
      return;
    }
    // FIXME(mitchilling): loop dependency between DevToolSettings and LynxEnv
    LynxEnv.inst().nativeSetLocalEnv(key, String.valueOf(defaultValue));
  }

  /*
   * This return value indicates the value of the switch ONLY.
   * It doesn't necessarily mean that DevTool is "enabled" without check the lifecycle status.
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public boolean isDevToolEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_DEVTOOL, false);
  }

  public void setDevToolEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_DEVTOOL, enabled);
    syncBooleanToNative(SP_KEY_ENABLE_DEVTOOL, enabled);
  }
}
