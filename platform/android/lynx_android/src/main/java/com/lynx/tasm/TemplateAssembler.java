// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.event.LynxCustomEvent;
import java.lang.ref.WeakReference;

/* package */
public class TemplateAssembler {
  private WeakReference<LynxContext> mLynxContext;

  private static final String TAG = "TemplateAssembler";

  // TODO(hexionghui): This interface will be deleted later. Since LynxSendCustomEventRunnable
  // relies on this interface, it cannot be deleted temporarily.
  public void sendCustomEvent(@NonNull LynxCustomEvent event) {
    if (mLynxContext.get() == null || mLynxContext.get().getEventEmitter() == null) {
      LLog.e(TAG,
          "sendCustomEvent event: " + event.getName()
              + " failed since mLynxContext or getEventEmitter() is null.");
      return;
    }
    mLynxContext.get().getEventEmitter().sendCustomEvent(event);
  }

  public void setLynxContext(LynxContext context) {
    mLynxContext = new WeakReference<>(context);
  }
}
