// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import androidx.annotation.RestrictTo;
import androidx.annotation.UiThread;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;

// TODO: Implement page initialization and rendering for host-owned surfaces.
@UiThread
public final class LynxPageView extends UIBodyView {
  /** Internal constructor; callers should use the host's createPageView method. */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public LynxPageView(Context context) {
    super(context);
  }

  /** Placeholder for releasing page resources. */
  public void destroy() {}
}
