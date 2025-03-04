// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_NAVIGATOR_CACHE_LYNXLRUCACHE_H_
#define DARWIN_COMMON_LYNX_NAVIGATOR_CACHE_LYNXLRUCACHE_H_

#import <Foundation/Foundation.h>

@class LynxRoute;
@class LynxView;

typedef LynxView * (^LynxViewReCreateBlock)(LynxRoute *);
typedef void (^LynxViewEvictedBlock)(LynxView *);

@interface LynxLruCache : NSObject

@property(nonatomic, readonly, assign) NSUInteger capacity;

- (instancetype)initWithCapacity:(NSUInteger)capacity
                        recreate:(LynxViewReCreateBlock)createBlock
                     viewEvicted:(LynxViewEvictedBlock)evictedBlock;
- (void)setObject:(id)object forKey:(id)key;
- (id)objectForKey:(id)key;
- (id)removeObjectForKey:(id)key;

@end

#endif  // DARWIN_COMMON_LYNX_NAVIGATOR_CACHE_LYNXLRUCACHE_H_
