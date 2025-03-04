// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxResourceResponse.h"

const NSInteger LynxResourceResponseCodeSuccess = 0;
const NSInteger LynxResourceResponseCodeFailed = -1;

@implementation LynxResourceResponse

- (instancetype)initWithData:(id)data {
  if (self = [super init]) {
    _data = data;
  }
  return self;
}

- (instancetype)initWithError:(NSError *)error code:(NSInteger)code {
  if (self = [super init]) {
    _error = error;
    _code = code;
  }
  return self;
}

- (bool)success {
  return _data != nil;
}

@end
