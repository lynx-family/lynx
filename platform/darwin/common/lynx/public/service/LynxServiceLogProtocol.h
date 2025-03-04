// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_PUBLIC_SERVICE_LynxServiceLogProtocol_h
#define DARWIN_COMMON_LYNX_PUBLIC_SERVICE_LynxServiceLogProtocol_h

#import <Foundation/Foundation.h>
#import "LynxServiceProtocol.h"

@protocol LynxServiceLogProtocol <LynxServiceProtocol>

- (void *)getWriteFunction;

@end

#endif /* DARWIN_COMMON_LYNX_PUBLIC_SERVICE_LynxServiceLogProtocol_h */
