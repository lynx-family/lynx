// Copyright 2024 The Lynx Authors.All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.basedevtool;

import androidx.annotation.Keep;
import com.lynx.basedevtool.CalledByNative;
import com.lynx.debugrouter.DebugRouterSlot;
import com.lynx.debugrouter.DebugRouterSlotDelegate;

@Keep
public class DevToolSlot implements DebugRouterSlotDelegate {
  private static final int STATUS_UNINITIALIZED = -1;
  private long mNativeHandler = STATUS_UNINITIALIZED;
  private DebugRouterSlot mDebugRouterSlot = null;
  private String mTemplateUrl = "";

  static {
    BaseDevToolLoadSoUtils.loadSo();
  }

  @CalledByNative
  public static DevToolSlot createInstance(long nativeHandler) {
    return new DevToolSlot(nativeHandler);
  }

  private DevToolSlot(long mNativeHandler) {
    this.mNativeHandler = mNativeHandler;
    mDebugRouterSlot = new DebugRouterSlot(this);
  }

  @Override
  public String getTemplateUrl() {
    return mTemplateUrl;
  }

  /**
   * @param type non-null
   * @param message non-null
   */
  @Override
  public void onMessage(String type, String message) {
    nativeOnSlotMessage(mNativeHandler, type, message);
  }

  private native void nativeOnSlotMessage(long nativeHandler, String type, String message);

  @CalledByNative
  public int plug(String url) {
    mTemplateUrl = url;
    return mDebugRouterSlot.plug();
  }

  @CalledByNative
  public void pull() {
    mDebugRouterSlot.pull();
  }

  @CalledByNative
  public void sendMessage(String type, String msg) {
    mDebugRouterSlot.sendDataAsync(type, msg);
  }
}
