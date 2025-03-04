// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.common;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.nio.ByteBuffer;

public interface MessageCodec {
  /**
   * Encode the input message to ByteBuffer.
   * Return null if input object is not supported.
   */
  @Nullable ByteBuffer encodeMessage(@NonNull Object message);

  /**
   * Decode the ByteBuffer to Java Object.
   * Return null if message has wrong format.
   */
  @Nullable Object decodeMessage(@NonNull ByteBuffer message);
}
