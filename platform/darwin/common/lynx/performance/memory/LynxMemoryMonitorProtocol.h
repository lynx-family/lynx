// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxMemoryRecord.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * @brief The protocol defines the interface for monitoring memory usage.
 *
 * This protocol is adopted by classes that need to monitor memory usage, such as
 * memory allocations, deallocations, and updates. Implementing this protocol
 * allows classes to receive notifications about memory usage changes and take
 * appropriate actions, such as logging or reporting.
 */
@protocol LynxMemoryMonitorProtocol <NSObject>

@required
// Increments memory usage and sends a PerformanceEntry.
// This interface increases the total memory usage for the specified category
// based on the provided size and detail.
- (void)allocateMemory:(LynxMemoryRecordBuilder _Nonnull)recordBuilder;

// Decrements memory usage and sends a PerformanceEntry.
// This interface decreases the total memory usage for the specified category
// based on the provided size and detail.
- (void)deallocateMemory:(LynxMemoryRecordBuilder _Nonnull)recordBuilder;

// Overwrites the memory usage and sends a PerformanceEntry.
// This interface updates the record corresponding to the specified category
// with the new size and detail information.
- (void)updateMemoryUsage:(LynxMemoryRecordBuilder _Nonnull)recordBuilder;

// Replaces the complete UI memory snapshot, including an empty snapshot.
- (void)updateUIMemoryUsage:(NSDictionary<NSString*, LynxMemoryRecord*>*)records;

// Checks if memory monitoring is enabled.
// Modules can call this before collecting data to avoid unnecessary
// collection.
+ (BOOL)isMemoryMonitorEnabled;

@end

NS_ASSUME_NONNULL_END
