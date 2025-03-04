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

  private static ILynxDevToolService sDevToolService = null;

  // Remote debug stuff
  private boolean remoteDebugAvailable = false;
  // Singleton
  public static LynxDevtoolGlobalHelper getInstance() {
    return SingletonHolder.INSTANCE;
  }

  private Context mContext;
  private Map<String, String> mAppInfo;

  private static class SingletonHolder {
    private static final LynxDevtoolGlobalHelper INSTANCE = new LynxDevtoolGlobalHelper();
  }

  private LynxDevtoolGlobalHelper() {
    mAppInfo = new HashMap<>();
    mAppInfo.put("sdkVersion", LynxEnv.inst().getLynxVersion());
    initRemoteDebugIfNecessary();
    sDevToolService = LynxServiceCenter.inst().getService(ILynxDevToolService.class);
  }

  private boolean initRemoteDebugIfNecessary() {
    if (!LynxEnv.inst().isNativeLibraryLoaded()) {
      if (mContext != null) {
        Toast.makeText(mContext, "Lynx initialization not finished!", Toast.LENGTH_SHORT).show();
      }
      LLog.w(TAG, "liblynx.so not loaded!");
      return false;
    }

    if (remoteDebugAvailable) {
      return true;
    }

    if (LynxEnv.inst().isLaunchRecordEnabled()) {
      if (sDevToolService != null) {
        sDevToolService.globalDebugBridgeStartRecord();
      } else {
        LLog.e(TAG, "failed to get DevToolService");
        return remoteDebugAvailable;
      }
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

    if (sDevToolService != null) {
      sDevToolService.globalDebugBridgeSetAppInfo(context, mAppInfo);
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
    if (sDevToolService != null) {
      return sDevToolService.globalDebugBridgeShouldPrepareRemoteDebug(url);
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
      if (mContext != null) {
        Toast.makeText(mContext, "Debugging not supported in this package", Toast.LENGTH_SHORT)
            .show();
      }
      LLog.w(TAG, "Debugging not supported in this package");
      return false;
    }

    if (!LynxEnv.inst().isDevtoolEnabled() && !LynxEnv.inst().isDevtoolEnabledForDebuggableView()) {
      if (mContext != null) {
        Toast.makeText(mContext, "DevTool not enabled, turn on the switch!", Toast.LENGTH_SHORT)
            .show();
      }
      LLog.w(TAG, "DevTool not enabled, turn on the switch!");
      return false;
    }

    setAppInfo(mContext, null);

    if (sDevToolService != null) {
      return sDevToolService.globalDebugBridgePrepareRemoteDebug(scheme);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
    return false;
  }

  public void setContext(Context context) {
    mContext = context;
    if (!initRemoteDebugIfNecessary()) {
      return;
    }
    if (sDevToolService != null) {
      sDevToolService.globalDebugBridgeSetContext(context);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }

  @Deprecated
  public void showDebugView(ViewGroup root) {}

  public void registerCardListener(LynxDevtoolCardListener listener) {
    if (!initRemoteDebugIfNecessary()) {
      return;
    }
    if (sDevToolService != null) {
      sDevToolService.globalDebugBridgeRegisterCardListener(listener);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }

  public void onPerfMetricsEvent(String eventName, @NonNull JSONObject data, int instanceId) {
    if (!remoteDebugAvailable) {
      return;
    }
    if (sDevToolService != null) {
      sDevToolService.globalDebugBridgeOnPerfMetricsEvent(eventName, data, instanceId);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }
}
