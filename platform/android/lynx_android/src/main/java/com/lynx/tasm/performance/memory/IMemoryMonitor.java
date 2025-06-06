// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.memory;

public interface IMemoryMonitor {
  public void allocateMemory(IMemoryRecordBuilder builder);

  public void deallocateMemory(IMemoryRecordBuilder builder);

  public void updateMemoryUsage(IMemoryRecordBuilder builder);
}
