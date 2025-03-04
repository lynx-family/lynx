// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.common.LepusBuffer;
import java.nio.ByteBuffer;

/**
 * Async callBack for Lynx.
 */
public abstract class PlatformCallBack {
  public final static double InvalidId = -1;

  public abstract void onSuccess(Object data);

  @CalledByNative
  private void onDataBack(ByteBuffer buffer) {
    Object result = LepusBuffer.INSTANCE.decodeMessage(buffer);
    onSuccess(result);
  }
}
