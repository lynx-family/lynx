// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.util;

import androidx.annotation.Keep;
import com.lynx.animax.UIAnimaX;
import com.lynx.animax.service.AnimaXServiceCenter;
import com.lynx.animax.service.IAnimaXSettingService;
import com.lynx.animax.service.ServiceScope;
import com.lynx.animax.setting.LynxAnimaXSettingService;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxUI;

/**
 * An AnimaX global instance for init and create LynxUIAnimaX
 * Container will depend on it to init env and create lynx ui
 */
@Keep
public class LynxAnimaX {
  private static final String TAG = "LynxAnimaX";

  private static volatile LynxAnimaX sInstance;

  private ServiceScope mScope = ServiceScope.of(TAG);

  private volatile boolean mHasLibInit = false;

  private LynxAnimaX() {}

  public static LynxAnimaX inst() {
    if (sInstance == null) {
      synchronized (LynxAnimaX.class) {
        if (sInstance == null) {
          sInstance = new LynxAnimaX();
        }
      }
    }
    return sInstance;
  }

  public void init() {
    init(null);
  }

  public void init(INativeLibraryLoader loader) {
    if (!hasInitialized()) {
      initWithLock(loader);
    }
  }

  private synchronized void initWithLock(INativeLibraryLoader loader) {
    // AnimaX initialization
    if (!AnimaX.inst().hasInitialized()) {
      AnimaX.inst().init(loader == null ? null : new com.lynx.animax.base.INativeLibraryLoader() {
        @Override
        public void loadLibrary(String libName) throws UnsatisfiedLinkError {
          loader.loadLibrary(libName);
        }
      });
    }

    // Lynx AnimaX initialization
    if (!mHasLibInit) {
      createGlobalServices();

      mHasLibInit = true;
    }
  }

  public boolean hasInitialized() {
    return AnimaX.inst().hasInitialized() && mHasLibInit;
  }

  public LynxUI createUI(LynxContext context) {
    try {
      // Ensure that LynxAnimaX.inst().init() has been called.
      init();
      return new UIAnimaX(context);
    } catch (Throwable e) {
      AnimaXLog.e(TAG, "animax ui init error" + e.toString());
    }
    return null;
  }

  public ServiceScope getScope() {
    return mScope;
  }

  private void createGlobalServices() {
    AnimaXServiceCenter.inst().registerService(
        mScope, IAnimaXSettingService.class, new LynxAnimaXSettingService());
  }
}
