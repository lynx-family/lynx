// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.navigator;

import java.util.Map;

public interface LynxSchemaInterceptor {
  /**
   * Schema Intercept, You can handle custom schema Here.
   */
  boolean intercept(String schema, Map<String, Object> map);
}
