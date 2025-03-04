// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import android.app.Dialog;
import com.lynx.devtoolwrapper.OverlayService;
import java.util.ArrayList;

public class DevToolOverlayDelegate {
  private DevToolOverlayDelegate() {}

  private static class SingletonHelper {
    private static final DevToolOverlayDelegate INSTANCE = new DevToolOverlayDelegate();
  }

  public static DevToolOverlayDelegate getInstance() {
    return SingletonHelper.INSTANCE;
  }

  public ArrayList<Dialog> getGlobalOverlayNGView() {
    return mService != null ? mService.getGlobalOverlayNGView() : null;
  }

  public ArrayList<Integer> getAllVisibleOverlaySign() {
    return mService != null ? mService.getAllVisibleOverlaySign() : null;
  }

  public void init(OverlayService service) {
    mService = service;
  }

  private OverlayService mService;
}
