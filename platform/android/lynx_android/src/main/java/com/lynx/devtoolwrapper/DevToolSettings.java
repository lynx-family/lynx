// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

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

  private static final String SP_NAME = "lynx_env_config";

  // TODO(mitchilling): change these keys to private when encapsulated.
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
  private static final String SP_KEY_IGNORE_ERROR_TYPES = "ignore_error_types";

  private static final String SP_KEY_ACTIVATED_CDP_DOMAINS = "activated_cdp_domains";
  private static final String CDP_DOMAIN_KEY_PREFIX = "enable_cdp_domain_";
  public static final String SP_KEY_ENABLE_CDP_DOMAIN_CSS = "enable_cdp_domain_css";
  public static final String SP_KEY_ENABLE_CDP_DOMAIN_DEBUGGER = "enable_cdp_domain_debugger";
  public static final String SP_KEY_ENABLE_CDP_DOMAIN_DOM = "enable_cdp_domain_dom";
  public static final String SP_KEY_ENABLE_CDP_DOMAIN_OVERLAY = "enable_cdp_domain_overlay";
  public static final String SP_KEY_ENABLE_CDP_DOMAIN_PAGE = "enable_cdp_domain_page";
  public static final String SP_KEY_ENABLE_CDP_DOMAIN_RUNTIME = "enable_cdp_domain_runtime";

  public static final int V8_OFF = 0;
  public static final int V8_ON = 1;
  public static final int V8_ALIGN_WITH_PROD = 2;

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
    syncToNativeEnabledCDPDomains(getEnabledCDPDomains());
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

  private Set<String> getPersistedStringSet(String key) {
    if (mSharedPreferences == null) {
      return new HashSet<>();
    }
    Set<String> values = mSharedPreferences.getStringSet(key, Collections.emptySet());
    if (values == null) {
      return new HashSet<>();
    }
    return new HashSet<>(values);
  }

  private void setPersistedStringSet(String key, @NonNull Set<String> values) {
    if (mSharedPreferences != null) {
      mSharedPreferences.edit().putStringSet(key, new HashSet<>(values)).apply();
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
    LynxEnv.inst().nativeSetLocalEnv(key, value ? "1" : "0");
  }

  private void syncToNativeInt(@NonNull String key, int value) {
    if (!DevToolLifecycle.getInstance().isInitialized()) {
      return;
    }
    LynxEnv.inst().nativeSetLocalEnv(key, String.valueOf(value));
  }

  private void syncToNativeEnabledCDPDomains(@NonNull Set<String> domains) {
    if (!DevToolLifecycle.getInstance().isInitialized()) {
      return;
    }
    LynxEnv.inst().nativeSetGroupedEnvWithGroupSet(
        SP_KEY_ACTIVATED_CDP_DOMAINS, new HashSet<>(domains));
  }

  private boolean verifyCDPDomainKey(@NonNull String key) {
    if (!key.startsWith(CDP_DOMAIN_KEY_PREFIX)) {
      LLog.e(TAG, "Invalid CDP domain key: " + key);
      return false;
    }
    return true;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public boolean isDevToolEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_DEVTOOL, false);
  }

  public void setDevToolEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_DEVTOOL, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_DEVTOOL, enabled);
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public boolean isLogBoxEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_LOGBOX, true);
  }

  public void setLogBoxEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_LOGBOX, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_LOGBOX, enabled);
  }

  public boolean isHighlightTouchEnabled() {
    return mHighlightTouchEnabled;
  }

  public void setHighlightTouchEnabled(boolean enabled) {
    mHighlightTouchEnabled = enabled;
  }

  public boolean isFSPScreenshotEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_FSP_SCREENSHOT, false);
  }

  public void setFSPScreenshotEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_FSP_SCREENSHOT, enabled);
  }

  public boolean isLaunchRecordEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_LAUNCH_RECORD, false);
  }

  public void setLaunchRecordEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_LAUNCH_RECORD, enabled);
  }

  public boolean isQuickJSDebugEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_QUICKJS_DEBUG, true);
  }

  public void setQuickJSDebugEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_QUICKJS_DEBUG, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_QUICKJS_DEBUG, enabled);
  }

  public boolean isDOMTreeEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_DOM_TREE, true);
  }

  public void setDOMTreeEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_DOM_TREE, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_DOM_TREE, enabled);
  }

  public boolean isLongPressMenuEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_LONG_PRESS_MENU, true);
  }

  public void setLongPressMenuEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_LONG_PRESS_MENU, enabled);
  }

  public boolean isPreviewScreenshotEnabled() {
    return mPreviewScreenshotEnabled;
  }

  public void setPreviewScreenshotEnabled(boolean enabled) {
    mPreviewScreenshotEnabled = enabled;
  }

  public boolean isQuickJSCacheEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_QUICKJS_CACHE, true);
  }

  public void setQuickJSCacheEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_QUICKJS_CACHE, enabled);
    syncToNativeBoolean(SP_KEY_ENABLE_QUICKJS_CACHE, enabled);
  }

  public boolean isPixelCopyEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_PIXEL_COPY, true);
  }

  public void setPixelCopyEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_PIXEL_COPY, enabled);
  }

  public boolean isDebugModeEnabled() {
    return getPersistedBoolean(SP_KEY_ENABLE_DEBUG_MODE, false);
  }

  public void setDebugModeEnabled(boolean enabled) {
    setPersistedBoolean(SP_KEY_ENABLE_DEBUG_MODE, enabled);
  }

  public int getV8Enabled() {
    return getPersistedInt(SP_KEY_ENABLE_V8, V8_ALIGN_WITH_PROD);
  }

  public void setV8Enabled(int enabled) {
    setPersistedInt(SP_KEY_ENABLE_V8, enabled);
    syncToNativeInt(SP_KEY_ENABLE_V8, enabled);
  }

  public boolean isPerfMetricsEnabled() {
    return mPerfMetricsEnabled;
  }

  public void setPerfMetricsEnabled(boolean enabled) {
    mPerfMetricsEnabled = enabled;
  }

  public boolean isCSSErrorIgnored() {
    return isErrorTypeIgnored(LynxSubErrorCode.E_CSS);
  }

  public void setCSSErrorIgnored(boolean ignored) {
    setErrorTypeIgnored(LynxSubErrorCode.E_CSS, ignored);
  }

  @NonNull
  public Set<String> getIgnoredErrorTypes() {
    return getPersistedStringSet(SP_KEY_IGNORE_ERROR_TYPES);
  }

  public void setIgnoredErrorTypes(@NonNull Collection<String> errorTypes) {
    setPersistedStringSet(SP_KEY_IGNORE_ERROR_TYPES, new HashSet<>(errorTypes));
  }

  public boolean isErrorTypeIgnored(int errorType) {
    return getIgnoredErrorTypes().contains(String.valueOf(errorType));
  }

  public void setErrorTypeIgnored(int errorType, boolean ignored) {
    Set<String> ignoredErrorTypes = getIgnoredErrorTypes();
    String errorTypeKey = String.valueOf(errorType);
    boolean changed =
        ignored ? ignoredErrorTypes.add(errorTypeKey) : ignoredErrorTypes.remove(errorTypeKey);
    if (!changed) {
      return;
    }
    setPersistedStringSet(SP_KEY_IGNORE_ERROR_TYPES, ignoredErrorTypes);
  }

  @NonNull
  public Set<String> getEnabledCDPDomains() {
    return getPersistedStringSet(SP_KEY_ACTIVATED_CDP_DOMAINS);
  }

  public void setEnabledCDPDomains(@NonNull Collection<String> domains) {
    Set<String> domainSnapshot = new HashSet<>(domains);
    for (String key : domainSnapshot) {
      if (!verifyCDPDomainKey(key)) {
        return;
      }
    }
    setPersistedStringSet(SP_KEY_ACTIVATED_CDP_DOMAINS, domainSnapshot);
    syncToNativeEnabledCDPDomains(domainSnapshot);
  }

  public boolean isCDPDomainEnabled(@NonNull String key) {
    if (!verifyCDPDomainKey(key)) {
      return false;
    }
    return getEnabledCDPDomains().contains(key);
  }

  public void setCDPDomainEnabled(@NonNull String key, boolean enabled) {
    if (!verifyCDPDomainKey(key)) {
      return;
    }
    Set<String> enabledDomains = getEnabledCDPDomains();
    boolean changed = enabled ? enabledDomains.add(key) : enabledDomains.remove(key);
    if (!changed) {
      return;
    }
    setPersistedStringSet(SP_KEY_ACTIVATED_CDP_DOMAINS, enabledDomains);
    syncToNativeEnabledCDPDomains(enabledDomains);
  }
}
