// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import androidx.annotation.Keep;
import com.lynx.devtoolwrapper.CDPEventListener;
import com.lynx.tasm.base.CalledByNative;
import java.lang.ref.WeakReference;

@Keep
public class CDPEventListenerWrapper {
  public CDPEventListenerWrapper(CDPEventListener eventListener) {
    listenerRef = new WeakReference<CDPEventListener>(eventListener);
  }

  @CalledByNative
  public void onEvent(String event) {
    CDPEventListener listener = listenerRef.get();
    if (listener != null) {
      listener.onEvent(event);
    }
  }

  private WeakReference<CDPEventListener> listenerRef;
}
