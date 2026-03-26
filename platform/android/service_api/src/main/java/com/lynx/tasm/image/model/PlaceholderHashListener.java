// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image.model;

import androidx.annotation.NonNull;
import com.lynx.tasm.image.ImageContent;
public interface PlaceholderHashListener {
  void onSuccess(@NonNull ImageContent content);

  void onFailure(@NonNull Throwable t);
}
