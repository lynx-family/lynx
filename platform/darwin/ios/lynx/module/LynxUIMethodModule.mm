// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIMethodModule.h"
#import "LynxContext+Internal.h"
#import "LynxContext.h"
#import "LynxThreadManager.h"
#import "LynxUIOwner.h"

using namespace lynx;

@implementation LynxUIMethodModule {
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
  return @"LynxUIMethodModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"invokeUIMethod" : NSStringFromSelector(@selector(invokeUIMethod:
                                                                nodes:method:params:callback:)),
  };
}

+ (LynxUIMethodCallbackBlock)wrapInvokeUIMethodCallback:(LynxCallbackBlock)callback {
  return ^(int code, id _Nullable data) {
    if (callback) {
      callback(@{@"code" : @(code), @"data" : data ?: @{}});
    }
  };
}

+ (void)runOnUiThread:(dispatch_block_t)uiTask {
  if ([LynxThreadManager isMainQueue]) {
    uiTask();
    return;
  }
  base::UIThread::GetRunner()->PostTask([uiTask]() { uiTask(); });
}

// for compatibility with old getNodeRef
- (void)invokeUIMethod:(NSString *)componentId
                 nodes:(NSArray *)nodes
                method:(NSString *)method
                params:(NSDictionary *)params
              callback:(LynxCallbackBlock)callback {
  NSString *sign = kDefaultComponentID;
  if (componentId.length > 0) {
    sign = componentId;
  }
  __weak LynxUIMethodModule *weakSelf = self;
  [LynxUIMethodModule runOnUiThread:^{
    if (weakSelf) {
      __strong LynxUIMethodModule *strongSelf = weakSelf;
      LynxUIOwner *owner = strongSelf->context_.uiOwner;
      if (owner) {
        [owner invokeUIMethod:method
                       params:params
                     callback:[LynxUIMethodModule wrapInvokeUIMethodCallback:callback]
                     fromRoot:sign
                      toNodes:nodes];
      }
    }
  }];
}

@end
