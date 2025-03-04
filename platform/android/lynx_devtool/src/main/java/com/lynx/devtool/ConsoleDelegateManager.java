// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool;

import com.lynx.react.bridge.Callback;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class ConsoleDelegateManager {
  private LynxInspectorConsoleDelegate mConsoleDelegate = null;
  private HashMap<Integer, Callback> mConsoleObjectCallback = null;
  private static AtomicInteger nextCallbackId = new AtomicInteger(0);

  public void setLynxInspectorConsoleDelegate(
      Object delegate, DevToolPlatformAndroidDelegate platform, long facadePtr) {
    if (delegate instanceof LynxInspectorConsoleDelegate) {
      mConsoleDelegate = (LynxInspectorConsoleDelegate) delegate;
      if (platform != null && facadePtr != 0) {
        platform.nativeFlushConsoleMessages(facadePtr);
      }
    }
  }

  public void getConsoleObject(String objectId, boolean needStringify, Callback callback,
      DevToolPlatformAndroidDelegate platform, long facadePtr) {
    if (platform != null && facadePtr != 0) {
      if (mConsoleObjectCallback == null) {
        mConsoleObjectCallback = new HashMap<>();
      }
      int callbackId = nextCallbackId.decrementAndGet();
      mConsoleObjectCallback.put(callbackId, callback);
      platform.nativeGetConsoleObject(facadePtr, objectId, needStringify, callbackId);
    }
  }

  public void onConsoleMessage(String msg) {
    if (mConsoleDelegate != null) {
      mConsoleDelegate.onConsoleMessage(msg);
    }
  }

  public void onConsoleObject(String detail, int callbackId) {
    if (mConsoleObjectCallback != null && mConsoleObjectCallback.containsKey(callbackId)) {
      Callback callback = mConsoleObjectCallback.get(callbackId);
      if (callback != null) {
        callback.invoke(detail);
      }
      mConsoleObjectCallback.remove(callbackId);
    }
  }
}
