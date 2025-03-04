// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Foundation/Foundation.h>

@protocol LynxMemoryReporter <NSObject>

@required

- (void)uploadImageInfo:(NSDictionary*)data;

@end

@interface LynxMemoryListener : NSObject

@property(nonatomic) NSMutableArray<id<LynxMemoryReporter>>* memoryReporters;

+ (instancetype)shareInstance;

- (void)uploadImageInfo:(NSDictionary*)data;

- (void)addMemoryReporter:(id<LynxMemoryReporter>)report;

- (void)removeMemoryReporter:(id<LynxMemoryReporter>)report;

@end
