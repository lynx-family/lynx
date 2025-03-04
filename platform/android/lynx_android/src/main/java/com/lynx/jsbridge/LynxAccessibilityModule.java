// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.SafeRunnable;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.UIThreadUtils;

public class LynxAccessibilityModule extends LynxContextModule {
  public static final String NAME = "LynxAccessibilityModule";
  public static final String MSG_MUTATION_STYLES = "mutation_styles";
  public static final String MSG_CONTENT = "content";
  public static final String MSG = "msg";

  public LynxAccessibilityModule(LynxContext context) {
    super(context);
  }

  @LynxMethod
  void registerMutationStyle(final ReadableMap params, final Callback callback) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        JavaOnlyMap res = new JavaOnlyMap();
        registerMutationStyleInner(params, res);
        callback.invoke(res);
      }
    });
  }

  void registerMutationStyleInner(final ReadableMap params, final JavaOnlyMap res) {
    if (mLynxContext.getLynxAccessibilityWrapper() == null) {
      res.putString(MSG, "Fail: init accessibility env error with a11y wrapper is null");
      return;
    }
    mLynxContext.getLynxAccessibilityWrapper().registerMutationStyleInner(params, res);
  }

  @LynxMethod
  void accessibilityAnnounce(final ReadableMap params, final Callback callback) {
    UIThreadUtils.runOnUiThread(new SafeRunnable(mLynxContext) {
      @Override
      public void unsafeRun() {
        JavaOnlyMap res = new JavaOnlyMap();
        accessibilityAnnounceInner(params, res);
        callback.invoke(res);
      }
    });
  }

  void accessibilityAnnounceInner(final ReadableMap params, final JavaOnlyMap res) {
    String content = params != null ? params.getString(MSG_CONTENT) : null;
    if (content != null) {
      if (mLynxContext != null && mLynxContext.getLynxView() != null) {
        mLynxContext.getLynxView().announceForAccessibility(content);
        res.putString(MSG, "Success");
      } else {
        res.putString(MSG, "Error: LynxView missing");
      }
    } else {
      res.putString(MSG, "Params error: no content found");
    }
  }
}
