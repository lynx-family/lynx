// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.base;

import com.lynx.base.LynxBaseEnv;
import com.lynx.base.log.LynxLog;
import com.lynx.tasm.service.ILynxTraceService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.concurrent.atomic.AtomicBoolean;

public class LynxBaseTrace {
  private static final String TAG = "LynxBaseTrace";
  private static volatile boolean sIsNativeLibLoad = false;
  private static final AtomicBoolean sHasInit = new AtomicBoolean(false);

  public static void init() {
    if (!sHasInit.compareAndSet(false, true)) {
      return;
    }
    new Thread(new Runnable() {
      @Override
      public void run() {
        try {
          if (!sIsNativeLibLoad) {
            sIsNativeLibLoad = LynxBaseEnv.inst().isNativeLibraryLoaded();
          }
          if (sIsNativeLibLoad) {
            initNativeBaseTrace();
          }
        } catch (Exception error) {
          LynxLog.e("lynx", "init LynxBaseTrace exception [ " + error.getMessage() + " ]");
        }
      }
    }, "LynxTraceInit").start();
  }

  private static boolean initNativeBaseTrace() {
    long address = 0;
    ILynxTraceService service = LynxServiceCenter.inst().getService(ILynxTraceService.class);
    if (service == null) {
      nativeInitBaseTrace(0);
      LynxLog.i(TAG, "LynxBaseTrace init successfully by itself.");
      return true;
    }
    address = service.getDefaultTraceFunction();
    if (address != 0) {
      nativeInitBaseTrace(address);
      LynxLog.i(TAG,
          "LynxBaseTrace init successfully by custom LynxBaseTraceService. function native address "
              + "is " + address);
      return true;
    }
    LynxLog.i(TAG, "failed to init LynxBaseTrace dependency");
    return false;
  }

  private static native void nativeInitBaseTrace(long addr);
}
