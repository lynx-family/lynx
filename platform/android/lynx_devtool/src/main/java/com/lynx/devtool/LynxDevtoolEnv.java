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
import com.lynx.devtool.memory.MemoryController;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.LynxTraceEnv;
import java.util.ArrayList;
import java.util.Arrays;
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
  private Map<String, ArrayList<Object>> mSwitchAttrMap;
  // be used to load devtool native library
  private INativeLibraryLoader mDevtoolLibraryLoader = null;

  enum KeyType { NORMAL_KEY, ERROR_KEY, CDP_DOMAIN_KEY }

  private ReadWriteLock mReadWriteLock;
  private Map<String, Boolean> mSwitchMasks;

  public static final int V8_OFF = 0;
  public static final int V8_ON = 1;
  public static final int V8_ALIGN_WITH_PROD = 2;
  public static final String ENABLE_PERF_METRICS = "enable_perf_metrics";

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

      initSwitchAttribute();
      initSyncToNative();

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
  }

  private void initSwitchAttribute() {
    /**
     mSwitchAttrMap: A dictionary indicating all switches' attributes.
     key: switch name.
     value: an array of bool values indicating attributes of current switch.
      The meaning of each value in array is as follows:
        whether needs to be persisted
        whether needs to be synchronized to native
        default value
     */
    mSwitchAttrMap = new HashMap<String, ArrayList<Object>>() {
      {
        put(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL,
            new ArrayList<Object>(Arrays.asList(true, true, false)));
        put(LynxEnvKey.SP_KEY_ENABLE_LOGBOX,
            new ArrayList<Object>(Arrays.asList(true, true, true)));
        put(LynxEnvKey.SP_KEY_ENABLE_HIGHLIGHT_TOUCH,
            new ArrayList<Object>(Arrays.asList(false, false, false)));
        put(LynxEnvKey.SP_KEY_ENABLE_LAUNCH_RECORD,
            new ArrayList<Object>(Arrays.asList(true, false, false)));
        put(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG,
            new ArrayList<Object>(Arrays.asList(true, true, true)));
        put(LynxEnvKey.SP_KEY_ENABLE_DOM_TREE,
            new ArrayList<Object>(Arrays.asList(true, true, true)));
        put(LynxEnvKey.SP_KEY_ENABLE_LONG_PRESS_MENU,
            new ArrayList<Object>(Arrays.asList(true, false, true)));
        put(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW,
            new ArrayList<Object>(Arrays.asList(false, true, false)));
        put(LynxEnvKey.SP_KEY_DEVTOOL_CONNECTED,
            new ArrayList<Object>(Arrays.asList(false, true, false)));
        put(LynxEnvKey.SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT,
            new ArrayList<Object>(Arrays.asList(false, false, true)));
        put(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_CACHE,
            new ArrayList<Object>(Arrays.asList(true, true, true)));
        put(LynxEnvKey.SP_KEY_ENABLE_PIXEL_COPY,
            new ArrayList<Object>(Arrays.asList(true, false, true)));
        put(LynxEnvKey.SP_KEY_ENABLE_DEBUG_MODE,
            new ArrayList<Object>(Arrays.asList(true, false, false)));
        put(LynxEnvKey.SP_KEY_ENABLE_V8,
            new ArrayList<Object>(Arrays.asList(true, true, V8_ALIGN_WITH_PROD)));
        put(ENABLE_PERF_METRICS, new ArrayList<Object>(Arrays.asList(false, false, false)));
      }
    };
  }

  private void initSyncToNative() {
    syncToNative(
        LynxEnvKey.SP_KEY_ENABLE_DEVTOOL, getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL, false));
    syncToNative(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_CACHE,
        getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_CACHE, true));
    syncToNative(LynxEnvKey.SP_KEY_ENABLE_V8, getV8Enabled());
    syncToNative(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG, isQuickjsDebugEnabled());
    syncToNative(LynxEnvKey.SP_KEY_ENABLE_DOM_TREE, isDomTreeEnabled());
    syncToNative(
        LynxEnvKey.SP_KEY_ENABLE_LOGBOX, getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_LOGBOX, true));
    syncToNative(
        LynxEnvKey.SP_KEY_DEVTOOL_CONNECTED, LynxGlobalDebugBridge.getInstance().isEnabled());
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

  public void loadNativeDevtoolLibrary() {
    if (mDevtoolLibraryLoader != null) {
      mDevtoolLibraryLoader.loadLibrary("lynxdebugrouter");
      mDevtoolLibraryLoader.loadLibrary("v8_libfull.cr");
      mDevtoolLibraryLoader.loadLibrary("basedevtool");
      mDevtoolLibraryLoader.loadLibrary("lynxdevtool");
      LLog.i(TAG, "liblynxdevtool and libv8_libfull loaded via custom devtool library loader");
    } else {
      if (LynxEnv.inst().getLibraryLoader() != null) {
        LynxEnv.inst().getLibraryLoader().loadLibrary("lynxdebugrouter");
        LynxEnv.inst().getLibraryLoader().loadLibrary("v8_libfull.cr");
        LynxEnv.inst().getLibraryLoader().loadLibrary("basedevtool");
        LynxEnv.inst().getLibraryLoader().loadLibrary("lynxdevtool");
        LLog.i(TAG, "liblynxdevtool loaded via library loader");
      } else {
        System.loadLibrary("v8_libfull.cr");
        System.loadLibrary("basedevtool");
        System.loadLibrary("lynxdevtool");
        LLog.i(TAG, "liblynxdevtool loaded via system loader");
      }
    }
    LynxEnv.inst().setDevLibraryLoaded(true);
  }

  private void syncToNative(String key, Object defaultValue) {
    if (key == null || defaultValue == null) {
      throw new RuntimeException("syncToNative failed! key or defaultValue is null");
    }
    if (getDefaultValue(key) == null
        || (!getDefaultValue(key).getClass().equals(defaultValue.getClass()))) {
      throw new RuntimeException(
          "syncToNative failed! key: " + key + "value: " + defaultValue.toString());
    }
    if (defaultValue instanceof Boolean) {
      LynxEnv.inst().nativeSetLocalEnv(key, ((Boolean) defaultValue).booleanValue() ? "1" : "0");
    } else {
      LynxEnv.inst().nativeSetLocalEnv(key, String.valueOf((Integer) defaultValue));
    }
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

  private void syncMaskToNative(String key) {
    LynxEnv.inst().nativeSetEnvMask(key, getDevtoolEnvMask(key));
  }

  public void setDevtoolEnv(String key, Object value) {
    try {
      boolean persist = needPersist(key);
      boolean syncToNative = needSyncToNative(key);
      KeyType type = getKeyType(key);
      switch (type) {
        case NORMAL_KEY:
          setDevtoolEnvInternal(key, value, persist, syncToNative);
          break;
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

  public Integer getDevtoolEnv(String key, Integer defaultValue) {
    return (Integer) getDevtoolObjectEnv(key, defaultValue);
  }

  // This function will be called in LynxInspectorOwner when handle GetGlobalSwitch messages.
  Object getDevtoolObjectEnv(String key, Object defaultValue) {
    try {
      KeyType type = getKeyType(key);
      switch (type) {
        case NORMAL_KEY:
          return getDevtoolEnvInternal(key, defaultValue);
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

  public Object getDefaultValue(String key) {
    if (mSwitchAttrMap == null) {
      return null;
    }
    if (mSwitchAttrMap.containsKey(key) && mSwitchAttrMap.get(key) != null) {
      return mSwitchAttrMap.get(key).get(2);
    }
    return null;
  }

  private void setDevtoolEnvInternal(
      String key, Object value, Boolean persist, Boolean syncToNative) {
    if (getDefaultValue(key) == null
        || (!getDefaultValue(key).getClass().equals(value.getClass()))) {
      throw new RuntimeException(
          "setDevtoolEnvInternal failed! key: " + key + ", value: " + value.toString());
    }
    if (persist && mSharedPreferences != null) {
      if (value instanceof Boolean) {
        mSharedPreferences.edit().putBoolean(key, (Boolean) value).apply();
      } else {
        mSharedPreferences.edit().putInt(key, (Integer) value).apply();
      }
    } else if (!persist) {
      mSwitchNotPersist.put(key, value);
    }
    if (syncToNative) {
      syncToNative(key, value);
    }
  }

  private Object getDevtoolEnvInternal(String key, Object defaultValue) {
    if (getDefaultValue(key) == null
        || (!getDefaultValue(key).getClass().equals(defaultValue.getClass()))) {
      throw new RuntimeException(
          "getDevtoolEnvInternal failed! key: " + key + ", value: " + defaultValue.toString());
    }
    Boolean mask = getDevtoolEnvMask(key);
    if (mSharedPreferences != null) {
      if (defaultValue instanceof Boolean) {
        if (mSharedPreferences.contains(key) || !mSwitchNotPersist.containsKey(key)) {
          return mSharedPreferences.getBoolean(key, (Boolean) defaultValue) && mask;
        } else {
          return mask && (Boolean) mSwitchNotPersist.get(key);
        }
      } else {
        if (mSharedPreferences.contains(key) || !mSwitchNotPersist.containsKey(key)) {
          return mask ? mSharedPreferences.getInt(key, (Integer) defaultValue) : 0;
        } else {
          return mask ? mSwitchNotPersist.get(key) : 0;
        }
      }
    } else {
      LLog.e(TAG, "getDevtoolEnv must be called after init! key: " + key);
      if (defaultValue instanceof Boolean) {
        return mask && (Boolean) defaultValue;
      } else {
        return mask ? defaultValue : 0;
      }
    }
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

  public void setDevtoolEnvMask(String key, boolean value) {
    if (mSwitchMasks != null && mReadWriteLock != null) {
      mReadWriteLock.writeLock().lock();
      mSwitchMasks.put(key, value);
      mReadWriteLock.writeLock().unlock();
      syncMaskToNative(key);
    }
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

  public boolean isQuickjsCacheEnabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_CACHE, true);
  }

  public void enableQuickjsCache(boolean enabled) {
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_CACHE, enabled);
  }

  public int getV8Enabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_V8, V8_ALIGN_WITH_PROD);
  }

  // 0 means Off, 1 means On, 2 means AlignWithProd. Use 2 as default.
  // When the value is 2, the V8 engine will be used on LynxView which configured with enable_v8,
  // and the Quickjs engine will be used on other LynxView.
  public void enableV8(int enabled) {
    if (enabled > V8_ALIGN_WITH_PROD) {
      enabled = V8_ALIGN_WITH_PROD;
      LLog.w(TAG, "The value must be 0 or 1 or 2. Change the value to 2!");
    } else if (enabled < V8_OFF) {
      enabled = V8_OFF;
      LLog.w(TAG, "The value must be 0 or 1 or 2. Change the value to 0!");
    }
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_V8, enabled);
  }

  public boolean isDomTreeEnabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DOM_TREE, true);
  }

  public void enableDomTree(boolean enabled) {
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DOM_TREE, enabled);
  }

  public boolean isLongPressMenuEnabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_LONG_PRESS_MENU, true);
  }

  public void enableLongPressMenu(boolean enabled) {
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_LONG_PRESS_MENU, enabled);
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
      case NORMAL_KEY:
        if (mSwitchAttrMap != null && mSwitchAttrMap.containsKey(key)
            && mSwitchAttrMap.get(key) != null) {
          return (Boolean) mSwitchAttrMap.get(key).get(0);
        }
      default:
        return false;
    }
  }

  private boolean needSyncToNative(String key) {
    KeyType type = getKeyType(key);
    switch (type) {
      case CDP_DOMAIN_KEY:
        return true;
      case NORMAL_KEY:
        if (mSwitchAttrMap != null && mSwitchAttrMap.containsKey(key)
            && mSwitchAttrMap.get(key) != null) {
          return (Boolean) mSwitchAttrMap.get(key).get(1);
        }
      default:
        return false;
    }
  }

  public void enableQuickjsDebug(boolean enabled) {
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG, enabled);
  }

  public boolean isQuickjsDebugEnabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG, true);
  }

  public boolean isPreviewScreenShotEnabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT, true);
  }

  /**
   * API usage: devtool automation
   * @param enable <code>true</code> enable perf metrics report, <code>false</code> otherwise
   */
  public void enablePerfMetrics(boolean enable) {
    setDevtoolEnv(ENABLE_PERF_METRICS, enable);
  }

  public boolean isPerfMetricsEnabled() {
    return getDevtoolEnv(ENABLE_PERF_METRICS, false);
  }

  public boolean isAttached() {
    return true;
  }
}
