// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICELOGPROTOCOL_H_
#define BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICELOGPROTOCOL_H_

#import <Foundation/Foundation.h>
#import <LynxBase/LynxBaseServiceProtocol.h>

@protocol LynxBaseServiceLogProtocol <LynxBaseServiceProtocol>

- (void *)getWriteFunction;

@end

#endif  // BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICELOGPROTOCOL_H_
