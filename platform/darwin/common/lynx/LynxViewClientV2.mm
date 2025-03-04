// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxViewClientV2.h"

@interface LynxPipelineInfo ()

@property(nonatomic, readwrite, nullable, copy) NSString *url;

@end

@implementation LynxPipelineInfo {
  NSString *_url;
  NSInteger _pipelineOrigin;
}

- (nonnull instancetype)initWithUrl:(nullable NSString *)url {
  self = [super self];
  if (self) {
    _url = url;
  }
  return self;
}

- (void)addPipelineOrigin:(NSInteger)pipelineOrigin {
  _pipelineOrigin |= pipelineOrigin;
}

@end
