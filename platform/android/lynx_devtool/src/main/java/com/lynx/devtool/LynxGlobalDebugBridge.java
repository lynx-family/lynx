// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool;

import android.content.Context;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.debugrouter.ConnectionState;
import com.lynx.debugrouter.ConnectionType;
import com.lynx.debugrouter.DebugRouter;
import com.lynx.debugrouter.DebugRouterGlobalHandler;
import com.lynx.debugrouter.StateListener;
import com.lynx.devtool.testbench.SwitchLaunchRecord;
import com.lynx.devtoolwrapper.LynxDevtoolCardListener;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.eventreport.ILynxEventReportObserver;
import com.lynx.tasm.eventreport.LynxEventReporter;
import java.lang.ref.WeakReference;
import java.lang.reflect.Method;
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

  // app info
  public static final String APP_NAME = "Unspecified";
  public static final String APP_VERSION = "0.0";

  // protocol
  private static final String RECEIVE_STOP_JS_AT_ENTRY = "D2RStopAtEntry";
  private static final String SEND_STOP_JS_AT_ENTRY = "R2DStopAtEntry";
  private static final String RECEIVE_STOP_LEPUS_AT_ENTRY = "D2RStopLepusAtEntry";
  private static final String SEND_STOP_LEPUS_AT_ENTRY = "R2DStopLepusAtEntry";
  private static final String CUSTOM_FOR_SET_GLOBAL_SWITCH = "SetGlobalSwitch";
  private static final String CUSTOM_FOR_GET_GLOBAL_SWITCH = "GetGlobalSwitch";

  private boolean mHasContext = false;
  private Context mContext;

  // debug status view
  private WeakReference<ViewGroup> mRootView = null;

  private DevToolAgentDispatcher mAgentDispatcher;
  private SwitchLaunchRecord mSwitchLaunchRecord;

  private Set<LynxDevtoolCardListener> mCardListeners = new HashSet<>();

  // Singleton
  public static LynxGlobalDebugBridge getInstance() {
    return SingletonHolder.INSTANCE;
  }

  private static class SingletonHolder {
    private static final LynxGlobalDebugBridge INSTANCE = new LynxGlobalDebugBridge();
  }

  private static class DevToolAgentDispatcher extends LynxInspectorOwner {}

  private static boolean autoDevToolConnect(String scheme) {
    LynxEnv.inst().lazyInitIfNeeded();
    LynxEnv.inst().enableLynxDebug(true);
    LynxEnv.inst().enableDevtool(true);
    LynxEnv.inst().enablePixelCopy(true);
    // when exec e2e test, disable longpress menu
    LynxDevtoolEnv.inst().enableLongPressMenu(false);

    /*
     * Currently DebugRouter do not set lynx sdk version in Shoots-Lynx path,
     * we set it here to get lynx sdk version info in 'sessionList' data structure afterwards
     */
    Map<String, String> lynxInfo = new HashMap<>();
    lynxInfo.put("sdkVersion", LynxEnv.inst().getLynxVersion());
    DebugRouter.getInstance().setAppInfo(null, lynxInfo);

    LynxGlobalDebugBridge bridge = LynxGlobalDebugBridge.getInstance();
    if (bridge.shouldPrepareRemoteDebug(scheme)) {
      return bridge.prepareRemoteDebug(scheme);
    } else {
      Log.e(TAG, "prepareRemoteDebug failed with  " + scheme);
      return false;
    }
  }

  /**
   * API usage: Shoots-Lynx
   * @param enableV8 <code>true</code> use V8 engine, <code>false</code> use quickjs instead
   */
  private static void autoSwitchToV8(int enableV8) {
    LynxEnv.inst().lazyInitIfNeeded();
    LynxDevtoolEnv.inst().enableV8(enableV8);
  }

  /**
   * API usage: Shoots-Lynx
   * @param enableDebugMode <code>true</code> open debug mode, <code>false</code> close debug mode
   */
  private static void autoSwitchToDebugMode(boolean enableDebugMode) {
    LynxEnv.inst().lazyInitIfNeeded();
    LynxEnv.inst().enableDebugMode(enableDebugMode);
  }

  private LynxGlobalDebugBridge() {
    mAgentDispatcher = new DevToolAgentDispatcher();
    DebugRouter.getInstance().addGlobalHandler(this);
    DebugRouter.getInstance().addStateListener(this);
    mSwitchLaunchRecord = new SwitchLaunchRecord();
    LynxEventReporter.addObserver(this);
  }

  public void setContext(Context ctx) {
    if (mHasContext) {
      return;
    }
    mContext = ctx;
    mHasContext = true;
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
    if (type.equals(RECEIVE_STOP_JS_AT_ENTRY)) {
      boolean stop = message.equals("true");
      mAgentDispatcher.setStopAtEntry(stop);
      DebugRouter.getInstance().sendDataAsync(SEND_STOP_JS_AT_ENTRY, -1, message);
    } else if (type.equals(RECEIVE_STOP_LEPUS_AT_ENTRY)) {
      boolean stop = message.equals("true");
      mAgentDispatcher.setStopLepusAtEntry(stop);
      DebugRouter.getInstance().sendDataAsync(SEND_STOP_LEPUS_AT_ENTRY, -1, message);
    } else if (type.equals(CUSTOM_FOR_SET_GLOBAL_SWITCH)) {
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
    mSwitchLaunchRecord.startRecord();
  }

  @Override
  public void onClose(int code, String reason) {
    enableTraceMode(false);
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
    enableTraceMode(false);
  }

  @Override
  public void onOpen(ConnectionType type) {
    LynxDevtoolEnv.inst().setDevtoolEnv(LynxEnvKey.SP_KEY_DEVTOOL_CONNECTED, true);
  }

  @Override
  public void onMessage(String text) {}

  @Override
  public void onReportEvent(@NonNull String eventName, int instanceId,
      @NonNull Map<String, ?> props, @Nullable Map<String, ?> extraData) {
    onPerfMetricsEvent(eventName, new JSONObject(props), instanceId);
  }

  public void onPerfMetricsEvent(String eventName, @NonNull JSONObject data, int instanceId) {
    if (LynxDevtoolEnv.inst().isPerfMetricsEnabled() && mAgentDispatcher != null) {
      try {
        data.put("instanceId", instanceId);
        mAgentDispatcher.onPerfMetricsEvent(eventName, data);
      } catch (JSONException e) {
        LLog.e(TAG, e.toString());
      }
    }
  }
}
