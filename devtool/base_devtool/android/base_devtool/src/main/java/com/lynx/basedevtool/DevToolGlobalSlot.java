// Copyright 2024 The Lynx Authors.All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.basedevtool;

import androidx.annotation.Keep;
import com.lynx.basedevtool.CalledByNative;
import com.lynx.debugrouter.DebugRouter;
import com.lynx.debugrouter.DebugRouterGlobalHandler;

@Keep
public class DevToolGlobalSlot implements DebugRouterGlobalHandler {
  private static final int STATUS_UNINITIALIZED = -1;
  private static final int GLOBAL_MESSAGE_SESSION_ID = -1;
  private long mNativeHandler = STATUS_UNINITIALIZED;

  static {
    BaseDevToolLoadSoUtils.loadSo();
  }

  @CalledByNative
  public static DevToolGlobalSlot createInstance(long nativeHandler) {
    return new DevToolGlobalSlot(nativeHandler);
  }

  private DevToolGlobalSlot(long mNativeHandler) {
    this.mNativeHandler = mNativeHandler;
    DebugRouter.getInstance().addGlobalHandler(this);
  }

  @Override // TODO(zhoumingsong.smile) refactor, openCard should not coupled with message channel
  public void openCard(String url) {}

  @Override
  public void onMessage(String type, int sessionId, String message) {
    nativeOnGlobalSlotMessage(mNativeHandler, type, message);
  }

  private native void nativeOnGlobalSlotMessage(long nativeHandler, String type, String message);

  @CalledByNative
  public void sendMessage(String type, String msg) {
    DebugRouter.getInstance().sendDataAsync(type, GLOBAL_MESSAGE_SESSION_ID, msg);
  }
}
