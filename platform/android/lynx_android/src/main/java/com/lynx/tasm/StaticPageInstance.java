// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Map;

/** Platform contract implemented by a statically compiled page. */
public interface StaticPageInstance {
  void renderPage(@NonNull Map<String, Object> data, @Nullable Map<String, Object> globalProps);

  void updateMetaData(
      @Nullable Map<String, Object> data, @Nullable Map<String, Object> globalProps);

  void destroy();
}
