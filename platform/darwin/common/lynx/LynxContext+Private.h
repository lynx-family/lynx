// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>

#include <stdint.h>

@class LynxEngineProxy;

@interface LynxContext ()

@property(nonatomic, weak) LynxEngineProxy* _Nullable engineProxy;

- (void)reportExternalMemoryWithTotalSize:(int64_t)totalSize garbageSize:(int64_t)garbageSize;

@end
