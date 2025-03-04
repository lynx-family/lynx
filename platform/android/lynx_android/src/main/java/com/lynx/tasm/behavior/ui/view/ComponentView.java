// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.view;

import android.content.Context;

public class ComponentView extends AndroidView {
  private int mPosition;

  public ComponentView(Context context) {
    super(context);
  }

  public int getPosition() {
    return mPosition;
  }

  public void setPosition(int position) {
    this.mPosition = position;
  }
}
