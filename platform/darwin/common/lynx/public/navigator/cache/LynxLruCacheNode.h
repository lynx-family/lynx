// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_NAVIGATOR_CACHE_LYNXLRUCACHENODE_H_
#define DARWIN_COMMON_LYNX_NAVIGATOR_CACHE_LYNXLRUCACHENODE_H_

#import <Foundation/Foundation.h>

@interface LynxLruCacheNode : NSObject <NSCoding>

@property(nonatomic, strong, readonly) id value;
@property(nonatomic, strong, readonly) id<NSCopying> key;
@property(nonatomic, strong) LynxLruCacheNode *next;
@property(nonatomic, strong) LynxLruCacheNode *prev;

+ (instancetype)nodeWithValue:(id)value key:(id<NSCopying>)key;
- (instancetype)initWithValue:(id)value key:(id<NSCopying>)key;

@end

#endif  // DARWIN_COMMON_LYNX_NAVIGATOR_CACHE_LYNXLRUCACHENODE_H_
