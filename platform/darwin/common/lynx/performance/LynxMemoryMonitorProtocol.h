// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxMemoryMonitorProtocol <NSObject>

@required
// Checks if memory monitoring is enabled.
// Modules can call this before collecting data to avoid unnecessary collection.
- (BOOL)enableMemoryMonitor;

// Increments memory usage and sends a PerformanceEntry.
// This interface increases the total memory usage for the specified category
// based on the provided size and detail.
- (void)allocateMemoryWithCategory:(NSString *_Nonnull)category
                            sizeKb:(float)sizeKb
                            detail:(NSDictionary<NSString *, NSString *> *_Nullable)detail;

// Decrements memory usage and sends a PerformanceEntry.
// This interface decreases the total memory usage for the specified category
// based on the provided size and detail.
- (void)deallocateMemoryWithCategory:(NSString *_Nonnull)category
                              sizeKb:(float)sizeKb
                              detail:(NSDictionary<NSString *, NSString *> *_Nullable)detail;

// Overwrites the memory usage and sends a PerformanceEntry.
// This interface updates the record corresponding to the specified category
// with the new size and detail information.
- (void)updateMemoryUsageWithCategory:(NSString *_Nonnull)category
                               sizeKb:(float)sizeKb
                               detail:(NSDictionary<NSString *, NSString *> *_Nullable)detail;

@end

NS_ASSUME_NONNULL_END
