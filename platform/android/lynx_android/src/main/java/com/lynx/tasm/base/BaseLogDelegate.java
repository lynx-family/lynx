// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import com.lynx.base.log.AbsBaseLogDelegate;

public class BaseLogDelegate extends AbsBaseLogDelegate {
  private static AbsLogDelegate sLogDelegate;
  private static volatile BaseLogDelegate sInstance;

  public static BaseLogDelegate inst() {
    if (sInstance == null) {
      synchronized (BaseLogDelegate.class) {
        if (sInstance == null) {
          sInstance = new BaseLogDelegate();
        }
      }
    }
    return sInstance;
  }

  public void setDelegate(AbsLogDelegate delegate) {
    sLogDelegate = delegate;
  }

  @Override
  public void v(String tag, String msg) {
    if (sLogDelegate == null) {
      return;
    }
    sLogDelegate.v(tag, msg);
  }

  @Override
  public void d(String tag, String msg) {
    if (sLogDelegate == null) {
      return;
    }
    sLogDelegate.d(tag, msg);
  }

  @Override
  public void i(String tag, String msg) {
    if (sLogDelegate == null) {
      return;
    }
    sLogDelegate.i(tag, msg);
  }

  @Override
  public void w(String tag, String msg) {
    if (sLogDelegate == null) {
      return;
    }
    sLogDelegate.w(tag, msg);
  }

  @Override
  public void e(String tag, String msg) {
    if (sLogDelegate == null) {
      return;
    }
    sLogDelegate.e(tag, msg);
  }
}
