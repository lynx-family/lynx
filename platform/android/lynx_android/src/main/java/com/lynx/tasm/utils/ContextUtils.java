// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.view.Window;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.LynxContext;

public class ContextUtils {
  @Nullable
  public static LynxContext toLynxContext(Context context) {
    // unwrap till LynxContext is revealed
    while (context instanceof ContextWrapper) {
      if (context instanceof LynxContext) {
        return (LynxContext) context;
      }
      context = ((ContextWrapper) context).getBaseContext();
    }
    return null;
  }

  @Nullable
  public static Activity getActivity(Context context) {
    while (context instanceof ContextWrapper) {
      if (context instanceof Activity) {
        return (Activity) context;
      }
      context = ((ContextWrapper) context).getBaseContext();
    }
    return null;
  }

  public static Window getWindow(Context context) {
    while (context instanceof ContextWrapper) {
      if (context instanceof Activity) {
        return ((Activity) context).getWindow();
      }
      context = ((ContextWrapper) context).getBaseContext();
    }
    return null;
  }
}
