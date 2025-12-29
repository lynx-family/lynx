// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool;

import android.content.Context;
import android.util.Log;
import android.view.ViewGroup;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.debugrouter.ConnectionState;
import com.lynx.debugrouter.ConnectionType;
import com.lynx.debugrouter.DebugRouter;
import com.lynx.debugrouter.DebugRouterGlobalHandler;
import com.lynx.debugrouter.StateListener;
import com.lynx.devtoolwrapper.DevToolLifecycle;
import com.lynx.devtoolwrapper.LynxDevtoolCardListener;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.eventreport.ILynxEventReportObserver;
import com.lynx.tasm.eventreport.LynxEventReporter;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.json.JSONException;
import org.json.JSONObject;

@Keep
public class LynxGlobalDebugBridge
    implements DebugRouterGlobalHandler, StateListener, ILynxEventReportObserver {
  private static final String TAG = "LynxGlobalDebugBridge";
  private Boolean mIsPerfMetricsEnabled = null;

  // protocol
  private static final String CUSTOM_FOR_SET_GLOBAL_SWITCH = "SetGlobalSwitch";
  private static final String CUSTOM_FOR_GET_GLOBAL_SWITCH = "GetGlobalSwitch";

  private DevToolAgentDispatcher mAgentDispatcher;

  private Set<LynxDevtoolCardListener> mCardListeners = new HashSet<>();

  // Singleton
  public static LynxGlobalDebugBridge getInstance() {
    return SingletonHolder.INSTANCE;
  }

  private static class SingletonHolder {
    private static final LynxGlobalDebugBridge INSTANCE = new LynxGlobalDebugBridge();
  }

  private static class DevToolAgentDispatcher extends LynxInspectorOwner {
    public DevToolAgentDispatcher() {
      super(null, true); // Global debug bridge always operates in debuggable mode
    }
  }

  private LynxGlobalDebugBridge() {
    mAgentDispatcher = new DevToolAgentDispatcher();
    DebugRouter.getInstance().addGlobalHandler(this);
    DebugRouter.getInstance().addStateListener(this);
    LynxEventReporter.addObserver(this);
    if (LynxEnv.inst().isLaunchRecordEnabled()) {
      startRecord();
    }
  }

  public boolean shouldPrepareRemoteDebug(String url) {
    return DebugRouter.getInstance().isValidSchema(url);
  }

  public boolean prepareRemoteDebug(String scheme) {
    return DebugRouter.getInstance().handleSchema(scheme);
  }

  public void setAppInfo(Context context, Map<String, String> appInfo) {
    if (appInfo == null) {
      return;
    }
    appInfo.put("sdkVersion", LynxEnv.inst().getLynxVersion());
    DebugRouter.getInstance().setAppInfo(context, appInfo);
  }

  public boolean isEnabled() {
    return DebugRouter.getInstance().getConnectionState() == ConnectionState.CONNECTED;
  }

  public void registerCardListener(LynxDevtoolCardListener listener) {
    if (listener != null) {
      mCardListeners.add(listener);
    }
  }

  @Override
  public void openCard(String url) {
    for (LynxDevtoolCardListener listener : mCardListeners) {
      LLog.i(TAG, "openCard: " + url + ", handled by " + listener.getClass().getName());
      listener.open(url);
    }
  }

  @Override
  public void onMessage(String type, int sessionId, String message) {
    if (type == null || message == null) {
      return;
    }
    if (type.equals(CUSTOM_FOR_SET_GLOBAL_SWITCH)) {
      Object result = mAgentDispatcher.setGlobalSwitch(message);
      DebugRouter.getInstance().sendDataAsync(
          CUSTOM_FOR_SET_GLOBAL_SWITCH, -1, String.valueOf(result));
    } else if (type.equals(CUSTOM_FOR_GET_GLOBAL_SWITCH)) {
      Object result = mAgentDispatcher.getGlobalSwitch(message);
      DebugRouter.getInstance().sendDataAsync(
          CUSTOM_FOR_GET_GLOBAL_SWITCH, -1, String.valueOf(result));
    }
  }

  public void startRecord() {
    RecorderController.nativeStartRecord();
  }

  @Override
  public void onClose(int code, String reason) {
    LLog.i(TAG, "Connection closed. Code: " + code + ". Reason: " + reason);
    enableTraceMode(false);
    // FIXME(mitchilling): Theoretically, we should notify the DevToolLifecycle
    // to update the state to DISCONNECTED.
    // However, at the moment onClose() and onOpen() are called multiple times back and forth.
    // I need to look into debug router to find out the root cause.
    // For now, let's not update the state yet.
  }

  private void enableTraceMode(boolean enable) {
    LynxDevtoolEnv.inst().setDevtoolEnvMask(LynxEnvKey.SP_KEY_ENABLE_DOM_TREE, !enable);
    LynxDevtoolEnv.inst().setDevtoolEnvMask(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG, !enable);
    LynxDevtoolEnv.inst().setDevtoolEnvMask(LynxEnvKey.SP_KEY_ENABLE_V8, !enable);
    LynxDevtoolEnv.inst().setDevtoolEnvMask(LynxEnvKey.SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT, !enable);
    LynxDevtoolEnv.inst().setDevtoolEnvMask(LynxEnvKey.SP_KEY_ENABLE_HIGHLIGHT_TOUCH, !enable);
  }

  @Override
  public void onError(String error) {
    LLog.i(TAG, "Connection closed. Error: " + error);
    enableTraceMode(false);
  }

  @Override
  public void onOpen(ConnectionType type) {
    LLog.i(TAG, "Connection opened. Type: " + type);
    DevToolLifecycle.getInstance().onConnected();
  }

  @Override
  public void onMessage(String text) {}

  @Override
  public void onReportEvent(@NonNull String eventName, int instanceId,
      @NonNull Map<String, ?> props, @Nullable Map<String, ?> extraData) {
    if (mIsPerfMetricsEnabled == null) {
      mIsPerfMetricsEnabled = LynxDevtoolEnv.inst().isPerfMetricsEnabled();
    }
    if (!mIsPerfMetricsEnabled) {
      return;
    }
    onPerfMetricsEvent(eventName, new JSONObject(props), instanceId);
  }

  public void onPerfMetricsEvent(String eventName, @NonNull JSONObject data, int instanceId) {
    if (mIsPerfMetricsEnabled == null) {
      mIsPerfMetricsEnabled = LynxDevtoolEnv.inst().isPerfMetricsEnabled();
    }
    if (mIsPerfMetricsEnabled && mAgentDispatcher != null) {
      try {
        data.put("instanceId", instanceId);
        mAgentDispatcher.onPerfMetricsEvent(eventName, data);
      } catch (JSONException e) {
        LLog.e(TAG, e.toString());
      }
    }
  }
}
