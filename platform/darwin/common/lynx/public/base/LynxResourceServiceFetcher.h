// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXRESOURCESERVICEFETCHER_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXRESOURCESERVICEFETCHER_H_

#import <Lynx/LynxResourceFetcher.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxResourceServiceFetcher : NSObject <LynxResourceFetcher>

+ (BOOL)ensureLynxService;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXRESOURCESERVICEFETCHER_H_
