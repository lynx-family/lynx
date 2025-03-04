// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.text;

import android.text.Layout;
import androidx.annotation.Nullable;

public interface IUIText {
  @Nullable Layout getTextLayout();
}
