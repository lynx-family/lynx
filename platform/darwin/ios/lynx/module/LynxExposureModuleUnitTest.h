// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxContextModule.h>
#import "LynxExposureModule.h"
#import "LynxUIExposureUnitTest.h"

@interface LynxExposureModule ()

- (LynxUIExposure *)exposure;
- (void)stopExposure:(NSDictionary *)options;
- (void)resumeExposure;
typedef void (^LynxExposureBlock)(LynxExposureModule *);
- (void)runOnUIThreadSafely:(LynxExposureBlock)block;

@end
