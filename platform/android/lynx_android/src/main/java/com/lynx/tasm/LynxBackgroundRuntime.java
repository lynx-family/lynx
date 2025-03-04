// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import android.text.TextUtils;
import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.jsbridge.JSModule;
import com.lynx.jsbridge.LynxFetchModule;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.CleanupReference;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.core.JSProxy;
import com.lynx.tasm.core.LynxResourceLoader;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceProvider;
import java.lang.ref.WeakReference;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

// Lynx Living Specification:
//  Background Runtime
//  A scripting runtime environment comprises of
//    An instance of JavaScript engine, and
//    Extended scripting APIs, e.g. JSB (JavaScript Bridge) --- to be further defined.
//  Background Script
//    A kind of script that is intended to be executed under the background runtime.
// LynxBackgroundRuntime is an instance of `Background Runtime`, which allows:
//  1. user creates it asynchronously and pre-run some `Background Script`
//  2. user use it to build a LynxView
public class LynxBackgroundRuntime implements ILynxErrorReceiver {
  private static final String TAG = "LynxBackgroundRuntime";
  private LynxBackgroundRuntimeOptions mOptions;
  private LynxModuleFactory mModuleFactory;
  private LynxResourceLoader mResourceLoader;
  private JSProxy mJSProxy;
  private CopyOnWriteArrayList<LynxBackgroundRuntimeClient> mRuntimeClients;
  private Map<Double, PlatformCallBack> mPlatformCallBackMap;
  private CleanupReference mCleanupReference = null;
  private LynxDevtool mDevTool;
  private long mNativePtr = 0;
  private long mInspectorObserverPtr = 0;
  private volatile String mLastScriptUrl = "";

  // LynxBackgroundRuntime's initial state, under this state, all
  // APIs on it are functional. If user manually calls `destroy`
  // it transits into `STATE_DESTROYED`. If user uses it to build
  // a LynxView, it transits into `STATE_ATTACHED`.
  public static final int STATE_START = 0;
  // LynxBackgroundRuntime is destroyed, under this state, all
  // APIs on it are frozen and will be ignored.
  public static final int STATE_DESTROYED = 1 << 1;
  // LynxBackgroundRuntime is attached to a LynxView, under this state, all
  // APIs on it are frozen and will be ignored. If user wants to send
  // GlobalEvent, API on LynxView should be used.
  public static final int STATE_ATTACHED = 1 << 2;
  // Only if LynxEnv is not initialized, LynxBackgroundRuntime is invalid
  public static final int STATE_INVALID = 1 << 3;

  // mState is protected by mStateLock
  private int mState;
  // TODO(huzhanbo.luc): redesign this lock
  private final Object mStateLock = new Object();

  /**
   * Create a LynxBackgroundRuntime
   * @param context any Android context: eg. ApplicationContext/ActivityContext
   * @param options configuration for the runtime
   */
  @AnyThread
  public LynxBackgroundRuntime(
      @NonNull Context context, @NonNull LynxBackgroundRuntimeOptions options) {
    mRuntimeClients = new CopyOnWriteArrayList<>();
    mPlatformCallBackMap = new HashMap<>();
    if (!LynxEnv.inst().isNativeLibraryLoaded()) {
      LLog.e(TAG, "LynxBackgroundRuntime constructor called before LynxEnv init");
      mState = STATE_INVALID;
      return;
    }
    mState = STATE_START;
    mOptions = options;
    mModuleFactory = new LynxModuleFactory(context);
    mModuleFactory.registerModule(LynxFetchModule.NAME, LynxFetchModule.class, null);
    mModuleFactory.addModuleParamWrapper(options.getWrappers());

    if (LynxEnv.inst().isLynxDebugEnabled()) {
      initDevtool(context);
    }

    LynxGroup group = options.getLynxGroup();
    // TODO(huzhanbo.luc): merge this logic into LynxBackgroundOptions
    String groupId = group != null ? group.getID() : LynxGroup.SINGNLE_GROUP;
    String[] preloadJSPaths = group != null ? group.getPreloadJSPaths() : null;
    boolean enableJSGroupThread = group != null && group.enableJSGroupThread();
    String groupName = enableJSGroupThread ? groupId : "";

    mResourceLoader = new LynxResourceLoader(
        options, null, this, options.templateResourceFetcher, options.genericResourceFetcher);

    long whiteBoard = (group == null) ? 0 : group.getWhiteBoardPtr();

    // The natvie runtime wrapper should be read-only after this creation, and has the
    // same life-span as LynxBackgroundRuntime Java Object. `Destroy` will not modify
    // this wrapper, its release will be handled by mCleanupReference on UI Thread when
    // LynxBackgroundRuntime has no reference on it.
    mNativePtr = nativeCreateBackgroundRuntimeWrapper(mResourceLoader, mModuleFactory,
        mInspectorObserverPtr, whiteBoard, groupId, groupName, preloadJSPaths, false, false,
        options.useQuickJSEngine(), false, options.isEnableUserBytecode(),
        options.getBytecodeSourceUrl(), enableJSGroupThread);
    mInspectorObserverPtr = 0;

    TemplateData presetData = options.getPresetData();
    if (presetData != null) {
      nativeSetPresetData(mNativePtr, presetData.isReadOnly(), presetData.getNativePtr());
    }

    mJSProxy = new JSProxy(this, groupName);

    mCleanupReference = new CleanupReference(
        this, new LynxBackgroundRuntime.CleanupOnUiThread(mNativePtr, mJSProxy), true);
  }

  /**
   * Add LynxBackgroundRuntimeClient, client that listens to runtime's callback.
   * Call before other API to ensure callback won't be missed
   * @param client: client to be added
   */
  @AnyThread
  public void addLynxBackgroundRuntimeClient(LynxBackgroundRuntimeClient client) {
    if (!mRuntimeClients.contains(client)) {
      mRuntimeClients.add(client);
    }
  }

  /**
   * remove LynxBackgroundRuntimeClient
   * @param client: client to be removed
   */
  @AnyThread
  public void removeLynxBackgroundRuntimeClient(LynxBackgroundRuntimeClient client) {
    if (mRuntimeClients.contains(client)) {
      mRuntimeClients.remove(client);
    }
  }

  /**
   * Execute a Background Script, valid until LynxBackgroundRuntime is destroyed or attached.
   * @param url unique identifier for each script, will be used to track the script and related
   *     reports
   * @param sources content of the script
   */
  @AnyThread
  public void evaluateJavaScript(String url, @NonNull String sources) {
    synchronized (mStateLock) {
      if (mState == STATE_START && mDevTool != null) {
        mDevTool.attachToDebugBridge(url);
        mDevTool.onLoadFromURL(url, "", null, null, null);
      }
    }

    if (mNativePtr != 0) {
      mLastScriptUrl = url;
      nativeEvaluateScript(mNativePtr, url, sources.getBytes(Charset.forName("UTF-8")));
    }
  }

  @AnyThread
  public String getLastScriptUrl() {
    return mLastScriptUrl;
  }

  /**
   * Send GlobalEvent to Background Script, valid until LynxBackgroundRuntime is destroyed or
   * attached.
   * @param name event name
   * @param params event params
   */
  @AnyThread
  public void sendGlobalEvent(String name, JavaOnlyArray params) {
    LLog.i(TAG, "LynxContext sendGlobalEvent " + name + " with this: " + this.toString());
    JSModule eventEmitter = mJSProxy.getJSModule("GlobalEventEmitter");
    JavaOnlyArray args = new JavaOnlyArray();
    args.pushString(name);
    args.pushArray(params);
    eventEmitter.fire("emit", args);
  }

  /**
   * Manually destroy the runtime. If user uses the runtime to build a LynxView,
   * this API is no longer needed and will be ignored, the destruction will be
   * handled by LynxView.
   */
  @AnyThread
  public void destroy() {
    synchronized (mStateLock) {
      if (mState != STATE_START) {
        LLog.e(TAG, "call destroy on invalid state, will be ignored, state: " + mState);
        return;
      }

      if (mDevTool != null) {
        mDevTool.destroy();
        mDevTool = null;
      }

      nativeDestroyRuntime(mNativePtr);
      mState = STATE_DESTROYED;
    }
  }

  /**
   * Get runtime state
   */
  @AnyThread
  public int getState() {
    synchronized (mStateLock) {
      return mState;
    }
  }

  /**
   * Set Storage for Lynx From TemplateData, will always run on Lynx_JS thread
   * @param key shared-data-key
   * @param data shared-data
   */
  public void setSessionStorageItem(String key, TemplateData data) {
    if (mNativePtr != 0 && data != null && (!TextUtils.isEmpty(key))) {
      LLog.d(TAG, "setSessionStorageItem with key: " + key);
      data.flush();
      long ptr = data.getNativePtr();
      if (ptr == 0) {
        LLog.e(TAG, "setSessionStorageItem with zero data! key: " + key);
        return;
      }
      nativeSetSessionStorageItem(mNativePtr, key, data.getNativePtr(), data.isReadOnly());
    }
  }

  /**
   * Get data on session storage, will always run on Lynx_JS thread
   * @param key data key
   * @param callback callback with data, will always run on Lynx_JS thread
   */
  public void getSessionStorageItem(String key, PlatformCallBack callback) {
    if (mNativePtr != 0 && callback != null && (!TextUtils.isEmpty(key))) {
      LLog.d(TAG, "getSessionStorageItem with key: " + key);
      nativeGetSessionStorageItem(mNativePtr, key, callback);
    }
  }

  /**
   * Subscribe a listener to listen data change on session storage,
   * will always run on Lynx_JS thread, this call may block on Lynx_JS
   * @param key data key
   * @param callBack listener with data, will always run on Lynx_JS thread
   * @return listener id, used to unsubscribe
   */
  public double subscribeSessionStorage(String key, PlatformCallBack callBack) {
    if (mNativePtr != 0 && callBack != null && (!TextUtils.isEmpty(key))) {
      LLog.d(TAG, "subscribeSessionStorage with key: " + key);
      double listenerId = nativeSubscribeSessionStorage(mNativePtr, key, callBack);
      mPlatformCallBackMap.put(listenerId, callBack);
      return listenerId;
    }
    return PlatformCallBack.InvalidId;
  }

  /**
   * Unsubscribe a listener to listen data change on session storage, will always run on Lynx_JS
   * thread
   * @param key data key
   * @param id listener with id to be erased
   */
  public void unsubscribeSessionStorage(String key, double id) {
    if (mNativePtr != 0 && PlatformCallBack.InvalidId != id && (!TextUtils.isEmpty(key))) {
      nativeUnsubscribeSessionStorage(mNativePtr, key, id);
      mPlatformCallBackMap.remove(id);
    }
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  @Override
  @CalledByNative
  public void onErrorOccurred(LynxError error) {
    for (LynxBackgroundRuntimeClient client : mRuntimeClients) {
      client.onReceivedError(error);
    }
  }

  @CalledByNative
  public void onModuleMethodInvoked(String module, String method, int error_code) {
    for (LynxBackgroundRuntimeClient client : mRuntimeClients) {
      client.onModuleMethodInvoked(module, method, error_code);
    }
  }

  public boolean attachToLynxView() {
    synchronized (mStateLock) {
      if (mState != STATE_START) {
        LLog.e(TAG,
            "build LynxView using an invalid LynxBackgroundRuntime, state: " + mState
                + ", runtime: " + this);
        return false;
      }
      mState = STATE_ATTACHED;
      return true;
    }
  }

  public LynxModuleFactory getModuleFactory() {
    return mModuleFactory;
  }

  public LynxBackgroundRuntimeOptions getLynxRuntimeOptions() {
    return mOptions;
  }

  public LynxDevtool getDevtool() {
    return mDevTool;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public long getNativePtr() {
    return mNativePtr;
  }

  private void initDevtool(Context context) {
    mDevTool = new LynxDevtool(true, context);

    LynxGroup group = mOptions.getLynxGroup();
    String groupId = group != null ? group.getID() : LynxGroup.SINGNLE_GROUP;
    boolean enableJSGroupThread = group != null && group.enableJSGroupThread();
    mInspectorObserverPtr = mDevTool.onBackgroundRuntimeCreated(enableJSGroupThread ? groupId : "");
  }

  private static class CleanupOnUiThread implements Runnable {
    private long mNativePtr;
    private JSProxy mJSProxy;
    public CleanupOnUiThread(long nativePtr, JSProxy jsProxy) {
      mNativePtr = nativePtr;
      mJSProxy = jsProxy;
    }
    @Override
    public void run() {
      if (mNativePtr == 0) {
        return;
      }
      LLog.w(TAG, "destory wrapper " + mNativePtr);
      nativeDestroyWrapper(mNativePtr);
      mNativePtr = 0;
      mJSProxy.destroy();
    }
  };

  private native long nativeCreateBackgroundRuntimeWrapper(LynxResourceLoader resourceLoader,
      LynxModuleFactory moduleFactory, long inspectorObserverPtr, long whiteBoardPtr,
      String groupId, String groupName, String[] preloadJSPaths, boolean useProviderJsEnv,
      boolean forceReloadJSCore, boolean forceUseLightweightJSEngine, boolean enablePendingJsTask,
      boolean enableUserBytecode, String bytecodeSourceUrl, boolean enableJSGroupThread);

  private native void nativeSetPresetData(long nativePtr, boolean readOnly, long presetData);

  private native void nativeEvaluateScript(long nativePtr, String url, byte[] data);

  // Release C++ objects hold by actors, such as LynxRuntime, NativeFacade. This is only
  // necessary when the runtime is not attached to LynxView. This call does not allow
  // re-entry.
  private native void nativeDestroyRuntime(long nativePtr);

  // Release natvie runtime wrapper. This can only be safely called inside CleanupReference,
  // when LynxBackgroundRuntime is out of reach (no references to LynxBackgroundRuntime).
  private static native void nativeDestroyWrapper(long nativePtr);

  private native void nativeSetSessionStorageItem(
      long ptr, String key, long value, boolean readonly);

  private native void nativeGetSessionStorageItem(long ptr, String key, PlatformCallBack callback);

  private native double nativeSubscribeSessionStorage(
      long ptr, String key, PlatformCallBack callBack);

  private native void nativeUnsubscribeSessionStorage(long ptr, String key, double id);
}
