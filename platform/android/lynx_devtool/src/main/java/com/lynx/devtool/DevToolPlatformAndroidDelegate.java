// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import android.text.TextUtils;
import android.widget.Toast;
import androidx.annotation.Keep;
import com.lynx.devtool.helper.EmulateTouchHelper;
import com.lynx.devtool.helper.LepusDebugInfoHelper;
import com.lynx.devtool.helper.ScreenCapturer;
import com.lynx.devtool.helper.ScreenCastHelper;
import com.lynx.devtool.helper.UITreeHelper;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.PageReloadHelper;
import com.lynx.tasm.behavior.LynxUIOwner;
import java.lang.ref.WeakReference;

public class DevToolPlatformAndroidDelegate {
  private static final String TAG = "DevToolPlatformAndroidDelegate";
  private long mFacadePtr;

  // UITree
  private UITreeHelper mUITreeHelper;

  // EmulateTouch
  private EmulateTouchHelper mTouchHelper;

  // PageReload
  private PageReloadHelper mReloadHelper;

  private IDevToolDelegate mDevToolDelegate = null;

  // LynxView
  private WeakReference<LynxView> mLynxView;

  // ConsoleDelegateManager
  private ConsoleDelegateManager mConsoleDelegateManager;

  // ScreenCast
  private ScreenCastHelper mCastHelper;

  private LepusDebugInfoHelper mLepusDebugInfoHelper;

  private boolean mNavigatePending;

  @Keep
  public DevToolPlatformAndroidDelegate(LynxView lynxView, LynxUIOwner uiOwner) {
    mUITreeHelper = new UITreeHelper();
    mUITreeHelper.attachLynxUIOwner(uiOwner);
    mConsoleDelegateManager = new ConsoleDelegateManager();
    mLynxView = new WeakReference<>(lynxView);
    mLepusDebugInfoHelper = new LepusDebugInfoHelper();

    mCastHelper = new ScreenCastHelper(this, lynxView);

    mFacadePtr = nativeCreateDevToolPlatformFacade();

    if (lynxView != null) {
      mTouchHelper = new EmulateTouchHelper(mLynxView);
    }

    mReloadHelper = null;

    mNavigatePending = false;
  }

  public void setReloadHelper(PageReloadHelper reloadHelper) {
    mReloadHelper = reloadHelper;
  }

  public long getNativePtr() {
    return mFacadePtr;
  }

  public void setDevToolDelegate(IDevToolDelegate devToolDelegate) {
    mDevToolDelegate = devToolDelegate;
  }

  public void setLynxInspectorConsoleDelegate(Object delegate) {
    if (mConsoleDelegateManager != null && mFacadePtr != 0) {
      mConsoleDelegateManager.setLynxInspectorConsoleDelegate(delegate, this, mFacadePtr);
    }
  }

  public void getConsoleObject(String objectId, boolean needStringify, Callback callback) {
    if (mConsoleDelegateManager != null && mFacadePtr != 0) {
      mConsoleDelegateManager.getConsoleObject(objectId, needStringify, callback, this, mFacadePtr);
    }
  }

  @CalledByNative
  public void scrollIntoViewFromUI(int nodeId) {
    if (mDevToolDelegate != null) {
      mDevToolDelegate.scrollIntoViewFromUI(nodeId);
    }
  }

  @CalledByNative
  public int findNodeIdForLocationFromUI(float x, float y, String mode) {
    if (mDevToolDelegate != null) {
      return mDevToolDelegate.getNodeForLocation(x, y, mode);
    }
    return 0;
  }

  @CalledByNative
  public void onConsoleMessage(String msg) {
    if (mConsoleDelegateManager != null) {
      mConsoleDelegateManager.onConsoleMessage(msg);
    }
  }

  @CalledByNative
  public void onConsoleObject(String detail, int callbackId) {
    if (mConsoleDelegateManager != null) {
      mConsoleDelegateManager.onConsoleObject(detail, callbackId);
    }
  }

  @CalledByNative
  public void startCasting(int quality, int max_width, int max_height, String screenshot_mode) {
    if (mCastHelper != null) {
      mCastHelper.startCasting(quality, max_width, max_height, screenshot_mode, mDevToolDelegate);
    }
  }

  @CalledByNative
  public long getTemplateDataPtr() {
    if (mReloadHelper != null) {
      return mReloadHelper.getTemplateDataPtr();
    }
    return 0;
  }

  @CalledByNative
  public String getTemplateJsInfo(int offset, int size) {
    if (mReloadHelper != null) {
      return mReloadHelper.getTemplateJsInfo(offset, size);
    }
    return "";
  }

  public void dispatchScreencastVisibilityChanged(boolean status) {
    if (mFacadePtr != 0) {
      nativeDispatchScreencastVisibilityChanged(mFacadePtr, status);
    }
  }

  @CalledByNative
  public void onAckReceived() {
    if (mCastHelper != null) {
      mCastHelper.onAckReceived();
    }
  }

  @CalledByNative
  public void sendCardPreview() {
    if (mCastHelper != null) {
      mCastHelper.sendCardPreview(mDevToolDelegate);
    }
  }

  @CalledByNative
  public String getLynxUITree() {
    if (mUITreeHelper != null) {
      return mUITreeHelper.getLynxUITree();
    }
    return "";
  }

  @CalledByNative
  public String getUINodeInfo(int id) {
    if (mUITreeHelper != null) {
      return mUITreeHelper.getUINodeInfo(id);
    }
    return "";
  }

  @CalledByNative
  public int setUIStyle(int id, String name, String content) {
    if (mUITreeHelper != null) {
      return mUITreeHelper.setUIStyle(id, name, content);
    }
    return -1;
  }

  public void sendCardPreviewData(String data) {
    if (mFacadePtr != 0) {
      nativeSendLynxScreenshotCapturedEvent(mFacadePtr, data);
    }
  }

  public void sendScreenCast(String bitmap, ScreenCapturer.ScreenMetadata metadata,
      float pageScaleFactor, float timeCost) {
    if (bitmap == null || metadata == null) {
      return;
    }
    if (mFacadePtr != 0) {
      nativeSendPageScreencastFrameEvent(mFacadePtr, bitmap, metadata.mOffsetTop, pageScaleFactor,
          metadata.mDeviceWidth, metadata.mDeviceHeight, metadata.mScrollOffsetX,
          metadata.mScrollOffsetY, metadata.mTimestamp);
    }
  }

  public void sendConsoleEvent(String text, int level, long timestamp) {
    if (mFacadePtr != 0) {
      nativeSendConsoleEvent(mFacadePtr, text, level, timestamp);
    }
  }

  public void sendLayerTreeDidChangeEvent() {
    if (mFacadePtr != 0) {
      nativeSendLayerTreeDidChangeEvent(mFacadePtr);
    }
  }

  @CalledByNative
  public void stopCasting() {
    if (mCastHelper != null) {
      mCastHelper.stopCasting();
    }
  }

  public void continueCasting() {
    if (mCastHelper != null) {
      mCastHelper.continueCasting();
    }
  }

  public void pauseCasting() {
    if (mCastHelper != null) {
      mCastHelper.pauseCasting();
    }
  }

  @CalledByNative
  void pageReload(
      boolean ignoreCache, String templateBin, boolean fromTemplateFragments, int templateSize) {
    if (mReloadHelper != null) {
      LynxView lynxView = mLynxView.get();
      if (lynxView != null) {
        Toast.makeText(lynxView.getContext(), "Start to download & reload...", Toast.LENGTH_SHORT)
            .show();
      }
      mReloadHelper.reload(ignoreCache, templateBin, fromTemplateFragments, templateSize);
    }
  }

  @CalledByNative
  void navigate(String url) {
    if (mReloadHelper != null) {
      mNavigatePending = true;
      mReloadHelper.navigate(url);
    }
  }

  public void onLoadFinished() {
    if (mNavigatePending) {
      mNavigatePending = false;
      dispatchFrameNavigated();
    }
  }

  public void dispatchFrameNavigated() {
    if (mFacadePtr != 0) {
      nativeSendPageFrameNavigatedEvent(mFacadePtr, getTemplateUrl());
    }
  }

  public String getTemplateUrl() {
    if (mReloadHelper != null) {
      return mReloadHelper.getURL();
    }
    LLog.w(TAG, "mReloadHelper == null");
    return "";
  }

  public void attach(LynxView lynxView) {
    mLynxView = new WeakReference<>(lynxView);
    if (mCastHelper != null) {
      mCastHelper.attach(lynxView);
    }
    if (mTouchHelper != null) {
      mTouchHelper.attach(lynxView);
    }
  }

  @CalledByNative
  public void setLepusDebugInfoUrl(String url) {
    if (mLepusDebugInfoHelper != null) {
      mLepusDebugInfoHelper.setDebugInfoUrl(url);
    }
  }

  public String getLepusDebugInfoUrl() {
    if (mLepusDebugInfoHelper != null) {
      return mLepusDebugInfoHelper.getDebugInfoUrl();
    }
    return "";
  }

  @CalledByNative
  public String getLepusDebugInfo(String url) {
    if (mLepusDebugInfoHelper != null) {
      return mLepusDebugInfoHelper.getDebugInfo(url);
    }
    return "";
  }

  public void destroy() {
    if (mFacadePtr != 0) {
      nativeDestroyDevToolPlatformFacade(mFacadePtr);
    }
    mFacadePtr = 0;
  }

  @CalledByNative
  public void emulateTouch(final String type, final int x, final int y, final float deltaX,
      final float deltaY, final String button) {
    if (mTouchHelper == null) {
      return;
    }
    LynxView lynxView = mLynxView.get();
    if (lynxView != null) {
      float scale = lynxView.getLynxContext().getResources().getDisplayMetrics().density;
      mTouchHelper.emulateTouch(type, (int) (x * scale + 0.5f), (int) (y * scale + 0.5f),
          deltaX * scale + 0.5f, deltaY * scale + 0.5f, button, mDevToolDelegate);
    }
  }

  @CalledByNative
  public float[] getTransformValue(int sign, float[] padBorderMarginLayout) {
    if (mDevToolDelegate != null) {
      return mDevToolDelegate.getTransformValue(sign, padBorderMarginLayout);
    }
    return new float[0];
  }

  @CalledByNative
  public float[] getRectToWindow() {
    if (mUITreeHelper != null) {
      return mUITreeHelper.getRectToWindow();
    }
    return new float[] {0, 0, 0, 0};
  }

  @CalledByNative
  public static String getLynxVersion() {
    return LynxEnv.inst().getLynxVersion();
  }

  @CalledByNative
  public static void setDevToolSwitch(String key, boolean value) {
    LynxDevtoolEnv.inst().setDevtoolEnvMask(key, value);
  }

  @CalledByNative
  public void onReceiveTemplateFragment(String data, boolean eof) {
    if (mReloadHelper != null) {
      mReloadHelper.onReceiveTemplateFragment(data, eof);
    }
  }

  @CalledByNative
  public int[] getViewLocationOnScreen() {
    int[] res = {-1, -1};
    if (mUITreeHelper != null) {
      mUITreeHelper.getViewLocationOnScreen(res);
      return res;
    }
    return res;
  }

  @CalledByNative
  void sendEventToVM(String vmType, String eventName, String data) {
    if (mDevToolDelegate != null && !TextUtils.isEmpty(vmType) && !TextUtils.isEmpty(eventName)) {
      JavaOnlyMap map = new JavaOnlyMap();
      map.putString("type", eventName);
      map.putString("data", data != null ? data : "");
      // Note this will be checked by `TemplateAssembler::GetContextProxy`, accepted values are from
      // `lynx::runtime::ContextProxy::Type` Currently, only `DevTool` will send message to VM.
      map.putString("origin", "Devtool");
      map.putString("target", vmType);
      mDevToolDelegate.onDispatchMessageEvent(map);
    }
  }
  private native long nativeCreateDevToolPlatformFacade();
  private native void nativeDestroyDevToolPlatformFacade(long facadePtr);
  private native void nativeSendPageScreencastFrameEvent(long facadePtr, String data,
      float offsetTop, float pageScaleFactor, float deviceWidth, float deviceHeight,
      float scrollOffsetX, float scrollOffsetY, float timestamp);
  private native void nativeSendPageFrameNavigatedEvent(long facadePtr, String url);
  private native void nativeDispatchScreencastVisibilityChanged(long facadePtr, boolean status);
  private native void nativeSendLynxScreenshotCapturedEvent(long facadePtr, String data);
  public native void nativeFlushConsoleMessages(long facadePtr);
  public native void nativeGetConsoleObject(
      long facadePtr, String objectId, boolean needStringify, int callbackId);
  private native void nativeSendConsoleEvent(
      long facadePtr, String text, int level, long timestamp);
  private native void nativeSendLayerTreeDidChangeEvent(long facadePtr);
}
