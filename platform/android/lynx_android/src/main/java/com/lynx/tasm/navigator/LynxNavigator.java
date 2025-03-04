// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.navigator;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.UiThread;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxView;
import java.util.Iterator;
import java.util.Map;
import java.util.Stack;
import java.util.WeakHashMap;

public class LynxNavigator {
  private Stack<LynxCardManager> cardManagerStack = new Stack<>();
  private Map<LynxHolder, LynxCardManager> pageMap = new WeakHashMap<>();

  private static LynxNavigator lynxNavigator = new LynxNavigator();
  private LynxSchemaInterceptor interceptor;
  private int capacity = Integer.MAX_VALUE;

  private LynxNavigator() {}

  public static LynxNavigator inst() {
    return lynxNavigator;
  }

  public LynxNavigator setSchemaInterceptor(LynxSchemaInterceptor interceptor) {
    this.interceptor = interceptor;
    return this;
  }

  public LynxNavigator setMaxCapacity(int opacity) {
    this.capacity = opacity;
    return this;
  }

  @UiThread
  public void registerLynxHolder(LynxHolder holder) {
    registerLynxHolder(holder, null);
  }

  @UiThread
  public void registerLynxHolder(LynxHolder holder, LynxView initLynxView) {
    LynxCardManager cardManager = new LynxCardManager(holder, capacity);
    if (initLynxView != null) {
      cardManager.registerInitLynxView(initLynxView);
    }
    cardManagerStack.push(cardManager);
    pageMap.put(holder, cardManager);
  }

  public void unRegisterLynxHolder(LynxHolder holder) {
    LynxCardManager cardManager = pageMap.remove(holder);
    if (cardManager != null) {
      cardManager.onDestroy();
      Iterator<LynxCardManager> iterator = cardManagerStack.iterator();
      while (iterator.hasNext()) {
        if (iterator.next() == cardManager) {
          iterator.remove();
          break;
        }
      }
    }
  }

  @UiThread
  public void navigate(@NonNull String name, Map<String, Object> param) {
    if (!TextUtils.isEmpty(name)) {
      if (interceptor == null || !interceptor.intercept(name, param)) {
        LynxCardManager cardManager = getCurrentCardManager();
        if (cardManager != null) {
          cardManager.push(name, param);
        }
      }
    }
  }

  @UiThread
  public void replace(@NonNull String name, Map<String, Object> param) {
    if (!TextUtils.isEmpty(name)) {
      LynxCardManager cardManager = getCurrentCardManager();
      if (cardManager != null) {
        cardManager.replace(name, param);
      }
    }
  }

  @UiThread
  public void goBack() {
    LynxCardManager cardManager = getCurrentCardManager();
    if (cardManager != null) {
      cardManager.pop();
    }
  }

  @UiThread
  public void registerRoute(ReadableMap routeTable) {
    LynxCardManager cardManager = getCurrentCardManager();
    if (cardManager != null) {
      cardManager.registerRoute(routeTable);
    }
  }

  @UiThread
  public boolean onBackPressed(LynxHolder lynxHolder) {
    LynxCardManager cardManager = getCurrentCardManager();
    if (cardManager != null) {
      return cardManager.onBackPressed();
    }
    return false;
  }

  public LynxCardManager getCurrentCardManager() {
    if (cardManagerStack != null && !cardManagerStack.isEmpty()) {
      return cardManagerStack.peek();
    }
    return null;
  }

  public void onEnterForeground(LynxHolder lynxHolder) {
    LynxCardManager cardManager = getCurrentCardManager();
    if (cardManager != null) {
      cardManager.onEnterForeground();
    }
  }

  public void onEnterBackground(LynxHolder lynxHolder) {
    LynxCardManager cardManager = getCurrentCardManager();
    if (cardManager != null) {
      cardManager.onEnterBackground();
    }
  }
}
