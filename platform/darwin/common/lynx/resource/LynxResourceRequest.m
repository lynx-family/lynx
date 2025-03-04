// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxResourceRequest.h"

@implementation LynxResourceRequest

- (instancetype)initWithUrl:(NSString *)url {
  if (self = [super init]) {
    _url = url;
  }
  return self;
}

- (instancetype)initWithUrl:(NSString *)url type:(LynxResourceRequestType)type {
  if (self = [super init]) {
    _url = url;
    _type = type;
  }
  return self;
}

- (instancetype)initWithUrl:(NSString *)url andRequestParams:(id)requestParams {
  if (self = [super init]) {
    _url = url;
    _requestParams = requestParams;
  }
  return self;
}

- (LynxServiceResourceRequestParameters *_Nullable)getLynxResourceServiceRequestParams {
  if (_requestParams != nil &&
      [_requestParams isKindOfClass:[LynxServiceResourceRequestParameters class]]) {
    return (LynxServiceResourceRequestParameters *)_requestParams;
  }
  return nil;
};

@end
