// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.memory;

import java.util.Map;

/**
 * @brief The protocol defines the interface for monitoring memory usage.
 *
 * This protocol is adopted by classes that need to monitor memory usage, such as
 * memory allocations, deallocations, and updates. Implementing this protocol
 * allows classes to receive notifications about memory usage changes and take
 * appropriate actions, such as logging or reporting.
 */
public interface IMemoryMonitor {
  /**
   * @brief Increments memory usage and sends a PerformanceEntry. This interface
   * increases the total memory usage for the specified category based on the
   * provided size and detail.
   * @param builder The builder used to construct the MemoryRecord.
   */
  public void allocateMemory(IMemoryRecordBuilder builder);

  /**
   * @brief Decrements memory usage and sends a PerformanceEntry. This interface
   * decreases the total memory usage for the specified category based on the
   * provided size and detail.
   * @param builder The builder used to construct the MemoryRecord.
   */
  public void deallocateMemory(IMemoryRecordBuilder builder);

  /**
   * @brief Updates memory usage and sends a PerformanceEntry. This interface
   * updates the total memory usage for the specified category based on the
   * provided size and detail.
   * @param builder The builder used to construct the MemoryRecord.
   */
  public void updateMemoryUsage(IMemoryRecordBuilder builder);

  /**
   * @brief Replaces the complete UI memory snapshot, including an empty snapshot.
   * @param recordMap The map of UI MemoryRecord.
   */
  public void updateUIMemoryUsage(Map<String, MemoryRecord> recordMap);
}
