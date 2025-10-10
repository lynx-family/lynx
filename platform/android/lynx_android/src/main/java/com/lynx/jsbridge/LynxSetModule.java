// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.service.ILynxLogService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.Field;

public class LynxSetModule extends LynxContextModule {
  public static final String NAME = "LynxSetModule";
  public LynxSetModule(LynxContext context) {
    super(context);
  }

  @LynxMethod
  public void switchKeyBoardDetect(boolean arg) {
    if (arg) {
      mLynxContext.getLynxView().getKeyboardEvent().start();
    }
  }

  @LynxMethod
  public void switchLogToSystem(boolean arg) {
    ILynxLogService service = LynxServiceCenter.inst().getService(ILynxLogService.class);
    if (service != null) {
      service.switchLogToSystem(arg);
    }
  }

  @LynxMethod
  public boolean getLogToSystemStatus() {
    ILynxLogService service = LynxServiceCenter.inst().getService(ILynxLogService.class);
    if (service != null) {
      return service.getLogToSystemStatus();
    }
    return false;
  }

  @LynxMethod
  public void switchEnableLayoutOnly(Boolean arg) {
    LynxEnv.inst().enableLayoutOnly(arg);
  }

  @LynxMethod
  public boolean getEnableLayoutOnly() {
    return LynxEnv.inst().isLayoutOnlyEnabled();
  }

  @LynxMethod
  public void switchIsCreateViewAsync(Boolean arg) {
    LynxEnv.inst().setCreateViewAsync(arg);
  }

  @LynxMethod
  public boolean getIsCreateViewAsync() {
    return LynxEnv.inst().getCreateViewAsync();
  }

  @LynxMethod
  public void switchEnableVsyncAlignedFlush(Boolean arg) {
    LynxEnv.inst().setVsyncAlignedFlushGlobalSwitch(arg);
  }

  @LynxMethod
  public boolean getEnableVsyncAlignedFlush() {
    return LynxEnv.inst().getVsyncAlignedFlushGlobalSwitch();
  }
}
