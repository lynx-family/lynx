// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <LynxServiceAPI/LynxServiceTrailProtocol.h>
#import <LynxServiceAPI/ServiceAPI.h>

#import <Lynx/LynxService.h>

@implementation LynxServiceHelper
+ (nullable id<LynxServiceTrailExtensionProtocol>)getLynxTrailExtensionService {
  static id<LynxServiceTrailExtensionProtocol> extensionService = nil;
  static dispatch_once_t sTrailOnceToken;
  dispatch_once(&sTrailOnceToken, ^{
    id<LynxServiceTrailProtocol> service = LynxService(LynxServiceTrailProtocol);
    if (service != nil &&
        [service conformsToProtocol:@protocol(LynxServiceTrailExtensionProtocol)]) {
      extensionService = (id<LynxServiceTrailExtensionProtocol>)service;
    }
  });

  return extensionService;
}
@end
