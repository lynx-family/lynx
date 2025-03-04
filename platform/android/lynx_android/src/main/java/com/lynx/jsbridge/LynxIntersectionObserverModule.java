// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.SafeRunnable;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxIntersectionObserver;
import com.lynx.tasm.behavior.LynxIntersectionObserverManager;
import com.lynx.tasm.utils.LynxConstants;
import com.lynx.tasm.utils.UIThreadUtils;

public class LynxIntersectionObserverModule extends LynxContextModule {
  public static final String NAME = "IntersectionObserverModule";

  public LynxIntersectionObserverModule(LynxContext context) {
    super(context);
  }

  @LynxMethod
  void createIntersectionObserver(
      final int observerId, final String componentSign, @Nullable final ReadableMap options) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        LynxIntersectionObserverManager manager = mLynxContext.getIntersectionObserverManager();
        if (manager.getObserverById(observerId) == null) {
          String sign = LynxConstants.LYNX_DEFAULT_COMPONENT_ID;
          if (!componentSign.isEmpty()) {
            sign = componentSign;
          }
          LynxIntersectionObserver observer =
              new LynxIntersectionObserver(manager, observerId, sign, options);
          manager.addIntersectionObserver(observer);
        }
      }
    });
  }

  @LynxMethod
  void relativeTo(
      final int observerId, final String selector, @Nullable final ReadableMap margins) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        LynxIntersectionObserverManager manager = mLynxContext.getIntersectionObserverManager();
        LynxIntersectionObserver observer = manager.getObserverById(observerId);
        if (observer != null) {
          observer.relativeTo(selector, margins);
        }
      }
    });
  }

  @LynxMethod
  void relativeToViewport(final int observerId, @Nullable final ReadableMap margins) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        LynxIntersectionObserverManager manager = mLynxContext.getIntersectionObserverManager();
        LynxIntersectionObserver observer = manager.getObserverById(observerId);
        if (observer != null) {
          observer.relativeToViewport(margins);
        }
      }
    });
  }

  @LynxMethod
  void relativeToScreen(final int observerId, @Nullable final ReadableMap margins) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        LynxIntersectionObserverManager manager = mLynxContext.getIntersectionObserverManager();
        LynxIntersectionObserver observer = manager.getObserverById(observerId);
        if (observer != null) {
          observer.relativeToScreen(margins);
        }
      }
    });
  }

  @LynxMethod
  void observe(final int observerId, final String selector, final int callbackId) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        LynxIntersectionObserverManager manager = mLynxContext.getIntersectionObserverManager();
        LynxIntersectionObserver observer = manager.getObserverById(observerId);
        if (observer != null) {
          observer.observe(selector, callbackId);
        }
      }
    });
  }

  @LynxMethod
  void disconnect(final int observerId) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        LynxIntersectionObserverManager manager = mLynxContext.getIntersectionObserverManager();
        LynxIntersectionObserver observer = manager.getObserverById(observerId);
        if (observer != null) {
          observer.disconnect();
        }
      }
    });
  }
}
