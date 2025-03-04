// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import com.lynx.tasm.provider.ILynxResourceRequestOperation;

public interface ILynxResourceServiceRequestOperation extends ILynxResourceRequestOperation {
  /**
   * @return ILynxResourceServiceResponse
   */
  ILynxResourceServiceResponse execute();

  /**
   * cancel the request.
   */
  void cancel();
}
