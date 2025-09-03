// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image.model;

import androidx.annotation.IntDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@IntDef({CacheChoice.DISK, CacheChoice.BITMAP})
@Retention(RetentionPolicy.SOURCE)
public @interface CacheChoice {
  int DISK = 0;
  int BITMAP = 1;
}
