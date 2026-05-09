// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.extension;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

@Keep
public interface LynxExtensionProvider {
  void register(@NonNull LynxExtensionRegistry registry);
}
