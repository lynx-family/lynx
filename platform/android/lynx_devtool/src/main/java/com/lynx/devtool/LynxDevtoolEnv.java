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
    // All initializations are done. Let's notify DevToolLifecycle.
    DevToolLifecycle.getInstance().onInitialized();
    DevToolSettings.inst().syncToNative();
    if (LynxGlobalDebugBridge.getInstance().isEnabled()) {
      DevToolLifecycle.getInstance().onConnected();
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
        put(LynxEnvKey.SP_KEY_ENABLE_FSP_SCREENSHOT,
            new ArrayList<Object>(Arrays.asList(true, false, false)));
        put(LynxEnvKey.SP_KEY_ENABLE_LAUNCH_RECORD,
            new ArrayList<Object>(Arrays.asList(true, false, false)));
        put(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG,
            new ArrayList<Object>(Arrays.asList(true, true, true)));
        put(LynxEnvKey.SP_KEY_ENABLE_DOM_TREE,
            new ArrayList<Object>(Arrays.asList(true, true, true)));
        put(LynxEnvKey.SP_KEY_ENABLE_LONG_PRESS_MENU,
            new ArrayList<Object>(Arrays.asList(true, false, true)));
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
      if (setDevToolSettingsValue(key, value)) {
        return;
      }
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
    if (LynxEnvKey.SP_KEY_ACTIVATED_CDP_DOMAINS.equals(groupKey)) {
      DevToolSettings.inst().setEnabledCDPDomains(
          newGroupValues == null ? new HashSet<String>() : newGroupValues);
      return;
    }
    if (LynxEnvKey.SP_KEY_IGNORE_ERROR_TYPES.equals(groupKey)) {
      DevToolSettings.inst().setIgnoredErrorTypes(
          newGroupValues == null ? new HashSet<String>() : newGroupValues);
      return;
    }
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
      Object settingsValue = getDevToolSettingsValue(key);
      if (settingsValue != null) {
        return settingsValue;
      }
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
    if (LynxEnvKey.SP_KEY_ACTIVATED_CDP_DOMAINS.equals(groupKey)) {
      return DevToolSettings.inst().getEnabledCDPDomains();
    }
    if (LynxEnvKey.SP_KEY_IGNORE_ERROR_TYPES.equals(groupKey)) {
      return DevToolSettings.inst().getIgnoredErrorTypes();
    }
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
    Object settingsDefaultValue = getDevToolSettingsDefaultValue(key);
    if (settingsDefaultValue != null) {
      return settingsDefaultValue;
    }
    if (mSwitchAttrMap == null) {
      return null;
    }
    if (mSwitchAttrMap.containsKey(key) && mSwitchAttrMap.get(key) != null) {
      return mSwitchAttrMap.get(key).get(2);
    }
    return null;
  }

  private Object getDevToolSettingsDefaultValue(String key) {
    if (key == null) {
      return null;
    }
    if (key.startsWith(CDP_DOMAIN_KEY_PREFIX)
        || LynxEnvKey.SP_KEY_ENABLE_IGNORE_ERROR_CSS.equals(key)) {
      return false;
    }
    switch (key) {
      case LynxEnvKey.SP_KEY_ENABLE_DEVTOOL:
      case LynxEnvKey.SP_KEY_ENABLE_HIGHLIGHT_TOUCH:
      case LynxEnvKey.SP_KEY_ENABLE_FSP_SCREENSHOT:
      case LynxEnvKey.SP_KEY_ENABLE_LAUNCH_RECORD:
      case LynxEnvKey.SP_KEY_ENABLE_DEBUG_MODE:
      case ENABLE_PERF_METRICS:
        return false;
      case LynxEnvKey.SP_KEY_ENABLE_LOGBOX:
      case LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG:
      case LynxEnvKey.SP_KEY_ENABLE_DOM_TREE:
      case LynxEnvKey.SP_KEY_ENABLE_LONG_PRESS_MENU:
      case LynxEnvKey.SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT:
      case LynxEnvKey.SP_KEY_ENABLE_QUICKJS_CACHE:
      case LynxEnvKey.SP_KEY_ENABLE_PIXEL_COPY:
        return true;
      case LynxEnvKey.SP_KEY_ENABLE_V8:
        return V8_ALIGN_WITH_PROD;
      default:
        return null;
    }
  }

  private boolean setDevToolSettingsValue(String key, Object value) {
    if (key == null || value == null) {
      return false;
    }
    if (key.startsWith(CDP_DOMAIN_KEY_PREFIX) && value instanceof Boolean) {
      DevToolSettings.inst().setCDPDomainEnabled(key, (Boolean) value);
      return true;
    }
    if (LynxEnvKey.SP_KEY_ENABLE_IGNORE_ERROR_CSS.equals(key) && value instanceof Boolean) {
      DevToolSettings.inst().setCSSErrorIgnored((Boolean) value);
      return true;
    }
    switch (key) {
      case LynxEnvKey.SP_KEY_ENABLE_DEVTOOL:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setDevToolEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_LOGBOX:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setLogBoxEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_HIGHLIGHT_TOUCH:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setHighlightTouchEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_FSP_SCREENSHOT:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setFSPScreenshotEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_LAUNCH_RECORD:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setLaunchRecordEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setQuickJSDebugEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_DOM_TREE:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setDOMTreeEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_LONG_PRESS_MENU:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setLongPressMenuEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setPreviewScreenshotEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_QUICKJS_CACHE:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setQuickJSCacheEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_PIXEL_COPY:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setPixelCopyEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_DEBUG_MODE:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setDebugModeEnabled((Boolean) value);
          return true;
        }
        break;
      case LynxEnvKey.SP_KEY_ENABLE_V8:
        if (value instanceof Integer) {
          DevToolSettings.inst().setV8Enabled((Integer) value);
          return true;
        }
        break;
      case ENABLE_PERF_METRICS:
        if (value instanceof Boolean) {
          DevToolSettings.inst().setPerfMetricsEnabled((Boolean) value);
          return true;
        }
        break;
      default:
        break;
    }
    return false;
  }

  private Object getDevToolSettingsValue(String key) {
    if (key == null) {
      return null;
    }
    if (key.startsWith(CDP_DOMAIN_KEY_PREFIX)) {
      return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isCDPDomainEnabled(key));
    }
    if (LynxEnvKey.SP_KEY_ENABLE_IGNORE_ERROR_CSS.equals(key)) {
      return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isCSSErrorIgnored());
    }
    switch (key) {
      case LynxEnvKey.SP_KEY_ENABLE_DEVTOOL:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isDevToolEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_LOGBOX:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isLogBoxEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_HIGHLIGHT_TOUCH:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isHighlightTouchEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_FSP_SCREENSHOT:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isFSPScreenshotEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_LAUNCH_RECORD:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isLaunchRecordEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isQuickJSDebugEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_DOM_TREE:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isDOMTreeEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_LONG_PRESS_MENU:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isLongPressMenuEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT:
        return getMaskedDevToolSettingsValue(
            key, DevToolSettings.inst().isPreviewScreenshotEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_QUICKJS_CACHE:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isQuickJSCacheEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_PIXEL_COPY:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isPixelCopyEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_DEBUG_MODE:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isDebugModeEnabled());
      case LynxEnvKey.SP_KEY_ENABLE_V8:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().getV8Enabled());
      case ENABLE_PERF_METRICS:
        return getMaskedDevToolSettingsValue(key, DevToolSettings.inst().isPerfMetricsEnabled());
      default:
        return null;
    }
  }

  private Object getMaskedDevToolSettingsValue(String key, Object value) {
    Boolean mask = getDevtoolEnvMask(key);
    if (value instanceof Boolean) {
      return (Boolean) value && mask;
    }
    if (value instanceof Integer) {
      return mask ? value : 0;
    }
    return value;
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
    DevToolSettings.inst().setQuickJSCacheEnabled(enabled);
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
    DevToolSettings.inst().setV8Enabled(enabled);
  }

  public boolean isDomTreeEnabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DOM_TREE, true);
  }

  public void enableDomTree(boolean enabled) {
    DevToolSettings.inst().setDOMTreeEnabled(enabled);
  }

  public boolean isLongPressMenuEnabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_LONG_PRESS_MENU, true);
  }

  public void enableLongPressMenu(boolean enabled) {
    DevToolSettings.inst().setLongPressMenuEnabled(enabled);
  }

  public boolean isIgnoreErrorTypeEnabled(final Integer errCode) {
    if (mErrorCodeMap == null || !mErrorCodeMap.containsValue(errCode)) {
      return false;
    }
    return DevToolSettings.inst().isErrorTypeIgnored(errCode);
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
    DevToolSettings.inst().setQuickJSDebugEnabled(enabled);
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
    DevToolSettings.inst().setPerfMetricsEnabled(enable);
  }

  public boolean isPerfMetricsEnabled() {
    return getDevtoolEnv(ENABLE_PERF_METRICS, false);
  }

  public boolean isAttached() {
    return true;
  }

  public void enableAllSessions() {
    DebugRouter.getInstance().enableAllSessions();
  }
}
