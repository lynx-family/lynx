// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.core.JSProxy;

public class JSModule {
  private final String mModuleName;
  private final JSProxy mProxy;

  public JSModule(String module, JSProxy proxy) {
    mModuleName = module;
    mProxy = proxy;
  }

  public void fire(String function, JavaOnlyArray args) {
    JavaOnlyArray jsArgs = (args != null) ? args : new JavaOnlyArray();
    mProxy.callFunction(mModuleName, function, jsArgs);
  }
}
