// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export enum MemoryPressureLevel {
  /**
   * No problems, there is enough memory to use. This event is not sent via callback, but the enum
   * is used in other places to find out the current state of the system.
   */
  NONE = 0,
  /**
   * Modules are advised to free buffers that are cheap to re-allocate and not immediately needed.
   */
  MODERATE = 1,
  /**
   * At this level, modules are advised to free all possible memory.  The alternative is to be
   * killed by the system, which means all memory will have to be re-created, plus the cost of a
   * cold start.
   */
  CRITICAL = 2,
  /**
   * This must be the last value in the enum. The casing is different from the other values to make
   * this enum work well with the UMA_HISTOGRAM_ENUMERATION macro.
   */
  MAX_VALUE = CRITICAL,
}

