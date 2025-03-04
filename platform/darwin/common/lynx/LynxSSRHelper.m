// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxSSRHelper.h"
#import "LynxErrorBehavior.h"

typedef NS_ENUM(NSInteger, LynxSSRHydrateStatus) {
  LynxSSRHydrateStatusUndefined = 0,
  LynxSSRHydrateStatusPending,
  LynxSSRHydrateStatusBeginning,
  LynxSSRHydrateStatusFailed,
  LynxSSRHydrateStatusSuccessful,
};

@implementation LynxSSRHelper {
  LynxSSRHydrateStatus _hydrateStatus;
}

- (instancetype)init {
  if (self = [super init]) {
    _hydrateStatus = LynxSSRHydrateStatusUndefined;
  }
  return self;
}

- (void)onLoadSSRDataBegan:(NSString*)url {
  _hydrateStatus = LynxSSRHydrateStatusPending;
}

- (void)onHydrateBegan:(NSString*)url {
  _hydrateStatus = LynxSSRHydrateStatusBeginning;
}

- (void)onHydrateFinished:(NSString*)url {
  _hydrateStatus = LynxSSRHydrateStatusSuccessful;
}

- (void)onErrorOccurred:(NSInteger)code sourceError:(NSError*)source {
  if (code == EBLynxAppBundleLoad) {
    _hydrateStatus = LynxSSRHydrateStatusFailed;
  }
}

- (BOOL)isHydratePending {
  return _hydrateStatus == LynxSSRHydrateStatusPending;
}

+ (NSArray*)processEventParams:(NSArray*)params {
  static NSString* cache_identify = @"from_ssr_cache";
  NSMutableArray* finalParams = params.mutableCopy;
  if (finalParams == nil) {
    finalParams = [NSMutableArray new];
  }
  [finalParams addObject:cache_identify];

  return finalParams.copy;
}

- (BOOL)shouldSendEventToSSR {
  if (_hydrateStatus == LynxSSRHydrateStatusPending ||
      _hydrateStatus == LynxSSRHydrateStatusBeginning ||
      _hydrateStatus == LynxSSRHydrateStatusFailed) {
    return YES;
  }
  return NO;
}

@end
