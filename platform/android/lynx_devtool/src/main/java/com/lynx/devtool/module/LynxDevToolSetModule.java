// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.module;

import com.lynx.devtool.LynxDevtoolEnv;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwner;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwnerNG;
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
  public void switchHighlightTouch(Boolean arg) {
    LynxEnv.inst().enableHighlightTouch(arg);
  }

  @LynxMethod
  public boolean isHighlightTouchEnabled() {
    return LynxEnv.inst().isHighlightTouchEnabled();
  }

  @LynxMethod
  public void switchQuickjsCache(Boolean arg) {
    LynxDevtoolEnv.inst().enableQuickjsCache(arg);
  }

  @LynxMethod
  public boolean isQuickjsCacheEnabled() {
    return LynxDevtoolEnv.inst().isQuickjsCacheEnabled();
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
    LynxDevtoolEnv.inst().enableV8(arg);
  }

  @LynxMethod
  public int getV8Enabled() {
    return LynxDevtoolEnv.inst().getV8Enabled();
  }

  @LynxMethod
  public void enableDomTree(Boolean arg) {
    LynxDevtoolEnv.inst().enableDomTree(arg);
  }

  @LynxMethod
  public boolean isDomTreeEnabled() {
    return LynxDevtoolEnv.inst().isDomTreeEnabled();
  }

  @LynxMethod
  public void switchLongPressMenu(Boolean arg) {
    LynxDevtoolEnv.inst().enableLongPressMenu(arg);
  }

  @LynxMethod
  public boolean isLongPressMenuEnabled() {
    return LynxDevtoolEnv.inst().isLongPressMenuEnabled();
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
    LynxDevtoolEnv.inst().enableQuickjsDebug(arg);
  }

  @LynxMethod
  public boolean isQuickjsDebugEnabled() {
    return LynxDevtoolEnv.inst().isQuickjsDebugEnabled();
  }
}
