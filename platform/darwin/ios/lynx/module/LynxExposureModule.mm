// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxExposureModule.h"
#import "LynxContext+Internal.h"
#import "LynxContext.h"
#import "LynxUIContext.h"
#import "LynxUIExposure.h"
#import "LynxUIOwner.h"

using namespace lynx;

@implementation LynxExposureModule {
  __weak LynxContext *context_;
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    context_ = context;
  }

  return self;
}

+ (NSString *)name {
  return @"LynxExposureModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"stopExposure" : NSStringFromSelector(@selector(stopExposure:)),
    @"resumeExposure" : NSStringFromSelector(@selector(resumeExposure)),
    @"setObserverFrameRate" : NSStringFromSelector(@selector(setObserverFrameRate:))
  };
}

typedef void (^LynxExposureBlock)(LynxExposureModule *);

- (void)runOnUIThreadSafely:(LynxExposureBlock)block {
  __weak LynxExposureModule *weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    if (weakSelf) {
      __strong LynxExposureModule *strongSelf = weakSelf;
      if (strongSelf->context_) {
        block(strongSelf);
      }
    }
  });
}

// stop exposure detection, use options to control the behavior of stopExposure.
// called by frontend
- (void)stopExposure:(NSDictionary *)options {
  [self runOnUIThreadSafely:^(LynxExposureModule *target) {
    LynxUIExposure *exposure = [self exposure];
    [exposure stopExposure:options];
  }];
}

// resume exposure detection, send exposure event for all exposed ui on screen.
// called by frontend
- (void)resumeExposure {
  [self runOnUIThreadSafely:^(LynxExposureModule *target) {
    LynxUIExposure *exposure = [self exposure];
    [exposure resumeExposure];
  }];
}

- (void)setObserverFrameRate:(NSDictionary *)options {
  [self runOnUIThreadSafely:^(LynxExposureModule *target) {
    LynxUIExposure *exposure = [self exposure];
    [exposure setObserverFrameRateDynamic:options];
  }];
}

- (LynxUIExposure *)exposure {
  return context_.uiOwner.uiContext.uiExposure;
}

@end
