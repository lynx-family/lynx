// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxEmbeddedModule.h>
#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxViewGroup.h>
#import <Lynx/LynxWeakProxy.h>
#import <objc/runtime.h>

@implementation LynxEmbeddedModule {
  __weak LynxContext *context_;
  __weak LynxViewGroup *lynxViewGroup_;
}

+ (NSString *)name {
  return @"LynxEmbeddedModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"updateData" : NSStringFromSelector(@selector(updateData:params:resolve:reject:)),
    @"getData" : NSStringFromSelector(@selector(getData:resolve:reject:)),
    @"setDataV2" : NSStringFromSelector(@selector(setDataV2:resolve:reject:)),
    @"getDataV2" : NSStringFromSelector(@selector(getDataV2:reject:)),
  };
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    context_ = context;
  }
  return self;
}

- (instancetype)initWithLynxContext:(LynxContext *)context WithParam:(id)param {
  self = [super init];
  if (self) {
    context_ = context;
    if ([param isKindOfClass:[LynxViewGroup class]]) {
      lynxViewGroup_ = (LynxViewGroup *)(object_getClass(param) == [LynxWeakProxy class]
                                             ? ((LynxWeakProxy *)param).target
                                             : param);
    }
  }
  return self;
}

- (LynxView *)getLynxViewById:(int)lynxViewId {
  if (lynxViewGroup_ != nil) {
    return [lynxViewGroup_ getLynxViewById:lynxViewId];
  }
  return nil;
}

- (void)updateData:(int)lynxViewId
            params:(NSDictionary *)params
           resolve:(LynxCallbackBlock)resolve
            reject:(LynxCallbackBlock)reject {
  LynxView *lynxView = [self getLynxViewById:lynxViewId];
  if (lynxView == nil) {
    return;
  }
  dispatch_async(dispatch_get_main_queue(), ^{
    LynxTemplateData *data = [[LynxTemplateData alloc] initWithDictionary:params];
    LynxUpdateMeta *updateMeta = [[LynxUpdateMeta alloc] init];
    [updateMeta setData:data];
    if (lynxView) {
      [lynxView updateMetaData:updateMeta];
      resolve(@"update");
    } else {
      reject(@"Can not get related LynxView");
    }
  });
}

- (void)getData:(int)lynxViewId
        resolve:(LynxCallbackBlock)resolve
         reject:(LynxCallbackBlock)reject {
  LynxView *lynxView = [self getLynxViewById:lynxViewId];
  if (lynxView == nil) {
    return;
  }
  dispatch_async(dispatch_get_main_queue(), ^{
    LynxTemplateData *data = [lynxView getTemplateData];
    resolve(data);
  });
}

- (void)setDataV2:(NSDictionary *)params
          resolve:(LynxCallbackBlock)resolve
           reject:(LynxCallbackBlock)reject {
  // V2 embedded APIs are callback-only. JS callers must pass resolve/reject callbacks.
  LynxView *lynxView = [context_ getLynxView];
  if (lynxView == nil) {
    if (reject) {
      reject(@{@"message" : @"Cannot get related lynxView."});
    }
    return;
  }

  dispatch_block_t block = ^{
    LynxTemplateData *data = [[LynxTemplateData alloc] initWithDictionary:params];
    LynxUpdateMeta *updateMeta = [[LynxUpdateMeta alloc] init];
    [updateMeta setData:data];
    [lynxView updateMetaData:updateMeta];
    if (resolve) {
      resolve(@{});
    }
  };
  if ([NSThread isMainThread]) {
    block();
  } else {
    dispatch_async(dispatch_get_main_queue(), block);
  }
}

- (void)getDataV2:(LynxCallbackBlock)resolve reject:(LynxCallbackBlock)reject {
  // V2 embedded APIs are callback-only. JS callers must pass resolve/reject callbacks.
  LynxView *lynxView = [context_ getLynxView];
  if (lynxView == nil) {
    if (reject) {
      reject(@{@"message" : @"Cannot get related lynxView."});
    }
    return;
  }

  dispatch_block_t block = ^{
    LynxTemplateData *data = [lynxView getTemplateData];
    if (resolve) {
      resolve(data);
    }
  };
  if ([NSThread isMainThread]) {
    block();
  } else {
    dispatch_async(dispatch_get_main_queue(), block);
  }
}
@end
