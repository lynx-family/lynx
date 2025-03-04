// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.utils;

import androidx.annotation.IntDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@IntDef({BorderWidth.THIN, BorderWidth.MEDIUM, BorderWidth.THICK})
@Retention(RetentionPolicy.SOURCE)
@interface BorderWidth {
  int THIN = 1;
  int MEDIUM = 3;
  int THICK = 5;
}
