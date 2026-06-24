// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxAnimaXResourceFactoryServiceImpl.h>
#import <XElement/LynxAnimaXResourceFetcherLoader.h>
#include "src/base/log/log.h"

@interface LynxAnimaXResourceFactoryServiceImpl ()

/**
 * The UI context associated with this service.
 * Stored as weak to avoid retain cycles.
 */
@property(nonatomic, weak) LynxUIContext *context;

@end

@implementation LynxAnimaXResourceFactoryServiceImpl

- (instancetype)initWithLynxUIContext:(LynxUIContext *)context {
  self = [super init];
  if (self) {
    _context = context;
  }
  return self;
}

#pragma mark - AnimaXResourceFactoryService Protocol

- (NSArray<id<AnimaXLoaderProtocol>> *)createAnimaXLoaders {
  NSMutableArray<id<AnimaXLoaderProtocol>> *loaders = [NSMutableArray array];

  LynxAnimaXResourceFetcherLoader *httpLoader =
      [LynxAnimaXResourceFetcherLoader loaderWithLynxUIContext:self.context];
  if (httpLoader) {
    [loaders addObject:httpLoader];
  } else {
    ANIMAX_LOGI("httpLoader is null");
  }

  // Return an immutable copy of the loaders array
  return [loaders copy];
}

@end
