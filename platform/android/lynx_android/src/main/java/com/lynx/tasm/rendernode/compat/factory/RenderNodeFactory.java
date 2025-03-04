// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.rendernode.compat;

import android.os.Build;

public class RenderNodeFactory {
  volatile static RenderNodeFactory mInstance = null;

  public static RenderNodeFactory getInstance() {
    try {
      if (mInstance == null) {
        synchronized (RenderNodeFactory.class) {
          if (mInstance == null) {
            mInstance = new RenderNodeFactory();
          }
        }
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
    return mInstance;
  }

  public RenderNodeCompat createRenderNodeCompat() {
    RenderNodeCompat nodeCompat;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      nodeCompat = new RenderNodeImpl();
    } else {
      return null;
    }
    nodeCompat.init();
    return nodeCompat;
  }
}
