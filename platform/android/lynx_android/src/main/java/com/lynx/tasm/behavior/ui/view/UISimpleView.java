// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.view;

import android.content.Context;
import android.view.ViewGroup;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIGroup;

public abstract class UISimpleView<T extends ViewGroup> extends UIGroup<T> {
  @Deprecated
  public UISimpleView(Context context) {
    this((LynxContext) context);
  }

  public UISimpleView(LynxContext context) {
    this(context, null);
  }

  public UISimpleView(LynxContext context, Object param) {
    super(context, param);
  }
}
