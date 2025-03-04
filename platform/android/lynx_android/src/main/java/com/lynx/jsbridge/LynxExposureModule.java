// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.SafeRunnable;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIExposure;
import com.lynx.tasm.utils.UIThreadUtils;

public class LynxExposureModule extends LynxContextModule {
  public static final String NAME = "LynxExposureModule";

  public LynxExposureModule(LynxContext context) {
    super(context);
  }

  // stop exposure detection, use options to control the behavior of stopExposure.
  // called by frontend
  @LynxMethod
  void stopExposure(ReadableMap options) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        UIExposure exposure = mLynxContext.getExposure();
        if (exposure != null) {
          exposure.stopExposure(options.asHashMap());
        }
      }
    });
  }

  // resume exposure detection, send exposure event for all exposed ui on screen.
  // called by frontend
  @LynxMethod
  void resumeExposure() {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        UIExposure exposure = mLynxContext.getExposure();
        if (exposure != null) {
          exposure.resumeExposure();
        }
      }
    });
  }

  // set the check frequency, use options to specify which frequency to set.
  // called by frontend
  @LynxMethod
  void setObserverFrameRate(ReadableMap options) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        UIExposure exposure = mLynxContext.getExposure();
        if (exposure != null) {
          exposure.setObserverFrameRate(options);
        }
      }
    });
  }
}
