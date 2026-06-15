// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import android.content.Context;
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
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.LynxTraceEnv;
import java.util.HashMap;
import java.util.Map;

@Keep
public class LynxDevtoolEnv {
  private final String TAG = "LynxDevtoolEnv";
  private volatile static LynxDevtoolEnv sInstance;
  private Context mContext;
  // be used to load devtool native library
  private INativeLibraryLoader mDevtoolLibraryLoader = null;

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

  private LynxDevtoolEnv() {}

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

      if (mContext == null) {
        mContext = context;
      }

      if (!LynxEnv.inst().isDevLibraryLoaded()) {
        loadNativeDevtoolLibrary();
      }

      MemoryController.getInstance().init(mContext);

      setDefaultAppInfo(context);
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

  public void setDevToolLibraryLoader(INativeLibraryLoader loader) {
    mDevtoolLibraryLoader = loader;
  }

  private boolean shouldLoadQJSBridge() {
    return DevToolSettings.inst().bootstrap().shouldLoadQJSBridge();
  }

  private boolean shouldLoadV8Bridge() {
    return DevToolSettings.inst().bootstrap().shouldLoadV8Bridge();
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
  public boolean isErrorTypeIgnored(int errCode) {
    return DevToolSettings.inst().isErrorTypeIgnored(errCode);
  }

  @Deprecated
  public boolean isIgnoreErrorTypeEnabled(final Integer errCode) {
    return errCode != null && DevToolSettings.inst().isErrorTypeIgnored(errCode);
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
