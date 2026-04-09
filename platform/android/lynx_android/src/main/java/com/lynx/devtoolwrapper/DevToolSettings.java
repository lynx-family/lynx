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
 * <p>
 * <b>Architectural Note on Lifecycle Checks:</b>
 * This class acts as a pure "Data Layer" representing the user's raw intent or saved preference.
 * It intentionally DOES NOT check `DevToolLifecycle.isEnabled()` or other system states when
 * returning values. This prevents "loss of information" where a user's preference is masked
 * by a transient system state.
 * To get the "Effective State" (User Preference + System Capability), callers should query
 * the facade layer (e.g., `LynxEnv`), which combines this data with the lifecycle state.
 */
public class DevToolSettings {
  private static final String TAG = "DevToolSettings";
  private static volatile DevToolSettings sInstance;
  private SharedPreferences mSharedPreferences;

  // TODO(mitchilling): use the same SP name as LynxEnv to share existing preferences
  private static final String SP_NAME = "lynx_env_config";

  // TODO(mitchilling): change these keys to private when encapsulated
  public static final String SP_KEY_ENABLE_DEVTOOL = "enable_devtool";
  public static final String SP_KEY_ENABLE_LOGBOX = "enable_logbox";
  public static final String SP_KEY_ENABLE_DEBUG_MODE = "enable_debug_mode";
  public static final String SP_KEY_ENABLE_LAUNCH_RECORD = "enable_launch_record";
  public static final String SP_KEY_ENABLE_QUICKJS_DEBUG = "enable_quickjs_debug";
  public static final String SP_KEY_ENABLE_QUICKJS_CACHE = "enable_quickjs_cache";
  public static final String SP_KEY_ENABLE_V8 = "enable_v8";
  public static final String SP_KEY_ENABLE_DOM_TREE = "enable_dom_tree";
  public static final String SP_KEY_ENABLE_LONG_PRESS_MENU = "enable_long_press_menu";
  public static final String SP_KEY_ENABLE_HIGHLIGHT_TOUCH = "enable_highlight_touch";
  public static final String SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT = "enable_preview_screen_shot";
  public static final String SP_KEY_ENABLE_PIXEL_COPY = "enable_pixel_copy";
  public static final String SP_KEY_ENABLE_FSP_SCREENSHOT = "enable_fsp_screenshot";
  public static final String SP_KEY_ENABLE_PERF_METRICS = "enable_perf_metrics";

  public static final int V8_OFF = 0;
  public static final int V8_ON = 1;
  public static final int V8_ALIGN_WITH_PROD = 2;

  // Member variables to store non-persisted settings
  private volatile boolean mHighlightTouchEnabled = false;
  private volatile boolean mPreviewScreenshotEnabled = true;
  private volatile boolean mPerfMetricsEnabled = false;

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
    // In order to receive calls to pre-set values before DevTool initializes, e.g.
    // SP_KEY_ENABLE_DEBUG_MODE is usually set very early so that DevTool can read its value from
    // SharedPreference and initialize in expected mode, we have divided init() into two methods.
    if (context == null) {
      LLog.e(TAG, "init with null context");
      return;
    }
    mSharedPreferences = context.getSharedPreferences(SP_NAME, Context.MODE_PRIVATE);
  }

  public void syncToNative() {
    if (!DevToolLifecycle.getInstance().isInitialized()) {
      LLog.e(TAG, "DevTool is not initialized yet, can't sync references to native");
      return;
    }

    syncToNativeBoolean(SP_KEY_ENABLE_DEVTOOL, getPersistedBoolean(SP_KEY_ENABLE_DEVTOOL, false));
    syncToNativeBoolean(
        SP_KEY_ENABLE_QUICKJS_CACHE, getPersistedBoolean(SP_KEY_ENABLE_QUICKJS_CACHE, true));
    syncToNativeInt(SP_KEY_ENABLE_V8, getPersistedInt(SP_KEY_ENABLE_V8, V8_ALIGN_WITH_PROD));
    syncToNativeBoolean(
        SP_KEY_ENABLE_QUICKJS_DEBUG, getPersistedBoolean(SP_KEY_ENABLE_QUICKJS_DEBUG, true));
    syncToNativeBoolean(SP_KEY_ENABLE_DOM_TREE, getPersistedBoolean(SP_KEY_ENABLE_DOM_TREE, true));
    syncToNativeBoolean(SP_KEY_ENABLE_LOGBOX, getPersistedBoolean(SP_KEY_ENABLE_LOGBOX, true));
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

  private int getPersistedInt(String key, int defaultValue) {
    if (mSharedPreferences != null) {
      return mSharedPreferences.getInt(key, defaultValue);
    }
    return defaultValue;
  }

  private void setPersistedInt(String key, int value) {
    if (mSharedPreferences != null) {
      mSharedPreferences.edit().putInt(key, value).apply();
    }
  }

  private void syncToNativeBoolean(@NonNull String key, boolean value) {
    if (!DevToolLifecycle.getInstance().isInitialized()) {
      return;
    }
    // FIXME(mitchilling): loop dependency between DevToolSettings and LynxEnv
    LynxEnv.inst().nativeSetLocalEnv(key, value ? "1" : "0");
  }

  private void syncToNativeInt(@NonNull String key, int value) {
    if (!DevToolLifecycle.getInstance().isInitialized()) {
      return;
    }
    // FIXME(mitchilling): loop dependency between DevToolSettings and LynxEnv
    LynxEnv.inst().nativeSetLocalEnv(key, String.valueOf(value));
  }

  // TODO(mitchilling): we need explanation of the purpose and usage of each setting in javadoc
  /**
   * The return value indicates the setting value ONLY.
   * It doesn't necessarily mean DevTool is "enabled" without checking DevTool lifecycle status.
   * <p>
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> true
   * <br><b>Default:</b> false
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public boolean isDevToolEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_DEVTOOL, false);
  }

  public void setDevToolEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_DEVTOOL, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_DEVTOOL, enabled);
  }

  /**
   * The return value indicates the setting value ONLY.
   * It doesn't necessarily mean LogBox is "enabled" without checking DevTool lifecycle status.
   * <p>
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> true
   * <br><b>Default:</b> true
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public boolean isLogBoxEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_LOGBOX, true);
  }

  public void setLogBoxEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_LOGBOX, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_LOGBOX, enabled);
  }

  /**
   * <b>Persistence:</b> false
   * <br><b>Sync to Native:</b> false
   * <br><b>Default:</b> false
   */
  public boolean isHighlightTouchEnabled() {
    return mHighlightTouchEnabled;
  }

  public void setHighlightTouchEnabled(boolean enabled) {
    mHighlightTouchEnabled = enabled;
  }

  /**
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> false
   * <br><b>Default:</b> false
   */
  public boolean isFSPScreenshotEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_FSP_SCREENSHOT, false);
  }

  public void setFSPScreenshotEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_FSP_SCREENSHOT, enabled);
  }

  /**
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> false
   * <br><b>Default:</b> false
   */
  public boolean isLaunchRecordEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_LAUNCH_RECORD, false);
  }

  public void setLaunchRecordEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_LAUNCH_RECORD, enabled);
  }

  /**
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> true
   * <br><b>Default:</b> true
   */
  public boolean isQuickJSDebugEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_QUICKJS_DEBUG, true);
  }

  public void setQuickJSDebugEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_QUICKJS_DEBUG, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_QUICKJS_DEBUG, enabled);
  }

  /**
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> true
   * <br><b>Default:</b> true
   */
  public boolean isDOMTreeEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_DOM_TREE, true);
  }

  public void setDOMTreeEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_DOM_TREE, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_DOM_TREE, enabled);
  }

  /**
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> false
   * <br><b>Default:</b> true
   */
  public boolean isLongPressMenuEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_LONG_PRESS_MENU, true);
  }

  public void setLongPressMenuEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_LONG_PRESS_MENU, enabled);
  }

  /**
   * <b>Persistence:</b> false
   * <br><b>Sync to Native:</b> false
   * <br><b>Default:</b> true
   */
  public boolean isPreviewScreenshotEnabled() {
    return mPreviewScreenshotEnabled;
  }

  public void setPreviewScreenshotEnabled(boolean enabled) {
    mPreviewScreenshotEnabled = enabled;
  }

  /**
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> true
   * <br><b>Default:</b> true
   */
  public boolean isQuickJSCacheEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_QUICKJS_CACHE, true);
  }

  public void setQuickJSCacheEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_QUICKJS_CACHE, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_QUICKJS_CACHE, enabled);
  }

  /**
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> false
   * <br><b>Default:</b> true
   */
  public boolean isPixelCopyEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_PIXEL_COPY, true);
  }

  public void setPixelCopyEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_PIXEL_COPY, enabled);
  }

  /**
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> false
   * <br><b>Default:</b> false
   */
  public boolean isDebugModeEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_DEBUG_MODE, false);
  }

  public void setDebugModeEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_DEBUG_MODE, enabled);
  }

  /**
   * <b>Persistence:</b> true
   * <br><b>Sync to Native:</b> true
   * <br><b>Default:</b> {@link #V8_ALIGN_WITH_PROD}
   */
  public int getV8Enabled() {
    return getPersistedInt(SP_KEY_ENABLE_V8, V8_ALIGN_WITH_PROD);
  }

  public void setV8Enabled(int enabled) {
    setPersistedInt(SP_KEY_ENABLE_V8, enabled);
    syncToNativeInt(SP_KEY_ENABLE_V8, enabled);
  }

  /**
   * API usage: devtool automation
   * enable perf metrics report
   * <b>Persistence:</b> false
   * <br><b>Sync to Native:</b> false
   * <br><b>Default:</b> false
   */
  public boolean isPerfMetricsEnabled() {
    return mPerfMetricsEnabled;
  }

  public void setPerfMetricsEnabled(boolean enabled) {
    mPerfMetricsEnabled = enabled;
  }
}
