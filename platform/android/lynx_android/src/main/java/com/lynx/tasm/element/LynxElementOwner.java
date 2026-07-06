// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.element;

import android.view.View;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.core.LynxEngineProxy;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class LynxElementOwner {
  static final int INVALID_SIGN = -1;

  static final int QUERY_ROOT = 0;
  static final int QUERY_STATUS = 1;
  static final int QUERY_IDENTITY = 2;
  static final int QUERY_PARENT = 3;
  static final int QUERY_CHILDREN = 4;
  static final int QUERY_FIND_BY_ID = 5;
  static final int QUERY_DATASET = 6;
  static final int QUERY_ATTRIBUTES = 7;
  static final int QUERY_ATTRIBUTE = 8;
  static final int QUERY_JSON = 9;
  static final int QUERY_POSITION = 10;

  private final WeakReference<LynxEngineProxy> mEngineProxy;
  private final WeakReference<LynxContext> mLynxContext;

  public LynxElementOwner(LynxEngineProxy engineProxy, LynxContext lynxContext) {
    mEngineProxy = new WeakReference<>(engineProxy);
    mLynxContext = new WeakReference<>(lynxContext);
  }

  public LynxElement element(int sign) {
    return sign == INVALID_SIGN ? null : new LynxElement(this, sign);
  }

  void query(
      int sign, int type, String argument, LynxEngineProxy.LynxElementQueryCallback callback) {
    LynxEngineProxy proxy = mEngineProxy.get();
    if (proxy == null) {
      if (callback != null) {
        callback.onResult(null);
      }
      return;
    }
    proxy.queryLynxElement(sign, type, argument, callback);
  }

  void getPlatformView(int sign, LynxElementCallback<View> callback) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        if (callback == null) {
          return;
        }
        LynxContext context = mLynxContext.get();
        if (context == null || context.getLynxUIOwner() == null) {
          callback.onResult(null);
          return;
        }
        LynxBaseUI ui = context.getLynxUIOwner().findLynxUIBySign(sign);
        if (ui instanceof LynxUI) {
          callback.onResult(((LynxUI<?>) ui).getView());
          return;
        }
        callback.onResult(null);
      }
    });
  }

  static Map<String, Object> asMap(Object value) {
    if (!(value instanceof JavaOnlyMap)) {
      return new HashMap<>();
    }
    return ((JavaOnlyMap) value).toHashMap();
  }

  static List<Integer> asSignList(Object value) {
    List<Integer> signs = new ArrayList<>();
    if (!(value instanceof JavaOnlyArray)) {
      return signs;
    }
    JavaOnlyArray array = (JavaOnlyArray) value;
    for (int i = 0; i < array.size(); i++) {
      signs.add(array.getInt(i));
    }
    return signs;
  }
}
