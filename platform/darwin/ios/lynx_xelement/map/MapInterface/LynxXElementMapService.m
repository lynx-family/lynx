// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxXElementMapService.h"

@implementation LynxXElementMapService

static id<LynxMapProvider> mapProvider;

+ (void)registerProvider:(id<LynxMapProvider>)provider {
  @synchronized(self) {
    mapProvider = provider;
  }
}

+ (id<LynxMapProvider>)provider {
  @synchronized(self) {
    if (!mapProvider) {
      [NSException raise:NSInternalInconsistencyException
                  format:@"Register a map provider before creating map elements"];
    }
    return mapProvider;
  }
}

@end
