// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxResourceListener.h>

#pragma mark - resource load success

@implementation LynxResourceLoadSuccessInfo

- (instancetype)initWithURL:(NSString *)url {
  if (self = [super init]) {
    _url = url;
  }
  return self;
}

@end

#pragma mark - resource load failed

@implementation LynxResourceLoadFailedInfo

- (instancetype)initWithErrorMessage:(NSString *)errorMessage
                           errorCode:(NSInteger)errorCode
                     categorizedCode:(NSInteger)categorizedCode {
  if (self = [super init]) {
    _errorMessage = errorMessage;
    _errorCode = errorCode;
    _categorizedCode = categorizedCode;
  }
  return self;
}

@end
