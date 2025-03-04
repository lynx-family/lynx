// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.debugrouter.DebugRouter;
import com.lynx.devtool.helper.TestbenchDumpFileHelper;
import com.lynx.devtool.helper.UITreeHelper;
import com.lynx.devtool.module.DevtoolWebSocketModule;
import com.lynx.devtool.network.DownloadCallback;
import com.lynx.devtool.network.LynxDevToolDownloader;
import com.lynx.devtool.tracing.FrameViewTrace;
import com.lynx.devtool.utils.ErrorUtils;
import com.lynx.devtoolwrapper.CDPResultCallback;
import com.lynx.devtoolwrapper.CustomizedMessage;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwnerNG;
import com.lynx.devtoolwrapper.MessageHandler;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.PageReloadHelper;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceResponse;
import java.io.File;
import java.lang.ref.WeakReference;
import java.util.Map;
import org.json.JSONException;
import org.json.JSONObject;

@Keep
public class LynxInspectorOwner implements LynxBaseInspectorOwnerNG {
  private static final String TAG = "LynxInspectorOwner";
  private static final String DEBUG_ACTIVE_MES = "Debugger.setDebugActive";
  private static final String GLOBAL_KEY = "global_key";
  private static final String GLOBAL_VALUE = "global_value";

  private WeakReference<LynxView> mLynxView;

  private boolean mIsDebugging;
  private boolean mNavigatePending;

  // LynxDevToolNGDelegate
  private LynxDevToolNGDelegate mLynxDevToolNG;

  // Dev Menu
  private LynxDevMenu mDevMenu;

  // PageReload
  private PageReloadHelper mReloadHelper;

  // UI Tree
  private UITreeHelper mUITreeHelper;

  //  DevToolPlatformAndroidDelegate for new architecture
  private DevToolPlatformAndroidDelegate mPlatform;

  private long mRecordID;

  private IDevToolDelegate mDevToolDelegate = null;

  private GlobalPropsObserver globalPropsObserver = null;
  private TemplateData cachedGlobalProps = null;

  public LynxInspectorOwner() {
    init();
  }

  public LynxInspectorOwner(LynxView lynxView) {
    init();

    // Base Data
    mLynxView = new WeakReference<>(lynxView);

    mRecordID = 0;

    mIsDebugging = false;

    mNavigatePending = false;

    // DevMenu
    mDevMenu = new LynxDevMenu(this, lynxView);

    // PageReload
    mReloadHelper = null;

    // UI Tree
    mUITreeHelper = new UITreeHelper();

    if (TraceEvent.enableTrace()) {
      FrameViewTrace.getInstance().attachView(lynxView);
    }
  }

  @Override
  public void attach(LynxView lynxView) {
    mLynxView = new WeakReference<>(lynxView);
    if (mDevMenu != null) {
      mDevMenu.attach(lynxView);
    }
    if (mPlatform != null) {
      mPlatform.attach(lynxView);
    }
    if (TraceEvent.enableTrace()) {
      FrameViewTrace.getInstance().attachView(lynxView);
    }
  }

  public void init() {
    if (!LynxEnv.inst().isDevLibraryLoaded()) {
      LynxDevtoolEnv.inst().loadNativeDevtoolLibrary();
    }
    mLynxDevToolNG = new LynxDevToolNGDelegate();
  }

  public void attachLynxUIOwnerToAgent(LynxUIOwner uiOwner) {
    if (mUITreeHelper != null) {
      mUITreeHelper.attachLynxUIOwner(uiOwner);
    }
    LynxView lynxView = mLynxView.get();
    if (lynxView != null) {
      mPlatform = new DevToolPlatformAndroidDelegate(lynxView, uiOwner);
    }
  }

  @Override
  public void setReloadHelper(PageReloadHelper reloadHelper) {
    mReloadHelper = reloadHelper;
    if (mPlatform != null) {
      mPlatform.setReloadHelper(reloadHelper);
    }
  }

  @Override
  public void invokeCDPFromSDK(String cdpMsg, CDPResultCallback callback) {
    mLynxDevToolNG.invokeCDPFromSDK(cdpMsg, callback);
  }

  @Override
  public void onRegisterModule(LynxModuleFactory moduleFactory) {
    moduleFactory.registerModule(DevtoolWebSocketModule.NAME, DevtoolWebSocketModule.class, null);
  }

  public boolean isDebugging() {
    return mIsDebugging;
  }

  public void attachToDebugBridge(String url) {
    LLog.i(TAG, "attachToDebugBridge:" + url);
    if (mLynxDevToolNG != null && (!mLynxDevToolNG.isAttachToDebugRouter())) {
      int sessionId = mLynxDevToolNG.attachToDebug(url == null ? "" : url);
      LynxView lynxView = getLynxView();
      if (sessionId > 0 && lynxView != null) {
        DebugRouter.getInstance().setSessionIdOfView(lynxView, sessionId);
      }
      initRecord();
    }
  }

  private void initRecord() {
    LynxView lynxView = getLynxView();
    if (lynxView == null || lynxView.getLynxContext() == null) {
      return;
    }
    File dir = lynxView.getLynxContext().getExternalFilesDir(null);
    if (null == dir) {
      dir = lynxView.getLynxContext().getFilesDir();
    }
    if (dir == null) {
      return;
    }

    Resources resources = lynxView.getContext().getResources();
    DisplayMetrics dm = resources.getDisplayMetrics();
    float screenWidth = dm.widthPixels;
    float screenHeight = dm.heightPixels;
    nativeInitRecorderConfig(
        dir.getPath(), mLynxDevToolNG.getSessionId(), screenWidth, screenHeight, mRecordID);
  }

  public void sendConsoleMessage(String text, int level, long timestamp) {
    if (mPlatform != null) {
      mPlatform.sendConsoleEvent(text, level, timestamp);
    }
  }

  @Override
  public void savePostURL(@NonNull final String postUrl) {
    // deprecated
  }

  @CalledByNative
  public void reload(boolean ignoreCache) {
    reload(ignoreCache, null, false, 0);
  }

  public void reload(
      boolean ignoreCache, String templateBin, boolean fromTemplateFragments, int templateSize) {
    if (mReloadHelper != null) {
      LynxView lynxView = getLynxView();
      if (lynxView != null) {
        Toast.makeText(lynxView.getContext(), "Start to download & reload...", Toast.LENGTH_SHORT)
            .show();
      }
      mReloadHelper.reload(ignoreCache, templateBin, fromTemplateFragments, templateSize);
    }
  }

  public void onReceiveTemplateFragment(String data, boolean eof) {
    if (mReloadHelper != null) {
      mReloadHelper.onReceiveTemplateFragment(data, eof);
    }
  }

  public void navigate(String url) {
    if (mReloadHelper != null) {
      mNavigatePending = true;
      mReloadHelper.navigate(url);
    }
  }

  @Override
  @Deprecated
  public String getGroupID() {
    return LynxGroup.SINGNLE_GROUP;
  }

  public void destroy() {
    try {
      stopCasting();
      if (mLynxDevToolNG != null) {
        mLynxDevToolNG.detachToDebug();
      }
      if (mLynxDevToolNG != null) {
        mLynxDevToolNG.destroy();
      }
      if (mPlatform != null) {
        mPlatform.destroy();
      }
    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  @Override
  public void onTemplateAssemblerCreated(long ptr) {
    if (mLynxDevToolNG != null) {
      mLynxDevToolNG.onTASMCreated(ptr);
      if (mPlatform != null) {
        mLynxDevToolNG.setDevToolPlatformAbility(mPlatform.getNativePtr());
        mPlatform.setDevToolDelegate(mDevToolDelegate);
      }
    }
    mRecordID = ptr;
  }

  @Override
  public long onBackgroundRuntimeCreated(String groupName) {
    if (mLynxDevToolNG != null) {
      return mLynxDevToolNG.onBackgroundRuntimeCreated(groupName);
    }
    return 0;
  }

  public static boolean isApkDebuggable(Context context) {
    try {
      ApplicationInfo info = context.getApplicationInfo();
      return (info.flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
    } catch (Throwable e) {
      e.printStackTrace();
    }
    return false;
  }

  @Override
  public void stopCasting() {
    try {
      mPlatform.stopCasting();
    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  @Override
  public void continueCasting() {
    try {
      if (mLynxDevToolNG != null && mLynxDevToolNG.isAttachToDebugRouter()) {
        mPlatform.continueCasting();
      }
    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  @Override
  public void pauseCasting() {
    try {
      if (mLynxDevToolNG != null && mLynxDevToolNG.isAttachToDebugRouter()) {
        mPlatform.pauseCasting();
      }
    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  public LynxView getLynxView() {
    return mLynxView.get();
  }

  @Deprecated
  public void sendResponse(String response) {
    Log.v(TAG, response);
    if (mLynxDevToolNG != null) {
      mLynxDevToolNG.sendMessageToDebugPlatform("CDP", response);
    }
  }

  public int getSessionID() {
    return mLynxDevToolNG != null ? mLynxDevToolNG.getSessionId() : 0;
  }

  public void onRootViewInputEvent(InputEvent event) {
    if (!LynxEnv.inst().isDevtoolEnabled()) {
      return;
    }
    if (event instanceof MotionEvent && mDevMenu != null) {
      MotionEvent motionEvent = (MotionEvent) event;
      if (LynxDevtoolEnv.inst().isLongPressMenuEnabled()) {
        mDevMenu.onRootViewTouchEvent(motionEvent);
      }
      String[] values = new String[4];
      values[0] = String.valueOf(motionEvent.getAction());
      values[1] = String.valueOf(motionEvent.getX());
      values[2] = String.valueOf(motionEvent.getY());
      values[3] = String.valueOf(motionEvent.getMetaState());
      nativeRecordEventAndroid(0, values, mRecordID);
    } else if (event instanceof KeyEvent) {
      KeyEvent keyEvent = (KeyEvent) event;
      String[] values = new String[3];
      values[0] = String.valueOf(keyEvent.getAction());
      values[1] = String.valueOf(keyEvent.getKeyCode());
      values[2] = String.valueOf(keyEvent.getMetaState());
      nativeRecordEventAndroid(1, values, mRecordID);
    }
  }

  @Deprecated
  public boolean isHttpServerWorking() {
    return false;
  }

  @Deprecated
  public String getHttpServerIp() {
    return "Not Supported";
  }

  @Deprecated
  public String getHttpServerPort() {
    return "";
  }

  public View getTemplateView() {
    return mLynxView.get();
  }

  @CalledByNative
  public String getTemplateUrl() {
    if (mPlatform != null) {
      return mPlatform.getTemplateUrl();
    }
    return "";
  }

  public long getTemplateDataPtr() {
    if (mReloadHelper != null) {
      return mReloadHelper.getTemplateDataPtr();
    }
    return 0;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public String getTemplateJsInfo(int offset, int size) {
    if (mReloadHelper != null) {
      return mReloadHelper.getTemplateJsInfo(offset, size);
    }
    return "";
  }

  @Override
  public void sendMessage(CustomizedMessage message) {
    if (mLynxDevToolNG != null && message != null) {
      mLynxDevToolNG.sendMessageToDebugPlatform(message.getType(), message.getData());
    }
  }

  @Override
  public void subscribeMessage(String type, MessageHandler handler) {
    if (mLynxDevToolNG != null) {
      mLynxDevToolNG.subscribeMessage(type, handler);
    }
  }

  @Override
  public void unsubscribeMessage(String type) {
    if (mLynxDevToolNG != null) {
      mLynxDevToolNG.unSubscribeMessage(type);
    }
  }

  @Override
  public void updateScreenMetrics(int width, int height, float density) {
    if (mLynxDevToolNG != null) {
      mLynxDevToolNG.updateScreenMetrics(width, height, density);
    }
  }

  public void setStopAtEntry(boolean stop) {
    nativeSetStopAtEntry(stop, false);
  }

  public void setStopLepusAtEntry(boolean stop) {
    nativeSetStopAtEntry(stop, true);
  }

  public Object setGlobalSwitch(String message) {
    try {
      JSONObject messageObj = new JSONObject(message);
      LynxDevtoolEnv.inst().setDevtoolEnv(
          messageObj.getString(GLOBAL_KEY), messageObj.get(GLOBAL_VALUE));
      return messageObj.get(GLOBAL_VALUE);
    } catch (JSONException e) {
      LLog.e(TAG, e.toString());
    }
    return null;
  }

  public Object getGlobalSwitch(String message) {
    try {
      JSONObject messageObj = new JSONObject(message);
      String key = messageObj.getString(GLOBAL_KEY);
      return LynxDevtoolEnv.inst().getDevtoolObjectEnv(
          key, LynxDevtoolEnv.inst().getDefaultValue(key));
    } catch (JSONException e) {
      LLog.e(TAG, e.toString());
    }
    return null;
  }

  public void addUITreeToTestbench() {
    LynxView lynxView = getLynxView();
    if (lynxView != null && lynxView.getLynxUIRoot() != null) {
      sendFileByAgent("UITree", TestbenchDumpFileHelper.getUITree(lynxView.getLynxUIRoot()));
    }
  }

  public void addViewTreeToTestbench() {
    LynxView lynxView = getLynxView();
    if (lynxView != null && lynxView.getRootView() != null) {
      sendFileByAgent("ViewTree", TestbenchDumpFileHelper.getViewTree(lynxView.getRootView()));
    }
  }

  // android send file api
  public void sendFileByAgent(String type, String file) {
    nativeSendFileByAgent(type, file);
  }

  public void endTestbench(String filePath) {
    LLog.d(TAG, "end testbench replay test");
    addUITreeToTestbench();
    addViewTreeToTestbench();

    nativeEndTestbench(filePath);
  }

  @Override
  public void onPageUpdate() {
    if (mPlatform != null) {
      mPlatform.sendLayerTreeDidChangeEvent();
    }
  }

  @Override
  public void downloadResource(final String url, final LynxResourceCallback<byte[]> callback) {
    new LynxDevToolDownloader(url, new DownloadCallback() {
      @Override
      public void onResponse(int status, int contentLength) {}

      @Override
      public void onData(byte[] bytes, int length) {
        callback.onResponse(LynxResourceResponse.success(bytes));
      }

      @Override
      public void onFailure(String reason) {
        callback.onResponse(LynxResourceResponse.failed(LynxResourceResponse.FAILED,
            new Exception("download fail with url: " + url + ", the reason is: " + reason)));
      }
    });
  }

  @Override
  public void setLynxInspectorConsoleDelegate(Object delegate) {
    if (mPlatform != null) {
      mPlatform.setLynxInspectorConsoleDelegate(delegate);
    }
  }

  @Override
  public void getConsoleObject(String objectId, boolean needStringify, Callback callback) {
    if (mPlatform != null) {
      mPlatform.getConsoleObject(objectId, needStringify, callback);
    }
  }

  public void onPerfMetricsEvent(String eventName, @NonNull JSONObject data) {
    if (!LynxDevtoolEnv.inst().isPerfMetricsEnabled()) {
      return;
    }
    try {
      JSONObject eventInfo = new JSONObject();
      eventInfo.put("method", eventName);
      eventInfo.put("params", data);
      sendMessage(new CustomizedMessage("CDP", eventInfo.toString()));
    } catch (JSONException e) {
      LLog.e(TAG, e.toString());
    }
  }

  public String getDebugInfoUrl() {
    if (mPlatform != null) {
      return mPlatform.getLepusDebugInfoUrl();
    }
    return "";
  }

  public void onReceiveMessageEvent(ReadableMap event) {
    if (mLynxDevToolNG == null || (!mLynxDevToolNG.isAttachToDebugRouter())) {
      return;
    }
    if (event == null) {
      return;
    }
    // 'eventName' corresponds to field 'type' from engine's event
    String eventName = event.getString("type");
    if (TextUtils.isEmpty(eventName)) {
      return;
    }
    try {
      JSONObject object = new JSONObject();
      object.putOpt("event", eventName);
      object.putOpt("vmType", event.getString("origin", ""));
      object.putOpt("data", event.getString("data", ""));
      handleCDPEvent("Lynx.onVMEvent", object);
    } catch (JSONException e) {
      LLog.e(TAG, e.toString());
    }
  }

  @Override

  public void setDevToolDelegate(IDevToolDelegate devToolDelegate) {
    mDevToolDelegate = devToolDelegate;
    if (mPlatform != null) {
      mPlatform.setDevToolDelegate(devToolDelegate);
    }
  }

  public void dispatchMessageEvent(ReadableMap event) {
    if (mDevToolDelegate != null) {
      mDevToolDelegate.onDispatchMessageEvent(event);
    }
  }

  private void handleCDPEvent(String event, JSONObject params) throws JSONException {
    if (mLynxDevToolNG == null || (!mLynxDevToolNG.isAttachToDebugRouter())) {
      return;
    }
    if (TextUtils.isEmpty(event)) {
      return;
    }
    JSONObject msg = new JSONObject();
    msg.put("method", event);
    if (params != null && params.length() > 0) {
      msg.put("params", params);
    }
    mLynxDevToolNG.sendMessageToDebugPlatform("CDP", msg.toString());
  }

  void getViewLocationOnScreen(int[] outLocation) {
    if (outLocation == null || outLocation.length < 2 || mLynxView == null) {
      return;
    }
    LynxView view = mLynxView.get();
    if (view != null && view.getLynxContext() != null) {
      view.getLocationOnScreen(outLocation);
      float scale = view.getLynxContext().getResources().getDisplayMetrics().density;
      outLocation[0] /= scale;
      outLocation[1] /= scale;
    }
  }

  public void registerGlobalPropsUpdatedObserver(GlobalPropsObserver observer) {
    this.globalPropsObserver = observer;
    if (cachedGlobalProps != null) {
      // if cachedGlobalProps existed. trigger observer immediately.
      triggerGlobalPropsObserver();
    }
  }

  @Override
  public void onGlobalPropsUpdated(TemplateData props) {
    cachedGlobalProps = props;
    triggerGlobalPropsObserver();
  }

  private void triggerGlobalPropsObserver() {
    if (this.globalPropsObserver != null) {
      Map globalProps = null;
      if (cachedGlobalProps != null) {
        globalProps = cachedGlobalProps.toMap();
      }
      this.globalPropsObserver.onGlobalPropsUpdated(globalProps);
    }
  }

  @Override
  public void showErrorMessageOnConsole(final LynxError error) {
    if (error == null || !error.isValid() || error.isJSError() || error.isLepusError()) {
      return;
    }
    String errMsg = ErrorUtils.getKeyMessage(error);
    String consoleLog = "Native error:\n" + errMsg;
    int level = ErrorUtils.errorLevelStrToInt(error.getLevel());
    sendConsoleMessage(consoleLog, level, System.currentTimeMillis());
  }

  @Override
  public void showMessageOnConsole(final String message, int level) {
    if (message == null) {
      return;
    }
    sendConsoleMessage(message, level, System.currentTimeMillis());
  }

  public interface GlobalPropsObserver {
    void onGlobalPropsUpdated(Map globalProps);
  }

  private native void nativeSetStopAtEntry(boolean stop, boolean isLepus);
  private native void nativeInitRecorderConfig(
      String filePath, int sessionID, float screenWidth, float screenHeight, long recordID);
  private native void nativeSendFileByAgent(String type, String file);
  private native void nativeEndTestbench(String filePath);
  // eventType: 0 stands for touch event (or MotionEvent), and 1 for key event.
  private native void nativeRecordEventAndroid(int eventType, String[] values, long RecordID);
}
