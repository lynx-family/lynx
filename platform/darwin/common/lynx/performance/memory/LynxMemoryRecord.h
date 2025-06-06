// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@class LynxMemoryRecord;
typedef LynxMemoryRecord* (^LynxMemoryRecordBuilder)();

@interface LynxMemoryRecord : NSObject

@property(nonatomic, copy, readonly) NSString* category;
@property(nonatomic, assign, readonly) float sizeKb;
@property(nonatomic, copy, readonly) NSDictionary<NSString*, NSString*>* detail;

- (instancetype)initWithCategory:(NSString*)category
                          sizeKb:(float)sizeKb
                          detail:(NSDictionary<NSString*, NSString*>*)detail;

@end
