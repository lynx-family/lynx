// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image.model;

import androidx.annotation.IntDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@IntDef({DiskCacheChoice.DEFAULT_DISK, DiskCacheChoice.SMALL_DISK, DiskCacheChoice.CUSTOM_DISK})
@Retention(RetentionPolicy.SOURCE)
public @interface DiskCacheChoice {
  int DEFAULT_DISK = 0;
  int SMALL_DISK = 1;
  int CUSTOM_DISK = 2;
}
