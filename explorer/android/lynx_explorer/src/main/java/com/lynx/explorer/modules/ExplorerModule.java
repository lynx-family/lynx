// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import android.content.Context;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.react.bridge.WritableMap;

public class ExplorerModule extends LynxModule {
  static final String DEVTOOL_SWITCH_ASSETS =
      "file://lynx?local://devtool_switch/switchPage/devtoolSwitch.lynx.bundle";

  public ExplorerModule(Context context) {
    super(context);
  }

  @LynxMethod
  public void openScan() {
    LynxModuleAdapter.getInstance().openScan();
  }

  @LynxMethod
  public void openSchema(String url) {
    LynxModuleAdapter.getInstance().openSchema(url);
  }

  @LynxMethod
  public void setThreadMode(int threadMode) {
    LynxModuleAdapter.getInstance().setThreadMode(threadMode);
  }

  @LynxMethod
  public void switchPreSize(boolean enablePresetSize) {
    LynxModuleAdapter.getInstance().setEnablePresetSize(enablePresetSize);
  }

  @LynxMethod
  public void switchRenderNode(boolean enableRenderNode) {
    LynxModuleAdapter.getInstance().enableRenderNode(enableRenderNode);
  }

  @LynxMethod
  public WritableMap getSettingInfo() {
    return LynxModuleAdapter.getInstance().getSettingInfo();
  }

  @LynxMethod
  public void navigateBack() {
    LynxModuleAdapter.getInstance().pageBack();
  }

  @LynxMethod
  public void openDevtoolSwitchPage() {
    LynxModuleAdapter.getInstance().openSchema(DEVTOOL_SWITCH_ASSETS);
  }
}
