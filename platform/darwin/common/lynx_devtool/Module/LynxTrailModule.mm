// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxTrailModule.h>

#import <Lynx/LynxLog.h>
#import <Lynx/LynxService.h>

@implementation LynxTrailModule {
  __weak LynxContext *context_;
}

+ (NSString *)name {
  return @"LynxTrailModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"getSettings" : NSStringFromSelector(@selector(getSettings)),
    @"getLatestSettings" : NSStringFromSelector(@selector(getLatestSettings:reject:)),
  };
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    context_ = context;
  }
  return self;
}

- (NSDictionary *)getSettings {
  return [LynxTrail getAllValues];
}

- (void)getLatestSettings:(LynxCallbackBlock)resolve reject:(LynxCallbackBlock)reject {
  id<LynxServiceTrailProtocol> trailService = LynxTrail;
  if (!trailService) {
    reject(@{
      @"message" : @"Lynx Trail Service not registered",
    });
    return;
  }
  SEL selector = NSSelectorFromString(@"getLatestSettingsWithCallback:");
  if (![trailService respondsToSelector:selector]) {
    reject(@{
      @"message" : @"Lynx Trail Service does not support latest settings fetch",
    });
    return;
  }
  void (^callback)(NSDictionary *_Nullable settings, NSError *_Nullable error) =
      ^(NSDictionary *_Nullable settings, NSError *_Nullable error) {
        if (error) {
          reject(@{
            @"message" : error.localizedDescription ?: @"Fetch latest settings failed",
          });
          return;
        }
        resolve(settings ?: @{});
      };
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [(id)trailService performSelector:selector withObject:callback];
#pragma clang diagnostic pop
}

@end
