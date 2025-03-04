// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import androidx.annotation.NonNull;
import com.lynx.devtoolwrapper.CDPResultCallback;
import com.lynx.devtoolwrapper.MessageHandler;

public class LynxDevToolNGDelegate {
  // native LynxDevToolNG
  private long mLynxDevToolNGPtr = 0;

  private int mSessionId = 0;

  public LynxDevToolNGDelegate() {
    mLynxDevToolNGPtr = nativeCreateLynxDevToolNG();
  }

  public int getSessionId() {
    return mSessionId;
  }

  public boolean isAttachToDebugRouter() {
    return mSessionId != 0;
  }

  private native long nativeCreateLynxDevToolNG();

  public void sendMessageToDebugPlatform(@NonNull String type, @NonNull String msg) {
    if (mLynxDevToolNGPtr != 0) {
      nativeSendMessageToDebugPlatform(mLynxDevToolNGPtr, type, msg);
    }
  }

  private native void nativeSendMessageToDebugPlatform(long nativePtr, String type, String msg);

  public long onBackgroundRuntimeCreated(String groupName) {
    if (mLynxDevToolNGPtr != 0) {
      return nativeOnBackgroundRuntimeCreated(mLynxDevToolNGPtr, groupName);
    }
    return 0;
  }

  private native long nativeOnBackgroundRuntimeCreated(long nativePtr, String groupName);

  public void onTASMCreated(long shellPtr) {
    if (mLynxDevToolNGPtr != 0) {
      nativeOnTasmCreated(mLynxDevToolNGPtr, shellPtr);
    }
  }

  private native void nativeOnTasmCreated(long nativePtr, long shellPtr);

  public void destroy() {
    if (mLynxDevToolNGPtr != 0) {
      nativeDestroy(mLynxDevToolNGPtr);
      mLynxDevToolNGPtr = 0;
    }
  }

  private native void nativeDestroy(long nativePtr);

  public int attachToDebug(String url) {
    if (mLynxDevToolNGPtr != 0) {
      mSessionId = nativeAttachToDebug(mLynxDevToolNGPtr, url);
      return mSessionId;
    }
    return 0;
  }

  private native int nativeAttachToDebug(long nativePtr, String url);

  public void detachToDebug() {
    if (mLynxDevToolNGPtr != 0) {
      nativeDetachToDebug(mLynxDevToolNGPtr);
      mSessionId = 0;
    }
  }

  private native void nativeDetachToDebug(long nativePtr);

  public void setDevToolPlatformAbility(long platformNativePtr) {
    if (mLynxDevToolNGPtr != 0) {
      nativeSetDevToolPlatformAbility(mLynxDevToolNGPtr, platformNativePtr);
    }
  }

  private native void nativeSetDevToolPlatformAbility(long nativePtr, long platformNativePtr);

  public void subscribeMessage(String type, MessageHandler handler) {
    nativeSubscribeMessage(mLynxDevToolNGPtr, type, new DevToolMessageHandlerDelegate(handler));
  }

  private native void nativeSubscribeMessage(
      long nativePtr, String type, DevToolMessageHandlerDelegate handler);

  public void unSubscribeMessage(String type) {
    nativeUnSubscribeMessage(mLynxDevToolNGPtr, type);
  }

  private native void nativeUnSubscribeMessage(long nativePtr, String type);

  public void invokeCDPFromSDK(String cdpMsg, CDPResultCallback callback) {
    nativeInvokeCDPFromSDK(mLynxDevToolNGPtr, cdpMsg, new CDPResultCallbackWrapper(callback));
  }

  private native void nativeInvokeCDPFromSDK(
      long nativePtr, String cdpMsg, CDPResultCallbackWrapper callback);

  public void updateScreenMetrics(int width, int height, float density) {
    nativeUpdateDevice(width, height, density);
  }

  private native void nativeUpdateDevice(int width, int height, float density);
}
