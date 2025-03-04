// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.MutableContextWrapper;
import androidx.annotation.Nullable;
import com.lynx.tasm.utils.ContextUtils;

class LynxBaseContext extends MutableContextWrapper {
  private boolean mHasAttached = false;
  // MutableContextWrapper should be used instead of directly replacing the Context object,
  // because using application cannot cover all interfaces, and here we need to be aware of
  // fallback interfaces.
  private MutableContextWrapper mWrapper = new MutableContextWrapper(null);

  public LynxBaseContext(Context base) {
    super(base);
    mWrapper.setBaseContext(base);
  }

  @Override
  public void setBaseContext(Context base) {
    super.setBaseContext(base);
    mWrapper.setBaseContext(base);
  }

  protected void setHasLynxViewAttached(boolean hasAttached) {
    mHasAttached = hasAttached;
  }

  public Context getContext() {
    if (mHasAttached) {
      return getBaseContext();
    } else {
      return mWrapper;
    }
  }

  public @Nullable Activity getActivity() {
    return ContextUtils.getActivity(this);
  }
}
