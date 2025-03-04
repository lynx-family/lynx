// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.Keep;

@Keep
public abstract class LynxModule {
  protected Context mContext;
  protected Object mParam;
  protected Object mExtraData;

  @Keep
  public LynxModule(Context context) {
    this(context, null);
  }

  @Keep
  public LynxModule(Context context, Object param) {
    mContext = context;
    mParam = param;
  }

  public void setExtraData(Object data) {
    mExtraData = data;
  }

  @Keep
  public void destroy() {}
}
