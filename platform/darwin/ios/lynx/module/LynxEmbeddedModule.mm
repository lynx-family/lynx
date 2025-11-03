// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxEmbeddedModule.h>
#import <Lynx/LynxViewGroup.h>
#import "LynxContext+Internal.h"
#import "LynxEnv+Internal.h"

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
      lynxViewGroup_ = (LynxViewGroup *)param;
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
@end
