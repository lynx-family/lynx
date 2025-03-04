// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.navigator;

import android.content.Context;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.HashMap;
import java.util.Map;

public class NavigationModule extends LynxModule {
  public static String NAME = "NavigationModule";

  public NavigationModule(Context context) {
    super(context);
  }

  public NavigationModule(Context context, Object param) {
    super(context, param);
  }

  @LynxMethod
  public void registerRoute(final ReadableMap map) {
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        LynxNavigator.inst().registerRoute(map);
      }
    });
  }

  @LynxMethod
  public void navigateTo(final String url, final ReadableMap param) {
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        Map<String, Object> map = new HashMap<>();
        if (param != null) {
          map = param.asHashMap();
        }
        LynxNavigator.inst().navigate(url, map);
      }
    });
  }

  @LynxMethod
  public void replace(final String url, final ReadableMap param) {
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        Map<String, Object> map = new HashMap<>();
        if (param != null) {
          map = param.asHashMap();
        }
        LynxNavigator.inst().replace(url, map);
      }
    });
  }

  @LynxMethod
  public void goBack() {
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        LynxNavigator.inst().goBack();
      }
    });
  }

  @LynxMethod
  public String getString() {
    return NAME;
  }
}
