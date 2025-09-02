// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "DemoMediaResourceFetcher.h"

@implementation DemoMediaResourceFetcher

- (NSString *)shouldRedirectUrl:(LynxResourceRequest *)request {
  if ([request.url hasPrefix:@"local://"]) {
    return [NSString stringWithFormat:@"Resource/%@", [request.url substringFromIndex:8]];
  }
  return request.url;
}

- (LynxResourceOptionalBool)isLocalResource:(NSURL *)url {
  return LynxResourceOptionalBoolUndefined;
}

@end
