// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.LynxServiceCenter;

public class LynxDevToolUtils {
  private static final String TAG = "LynxDevToolUtils";
  private static volatile ILynxDevToolService sDevToolService = null;

  static ILynxDevToolService getDevToolService() {
    if (sDevToolService == null) {
      sDevToolService = LynxServiceCenter.inst().getService(ILynxDevToolService.class);
    }
    return sDevToolService;
  }

  // set custom native loader for devtool.
  // the loader will be used to load v8 and devtool library.
  // the method must be used before devtool initialization.
  // the method is optional, if user does not set devtool loader,
  // devtool will load v8 and devtool native library by default way.
  static public void setDevToolLibraryLoader(INativeLibraryLoader loader) {
    ILynxDevToolService devToolService = getDevToolService();
    if (devToolService != null) {
      devToolService.devtoolEnvSetDevToolLibraryLoader(loader);
    } else {
      LLog.e(TAG, "failed to get DevToolService");
    }
  }
}
