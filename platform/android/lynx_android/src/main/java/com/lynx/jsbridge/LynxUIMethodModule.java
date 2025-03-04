// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.SafeRunnable;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.LynxConstants;
import com.lynx.tasm.utils.UIThreadUtils;

public class LynxUIMethodModule extends LynxContextModule {
  public static final String NAME = "LynxUIMethodModule";
  public LynxUIMethodModule(LynxContext context) {
    super(context);
  }

  // for compatibility with old getNodeRef
  @LynxMethod
  void invokeUIMethod(final String sign, final ReadableArray nodes, final String method,
      final ReadableMap params, final Callback callback) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        String componentSign = LynxConstants.LYNX_DEFAULT_COMPONENT_ID;
        if (!sign.isEmpty()) {
          componentSign = sign;
        }
        mLynxContext.invokeUIMethod(
            componentSign, nodes, method, params, LynxUIMethodModule.wrapCallback(callback));
      }
    });
  }

  private static Callback wrapCallback(final Callback jsCallback) {
    return new Callback() {
      @Override
      public void invoke(Object... args) {
        if (jsCallback == null) {
          return;
        }
        JavaOnlyMap res = new JavaOnlyMap();
        res.putInt("code", (Integer) args[0]);
        if (args.length > 1) {
          res.put("data", args[1]);
        }
        jsCallback.invoke(res);
      }
    };
  }
}
