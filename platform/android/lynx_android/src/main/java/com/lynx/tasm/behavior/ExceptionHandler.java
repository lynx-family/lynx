// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import androidx.annotation.RestrictTo;
import org.json.JSONObject;

public interface ExceptionHandler {
  void handleException(Exception e);
  void handleException(Exception e, JSONObject userDefinedInfo);

  /**
   * @deprecated The error code of an exception should always be LYNX_ERROR_EXCEPTION
   * instead of being manually assigned. Please use handleException(Exception).
   */
  @Deprecated void handleException(Exception e, int errCode);

  /**
   * @deprecated The error code of an exception should always be LYNX_ERROR_EXCEPTION
   * instead of being manually assigned. Please use handleException(Exception, JSONObject).
   */
  @Deprecated void handleException(Exception e, int errCode, JSONObject userDefinedInfo);
}
