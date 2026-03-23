// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import androidx.annotation.Keep;
import com.example.lynxdevtool.BuildConfig;
import com.lynx.config.LynxLiteConfigs;
import com.lynx.debugrouter.DebugRouter;
import com.lynx.devtool.memory.MemoryController;
import com.lynx.devtoolwrapper.DevToolLifecycle;
import com.lynx.devtoolwrapper.DevToolSettings;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.LynxTraceEnv;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

@Keep
public class LynxDevtoolEnv {
  private final String TAG = "LynxDevtoolEnv";
  private final String ERROR_CODE_KEY_PREFIX = "error_code";
  private final String CDP_DOMAIN_KEY_PREFIX = "enable_cdp_domain";
  private volatile static LynxDevtoolEnv sInstance;
  private Context mContext;
  private SharedPreferences mSharedPreferences;
  private Map<String, Integer> mErrorCodeMap;
  private Map<String, Set<String>> mGroupSets;
  private Map<String, Object> mSwitchNotPersist;
  // be used to load devtool native library
  private INativeLibraryLoader mDevtoolLibraryLoader = null;

  enum KeyType { NORMAL_KEY, ERROR_KEY, CDP_DOMAIN_KEY }

  private ReadWriteLock mReadWriteLock;
  private Map<String, Boolean> mSwitchMasks;

  // TODO(mitchilling): remove this deprecated value
  @Deprecated public static final String ENABLE_PERF_METRICS = "enable_perf_metrics";

  public static LynxDevtoolEnv inst() {
    if (sInstance == null) {
      synchronized (LynxDevtoolEnv.class) {
        if (sInstance == null) {
          sInstance = new LynxDevtoolEnv();
        }
      }
    }
    return sInstance;
  }

  private LynxDevtoolEnv() {
    mErrorCodeMap = new HashMap<>();
    mReadWriteLock = new ReentrantReadWriteLock(true);
    mSwitchMasks = new HashMap<>();
    mGroupSets = new HashMap<>();
    mSwitchNotPersist = new HashMap<>();
  }

  public String getVersion() {
    return BuildConfig.LYNX_SDK_VERSION;
  }

  public void init(Context context) {
    try {
      if (!LynxEnv.inst().isNativeLibraryLoaded()) {
        if (tryLoadDebugLynxLibrary(
                LynxEnv.inst().getLibraryLoader())) { // load lynx_debug.so when in debug mode
          LynxEnv.inst().setNativeLibraryLoaded(true);
        } else {
          LynxEnv.inst().initNativeLibraries(LynxEnv.inst().getLibraryLoader());
        }
      }

      if (mContext == null || mSharedPreferences == null) {
        mContext = context;
        if (context != null) {
          mSharedPreferences = context.getSharedPreferences(LynxEnv.SP_NAME, Context.MODE_PRIVATE);
        }
      }

      if (!LynxEnv.inst().isDevLibraryLoaded()) {
        loadNativeDevtoolLibrary();
      }

      MemoryController.getInstance().init(mContext);

      setDefaultAppInfo(context);
      initEnvGroups();
    } catch (Throwable t) {
      LLog.e(TAG, t.toString());
      throw t;
    }
    // All initializations are done. Let's notify DevToolLifecycle.
    DevToolLifecycle.getInstance().onInitialized();
    // Synchronize settings to native after initialization
    DevToolSettings.inst().syncToNative();
    if (LynxGlobalDebugBridge.getInstance().isEnabled()) {
      DevToolLifecycle.getInstance().onConnected();
    }
  }
  private void setDefaultAppInfo(Context context) {
    Map<String, String> appInfo = new HashMap<>();
    try {
      ApplicationInfo ai = context.getPackageManager().getApplicationInfo(
          context.getPackageName(), PackageManager.GET_META_DATA);
      String appName = ai.loadLabel(context.getPackageManager()).toString();
      if (appName != null) {
        appInfo.put("App", appName);
      }
    } catch (Throwable t) {
      LLog.w(TAG, t.toString());
    }
    LynxGlobalDebugBridge.getInstance().setAppInfo(context, appInfo);
  }

  private void initEnvGroups() {
    // only put error code which could be ignored
    mErrorCodeMap.put(LynxEnvKey.SP_KEY_ENABLE_IGNORE_ERROR_CSS, LynxSubErrorCode.E_CSS);
    if (mSharedPreferences == null) {
      return;
    }

    try {
      Set<String> ignoredErrors =
          mSharedPreferences.getStringSet(LynxEnvKey.SP_KEY_IGNORE_ERROR_TYPES, null);
      if (ignoredErrors != null) {
        mGroupSets.put(LynxEnvKey.SP_KEY_IGNORE_ERROR_TYPES, ignoredErrors);
      }
    } catch (Throwable t) {
      LLog.e(TAG, "failed to initEnvGroups: " + t.toString());
    }
  }

  public void setDevToolLibraryLoader(INativeLibraryLoader loader) {
    mDevtoolLibraryLoader = loader;
  }

  private boolean shouldLoadQJSBridge() {
    ILynxDevToolService devtoolService =
        LynxServiceCenter.inst().getService(ILynxDevToolService.class);
    return devtoolService != null && devtoolService.getLoadQJSBridge();
  }

  private boolean shouldLoadV8Bridge() {
    ILynxDevToolService devtoolService =
        LynxServiceCenter.inst().getService(ILynxDevToolService.class);
    return devtoolService != null && devtoolService.getLoadV8Bridge();
  }

  public void loadNativeDevtoolLibrary() {
    if (mDevtoolLibraryLoader != null) {
      mDevtoolLibraryLoader.loadLibrary("lynxdebugrouter");
      mDevtoolLibraryLoader.loadLibrary("basedevtool");
      mDevtoolLibraryLoader.loadLibrary("lynxdevtool");
      if (shouldLoadQJSBridge()) {
        mDevtoolLibraryLoader.loadLibrary("lynxdevtool_qjs_bridge");
      }
      if (shouldLoadV8Bridge()) {
        mDevtoolLibraryLoader.loadLibrary("v8_libfull.cr");
        mDevtoolLibraryLoader.loadLibrary("lynxdevtool_v8_bridge");
      }
      LLog.i(TAG, "liblynxdevtool and libv8_libfull loaded via custom devtool library loader");
    } else {
      if (LynxEnv.inst().getLibraryLoader() != null) {
        LynxEnv.inst().getLibraryLoader().loadLibrary("lynxdebugrouter");
        LynxEnv.inst().getLibraryLoader().loadLibrary("basedevtool");
        LynxEnv.inst().getLibraryLoader().loadLibrary("lynxdevtool");
        if (shouldLoadQJSBridge()) {
          LynxEnv.inst().getLibraryLoader().loadLibrary("lynxdevtool_qjs_bridge");
        }
        if (shouldLoadV8Bridge()) {
          LynxEnv.inst().getLibraryLoader().loadLibrary("v8_libfull.cr");
          LynxEnv.inst().getLibraryLoader().loadLibrary("lynxdevtool_v8_bridge");
        }
        LLog.i(TAG, "liblynxdevtool loaded via library loader");
      } else {
        System.loadLibrary("basedevtool");
        System.loadLibrary("lynxdevtool");
        if (shouldLoadQJSBridge()) {
          System.loadLibrary("lynxdevtool_qjs_bridge");
        }
        if (shouldLoadV8Bridge()) {
          System.loadLibrary("v8_libfull.cr");
          System.loadLibrary("lynxdevtool_v8_bridge");
        }
        LLog.i(TAG, "liblynxdevtool loaded via system loader");
      }
    }
    LynxEnv.inst().setDevLibraryLoaded(true);
  }

  private void syncToNative(String key, boolean defaultValue, String groupKey) {
    LynxEnv.inst().nativeSetGroupedEnv(key, defaultValue, groupKey);
  }

  private void syncToNative(String groupKey, Set<String> newGroupValues) {
    if (newGroupValues == null) {
      return;
    }
    LynxEnv.inst().nativeSetGroupedEnvWithGroupSet(groupKey, newGroupValues);
  }

  public void setDevtoolEnv(String key, Object value) {
    try {
      boolean persist = needPersist(key);
      boolean syncToNative = needSyncToNative(key);
      KeyType type = getKeyType(key);
      switch (type) {
        case CDP_DOMAIN_KEY:
          setDevtoolGroupedEnvInternal(
              key, LynxEnvKey.SP_KEY_ACTIVATED_CDP_DOMAINS, (Boolean) value, persist, syncToNative);
          break;
        case ERROR_KEY:
          Integer errorCode = mErrorCodeMap.get(key);
          if (errorCode != null) {
            setDevtoolGroupedEnvInternal(errorCode.toString(), LynxEnvKey.SP_KEY_IGNORE_ERROR_TYPES,
                (Boolean) value, persist, syncToNative);
          }
          break;
        default:
          LLog.e(TAG, "setDevtoolEnv, unsupported key: " + key);
          break;
      }
    } catch (RuntimeException e) {
      LLog.e(TAG, e.toString() + ", key: " + key + ", value: " + value.toString());
    }
  }

  public void setDevtoolEnv(String groupKey, Set<String> newGroupValues) {
    if (mGroupSets == null || newGroupValues == null || newGroupValues.isEmpty()) {
      return;
    }
    mGroupSets.put(groupKey, newGroupValues);
    String key = newGroupValues.iterator().next();
    if (mSharedPreferences != null && needPersist(key)) {
      mSharedPreferences.edit().putStringSet(groupKey, newGroupValues).apply();
    }
    if (needSyncToNative(key)) {
      syncToNative(groupKey, newGroupValues);
    }
  }

  public Boolean getDevtoolEnv(String key, Boolean defaultValue) {
    return (Boolean) getDevtoolObjectEnv(key, defaultValue);
  }

  // This function will be called in LynxInspectorOwner when handle GetGlobalSwitch messages.
  Object getDevtoolObjectEnv(String key, Object defaultValue) {
    try {
      KeyType type = getKeyType(key);
      switch (type) {
        case CDP_DOMAIN_KEY:
          return getDevtoolGroupedEnvInternal(
              key, LynxEnvKey.SP_KEY_ACTIVATED_CDP_DOMAINS, (Boolean) defaultValue);
        case ERROR_KEY:
          Integer errorCode = mErrorCodeMap.get(key);
          if (errorCode == null) {
            return false;
          }
          return getDevtoolGroupedEnvInternal(
              errorCode.toString(), LynxEnvKey.SP_KEY_IGNORE_ERROR_TYPES, (Boolean) defaultValue);
        default:
          LLog.e(TAG, "getDevtoolObjectEnv, unsupported key: " + key);
          return defaultValue;
      }
    } catch (RuntimeException e) {
      LLog.e(TAG, e.toString() + ", key: " + key + ", value: " + defaultValue.toString());
    }
    return defaultValue;
  }

  public Set<String> getDevtoolEnv(String groupKey) {
    if (mGroupSets == null) {
      return new HashSet<String>();
    }
    Set<String> set = mGroupSets.get(groupKey);
    if (set == null) {
      return new HashSet<String>();
    }
    return set;
  }

  private void setDevtoolGroupedEnvInternal(
      String switchKey, String groupKey, Boolean value, Boolean persist, Boolean syncToNative) {
    if (mGroupSets == null) {
      return;
    }
    Set<String> groupSet = mGroupSets.get(groupKey);
    if (groupSet == null) {
      groupSet = new HashSet<>();
      mGroupSets.put(groupKey, groupSet);
    }
    if (value) {
      groupSet.add(switchKey);
    } else {
      groupSet.remove(switchKey);
    }
    if (mSharedPreferences != null && persist) {
      mSharedPreferences.edit().putStringSet(groupKey, groupSet).apply();
    }
    if (syncToNative) {
      syncToNative(switchKey, value, groupKey);
    }
  }

  private Boolean getDevtoolGroupedEnvInternal(
      String switchKey, String groupKey, Boolean defaultValue) {
    if (mGroupSets == null) {
      return defaultValue;
    }
    Set<String> set = mGroupSets.get(groupKey);
    return set != null ? set.contains(switchKey) : defaultValue;
  }

  public Boolean getDevtoolEnvMask(String key) {
    Boolean mask = null;
    if (mSwitchMasks != null && mReadWriteLock != null) {
      mReadWriteLock.readLock().lock();
      mask = mSwitchMasks.get(key);
      mReadWriteLock.readLock().unlock();
    }
    if (mask == null) {
      return true;
    }
    return mask;
  }

  private boolean tryLoadDebugLynxLibrary(INativeLibraryLoader nativeLibraryLoader) {
    if (LynxEnv.inst().isDebugModeEnabled()) {
      try {
        if (nativeLibraryLoader != null) {
          // Make sure load quick.so first
          if (LynxLiteConfigs.requireQuickSharedLibrary()) {
            nativeLibraryLoader.loadLibrary("quick");
          }
          nativeLibraryLoader.loadLibrary("lynx_debug");
          if (!LynxTraceEnv.inst().isNativeLibraryLoaded()) {
            nativeLibraryLoader.loadLibrary("lynxtrace");
            LynxTraceEnv.inst().markNativeLibraryLoaded(true);
          }
        } else {
          // Make sure load quick.so first
          if (LynxLiteConfigs.requireQuickSharedLibrary()) {
            System.loadLibrary("quick");
          }
          System.loadLibrary("lynx_debug");
        }
        return true;
      } catch (UnsatisfiedLinkError error) {
        LLog.e(TAG, "Debug Lynx Library load from system with error message " + error.getMessage());
        return false;
      }
    }
    return false;
  }

  @Deprecated
  public boolean showDevtoolBadge() {
    return false;
  }

  @Deprecated
  public void setShowDevtoolBadge(boolean show) {}

  // TODO(mitchilling): remove these deprecated methods calling DevToolSettings
  @Deprecated
  public boolean isQuickjsCacheEnabled() {
    return DevToolSettings.inst().isQuickJSCacheEnabled();
  }

  @Deprecated
  public void enableQuickjsCache(boolean enabled) {
    DevToolSettings.inst().setQuickJSCacheEnabled(enabled);
  }

  @Deprecated
  public int getV8Enabled() {
    return DevToolSettings.inst().getV8Enabled();
  }

  // 0 means Off, 1 means On, 2 means AlignWithProd. Use 2 as default.
  // When the value is 2, the V8 engine will be used on LynxView which configured with enable_v8,
  // and the Quickjs engine will be used on other LynxView.
  @Deprecated
  public void enableV8(int enabled) {
    // TODO(mitchilling): if we do integrity check here, these values will stay public
    if (enabled > DevToolSettings.V8_ALIGN_WITH_PROD) {
      enabled = DevToolSettings.V8_ALIGN_WITH_PROD;
      LLog.w(TAG, "The value must be 0 or 1 or 2. Change the value to 2!");
    } else if (enabled < DevToolSettings.V8_OFF) {
      enabled = DevToolSettings.V8_OFF;
      LLog.w(TAG, "The value must be 0 or 1 or 2. Change the value to 0!");
    }
    DevToolSettings.inst().setV8Enabled(enabled);
  }

  @Deprecated
  public boolean isDomTreeEnabled() {
    return DevToolSettings.inst().isDOMTreeEnabled();
  }

  @Deprecated
  public void enableDomTree(boolean enabled) {
    DevToolSettings.inst().setDOMTreeEnabled(enabled);
  }

  @Deprecated
  public boolean isLongPressMenuEnabled() {
    return DevToolSettings.inst().isLongPressMenuEnabled();
  }

  @Deprecated
  public void enableLongPressMenu(boolean enabled) {
    DevToolSettings.inst().setLongPressMenuEnabled(enabled);
  }

  public boolean isIgnoreErrorTypeEnabled(final Integer errCode) {
    if (mErrorCodeMap == null || !mErrorCodeMap.containsValue(errCode)) {
      return false;
    }
    return getDevtoolGroupedEnvInternal(
        errCode.toString(), LynxEnvKey.SP_KEY_IGNORE_ERROR_TYPES, false);
  }

  private KeyType getKeyType(String key) {
    if (key.startsWith(ERROR_CODE_KEY_PREFIX)) {
      return KeyType.ERROR_KEY;
    } else if (key.startsWith(CDP_DOMAIN_KEY_PREFIX)) {
      return KeyType.CDP_DOMAIN_KEY;
    } else {
      return KeyType.NORMAL_KEY;
    }
  }

  private boolean needPersist(String key) {
    KeyType type = getKeyType(key);
    switch (type) {
      case ERROR_KEY:
        return true;
      default:
        return false;
    }
  }

  private boolean needSyncToNative(String key) {
    KeyType type = getKeyType(key);
    switch (type) {
      case CDP_DOMAIN_KEY:
        return true;
      default:
        return false;
    }
  }

  @Deprecated
  public void enableQuickjsDebug(boolean enabled) {
    DevToolSettings.inst().setQuickJSDebugEnabled(enabled);
  }

  @Deprecated
  public boolean isQuickjsDebugEnabled() {
    return DevToolSettings.inst().isQuickJSDebugEnabled();
  }

  @Deprecated
  public boolean isPreviewScreenShotEnabled() {
    return DevToolSettings.inst().isPreviewScreenshotEnabled();
  }

  /**
   * API usage: devtool automation
   * @param enable <code>true</code> enable perf metrics report, <code>false</code> otherwise
   */
  @Deprecated
  public void enablePerfMetrics(boolean enable) {
    DevToolSettings.inst().setPerfMetricsEnabled(enable);
  }

  @Deprecated
  public boolean isPerfMetricsEnabled() {
    return DevToolSettings.inst().isPerfMetricsEnabled();
  }

  public boolean isAttached() {
    return true;
  }

  public void enableAllSessions() {
    DebugRouter.getInstance().enableAllSessions();
  }
}
