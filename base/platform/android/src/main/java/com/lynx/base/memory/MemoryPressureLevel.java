// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.base.memory;

import androidx.annotation.IntDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@IntDef({MemoryPressureLevel.NONE, MemoryPressureLevel.MODERATE, MemoryPressureLevel.CRITICAL,
    MemoryPressureLevel.MAX_VALUE})
@Retention(RetentionPolicy.SOURCE)
public @interface MemoryPressureLevel {
  /**
   * No problems, there is enough memory to use. This event is not sent via callback, but the enum
   * is used in other places to find out the current state of the system.
   */
  int NONE = 0;
  /**
   * Modules are advised to free buffers that are cheap to re-allocate and not immediately needed.
   */
  int MODERATE = 1;
  /**
   * At this level, modules are advised to free all possible memory.  The alternative is to be
   * killed by the system, which means all memory will have to be re-created, plus the cost of a
   * cold start.
   */
  int CRITICAL = 2;
  /**
   * This must be the last value in the enum. The casing is different from the other values to make
   * this enum work well with the UMA_HISTOGRAM_ENUMERATION macro.
   */
  int MAX_VALUE = CRITICAL;
}
