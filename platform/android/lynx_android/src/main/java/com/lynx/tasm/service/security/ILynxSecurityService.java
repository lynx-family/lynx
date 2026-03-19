// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service.security;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import com.lynx.tasm.service.IServiceProvider;
import java.nio.ByteBuffer;

@Keep
public interface ILynxSecurityService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxSecurityService.class;
  }
  enum LynxTasmType { TYPE_TEMPLATE, TYPE_DYNAMIC_COMPONENT }

  /**
   * use specified verify logic to check the template consistency.
   *
   * @param template: the input tasm binary.
   * @param url: the input tasm identify.
   *
   * @return result of the verification.
   */
  SecurityResult verifyTASM(ILynxSecurityTarget target, byte[] template, ByteBuffer buffer,
      String url, LynxTasmType type);
}
