// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.provider;

import androidx.annotation.NonNull;

public interface ResProvider {
  void request(@NonNull final LynxResRequest requestParams, final LynxResCallback callback);
}
