// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.module;

import com.lynx.devtool.LynxDevtoolEnv;
import com.lynx.devtoolwrapper.CDPResultCallback;
import com.lynx.devtoolwrapper.DevToolSettings;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwner;
import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.react.bridge.Callback;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.behavior.LynxContext;

public class LynxDevToolSetModule extends LynxContextModule {
  public static final String NAME = "LynxDevToolSetModule";

  public LynxDevToolSetModule(LynxContext context) {
    super(context);
  }

  @LynxMethod
  public void switchLynxDebug(Boolean arg) {
    LynxEnv.inst().enableLynxDebug(arg);
  }

  @LynxMethod
  public boolean isLynxDebugEnabled() {
    return LynxEnv.inst().isLynxDebugEnabled();
  }

  @LynxMethod
  public void switchDevTool(Boolean arg) {
    LynxEnv.inst().enableDevtool(arg);
  }

  @LynxMethod
  public boolean isDevToolEnabled() {
    return LynxEnv.inst().isDevtoolEnabled();
  }

  @LynxMethod
  public void switchLogBox(Boolean arg) {
    LynxEnv.inst().enableLogBox(arg);
  }

  @LynxMethod
  public boolean isLogBoxEnabled() {
    return LynxEnv.inst().isLogBoxEnabled();
  }

  @LynxMethod
  public void switchFspScreenshot(Boolean arg) {
    LynxEnv.inst().enableFspScreenshot(arg);
  }

  @LynxMethod
  public boolean isFspScreenshotEnabled() {
    return LynxEnv.inst().isFspScreenshotEnabled();
  }

  @LynxMethod
  public void switchHighlightTouch(Boolean arg) {
    LynxEnv.inst().enableHighlightTouch(arg);
  }

  @LynxMethod
  public boolean isHighlightTouchEnabled() {
    return LynxEnv.inst().isHighlightTouchEnabled();
  }

  @LynxMethod
  public void switchQuickjsCache(Boolean arg) {
    DevToolSettings.inst().setQuickJSCacheEnabled(arg);
  }

  @LynxMethod
  public boolean isQuickjsCacheEnabled() {
    return DevToolSettings.inst().isQuickJSCacheEnabled();
  }

  @LynxMethod
  public boolean isDebugModeEnabled() {
    return LynxEnv.inst().isDebugModeEnabled();
  }

  @LynxMethod
  public void switchDebugModeEnable(Boolean arg) {
    LynxEnv.inst().enableDebugMode(arg);
  }

  @LynxMethod
  public boolean isLaunchRecord() {
    return LynxEnv.inst().isLaunchRecordEnabled();
  }

  @LynxMethod
  public void switchLaunchRecord(Boolean arg) {
    LynxEnv.inst().enableLaunchRecord(arg);
  }

  @LynxMethod
  public void switchV8(int arg) {
    DevToolSettings.inst().setV8Enabled(arg);
  }

  @LynxMethod
  public int getV8Enabled() {
    return DevToolSettings.inst().getV8Enabled();
  }

  @LynxMethod
  public void enableDomTree(Boolean arg) {
    DevToolSettings.inst().setDOMTreeEnabled(arg);
  }

  @LynxMethod
  public boolean isDomTreeEnabled() {
    return DevToolSettings.inst().isDOMTreeEnabled();
  }

  @LynxMethod
  public void switchLongPressMenu(Boolean arg) {
    DevToolSettings.inst().setLongPressMenuEnabled(arg);
  }

  @LynxMethod
  public boolean isLongPressMenuEnabled() {
    return DevToolSettings.inst().isLongPressMenuEnabled();
  }

  @LynxMethod
  public void switchIgnorePropErrors(Boolean arg) {
    LynxDevtoolEnv.inst().setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_IGNORE_ERROR_CSS, arg);
  }

  @LynxMethod
  public boolean isIgnorePropErrorsEnabled() {
    return LynxDevtoolEnv.inst().getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_IGNORE_ERROR_CSS, false);
  }

  @LynxMethod
  public void switchPixelCopy(Boolean arg) {
    LynxEnv.inst().enablePixelCopy(arg);
  }

  @LynxMethod
  public boolean isPixelCopyEnabled() {
    return LynxEnv.inst().isPixelCopyEnabled();
  }

  @LynxMethod
  public void switchQuickjsDebug(Boolean arg) {
    DevToolSettings.inst().setQuickJSDebugEnabled(arg);
  }

  @LynxMethod
  public boolean isQuickjsDebugEnabled() {
    return DevToolSettings.inst().isQuickJSDebugEnabled();
  }

  @LynxMethod
  public void invokeCdp(String message, Callback callback) {
    LynxBaseInspectorOwner owner = mLynxContext.getBaseInspectorOwner();
    if (owner == null) {
      return;
    }
    owner.invokeCDPFromSDK(message, new CDPResultCallback() {
      @Override
      public void onResult(String result) {
        if (mLynxContext == null || callback == null) {
          return;
        }
        mLynxContext.runOnJSThread(new Runnable() {
          @Override
          public void run() {
            callback.invoke(result);
          }
        });
      }
    });
  }
}
