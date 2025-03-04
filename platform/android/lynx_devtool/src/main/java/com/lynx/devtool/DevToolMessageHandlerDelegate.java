// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import androidx.annotation.Keep;
import com.lynx.devtoolwrapper.MessageHandler;
import com.lynx.tasm.base.CalledByNative;
import java.lang.ref.WeakReference;

@Keep
public class DevToolMessageHandlerDelegate {
  public DevToolMessageHandlerDelegate(MessageHandler messageHandler) {
    handlerRef = new WeakReference<MessageHandler>(messageHandler);
  }

  @CalledByNative
  public void onMessage(String message) {
    MessageHandler handler = handlerRef.get();
    if (handler != null) {
      handler.onMessage(message);
    }
  }

  private WeakReference<MessageHandler> handlerRef;
}
