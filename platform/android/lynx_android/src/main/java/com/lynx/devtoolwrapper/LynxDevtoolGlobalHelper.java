// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.view.ViewGroup;
import android.widget.Toast;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;
import org.json.JSONObject;

@Keep
public class LynxDevtoolGlobalHelper {
  private static final String TAG = "LynxDevtoolGlobalHelper";

  private static volatile ILynxDevToolService sDevToolService = null;

  static ILynxDevToolService getDevToolService() {
    if (sDevToolService == null) {
      sDevToolService = LynxServiceCenter.inst().getService(ILynxDevToolService.class);
    }
    return sDevToolService;
  }

  // Remote debug stuff
  private boolean remoteDebugAvailable = false;
  // Singleton
  public static LynxDevtoolGlobalHelper getInstance() {
    return SingletonHolder.INSTANCE;
  }

  private Map<String, String> mAppInfo;

  private static class SingletonHolder {
    private static final LynxDevtoolGlobalHelper INSTANCE = new LynxDevtoolGlobalHelper();
  }

  private LynxDevtoolGlobalHelper() {
    mAppInfo = new HashMap<>();
    mAppInfo.put("sdkVersion", LynxEnv.inst().getLynxVersion());
    if (LynxEnv.inst().isLynxDebugEnabled()) {
      initRemoteDebugIfNecessary();
    }
  }

  private boolean initRemoteDebugIfNecessary() {
    if (!LynxEnv.inst().isLynxDebugEnabled()) {
      return false;
    }
    if (!LynxEnv.inst().isNativeLibraryLoaded()) {
      LLog.w(TAG, "liblynx.so not loaded!");
      return false;
    }

    if (remoteDebugAvailable) {
      return true;
    }

    remoteDebugAvailable = true;

    return remoteDebugAvailable;
  }

  public void setAppInfo(Context context, Map<String, String> appInfo) {
    if (appInfo != null) {
      mAppInfo.putAll(appInfo);
    }

    if (!initRemoteDebugIfNecessary()) {
      return;
    }

    ILynxDevToolService devToolService = getDevToolService();
    if (devToolService != null) {
      devToolService.globalDebugBridgeSetAppInfo(context, mAppInfo);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }

  public void setAppInfo(Context context, String appName, String appVersion) {
    Map<String, String> appInfo = new HashMap<>();
    appInfo.put("App", appName);
    appInfo.put("AppVersion", appVersion);
    setAppInfo(context, appInfo);
  }

  @Deprecated
  public void setAppInfo(String appName, String appVersion) {
    setAppInfo(null, appName, appVersion);
  }

  public boolean isRemoteDebugAvailable() {
    return remoteDebugAvailable;
  }

  public boolean shouldPrepareRemoteDebug(String url) {
    if (!initRemoteDebugIfNecessary()) {
      return false;
    }
    ILynxDevToolService devToolService = getDevToolService();
    if (devToolService != null) {
      return devToolService.globalDebugBridgeShouldPrepareRemoteDebug(url);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
    return false;
  }

  public boolean prepareRemoteDebug(String scheme) {
    if (!initRemoteDebugIfNecessary()) {
      return false;
    }

    if (!LynxEnv.inst().isLynxDebugEnabled()) {
      LLog.w(TAG, "Debugging not supported in this package");
      return false;
    }

    if (!LynxEnv.inst().isDevtoolEnabled()) {
      LLog.w(TAG, "DevTool not enabled, turn on the switch!");
      return false;
    }

    ILynxDevToolService devToolService = getDevToolService();
    if (devToolService != null) {
      return devToolService.globalDebugBridgePrepareRemoteDebug(scheme);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
    return false;
  }

  public void registerCardListener(LynxDevtoolCardListener listener) {
    if (!initRemoteDebugIfNecessary()) {
      return;
    }
    ILynxDevToolService devToolService = getDevToolService();
    if (devToolService != null) {
      devToolService.globalDebugBridgeRegisterCardListener(listener);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public void onPerfMetricsEvent(String eventName, @NonNull JSONObject data, int instanceId) {
    if (!remoteDebugAvailable) {
      return;
    }
    ILynxDevToolService devToolService = getDevToolService();
    if (devToolService != null) {
      devToolService.globalDebugBridgeOnPerfMetricsEvent(eventName, data, instanceId);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }
}
