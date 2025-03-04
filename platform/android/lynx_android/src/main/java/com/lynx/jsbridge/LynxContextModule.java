// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import androidx.annotation.Keep;
import com.lynx.tasm.behavior.LynxContext;

public class LynxContextModule extends LynxModule {
  protected LynxContext mLynxContext;

  public LynxContextModule(LynxContext context) {
    super(context);
    mLynxContext = context;
  }

  public LynxContextModule(LynxContext context, Object param) {
    super(context, param);
    mLynxContext = context;
  }

  @Keep
  @Override
  public void destroy() {
    super.destroy();
  }
}
