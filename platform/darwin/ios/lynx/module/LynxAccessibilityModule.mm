// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxAccessibilityModule.h"
#import "LynxContext+Internal.h"
#import "LynxContext.h"
#import "LynxUIOwner+Accessibility.h"
#import "LynxUIOwner.h"

static NSString *NAME = @"LynxAccessibilityModule";
static NSString *MSG_MUTATION_STYLES = @"mutation_styles";
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
static NSString *MSG_CONTENT = @"content";
#pragma clang diagnostic pop
static NSString *MSG_KEY = @"msg";

@interface LynxAccessibilityModule ()
@property(nonatomic, weak) LynxContext *context;

@end

@implementation LynxAccessibilityModule

+ (NSString *)name {
  return NAME;
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"registerMutationStyle" : NSStringFromSelector(@selector(registerMutationStyle:callback:)),
    @"accessibilityAnnounce" : NSStringFromSelector(@selector(accessibilityAnnounce:callback:)),
  };
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  if (self = [super init]) {
    _context = context;
  }
  return self;
}

- (void)invokeCallback:(LynxCallbackBlock)callback withParams:(NSDictionary *)params {
  if (callback) {
    callback(params);
  }
}

- (void)runOnUIThreadSafely:(dispatch_block_t)block {
  if (dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
      dispatch_queue_get_label(dispatch_get_main_queue())) {
    block();
  } else {
    dispatch_async(dispatch_get_main_queue(), block);
  }
}

- (void)registerMutationStyle:(NSDictionary *)prefetchData callback:(LynxCallbackBlock)callback {
  __weak typeof(self) weakSelf = self;
  [self runOnUIThreadSafely:^{
    LynxUIOwner *owner = weakSelf.context.uiOwner;
    if (!owner) {
      [weakSelf invokeCallback:callback
                    withParams:@{MSG_KEY : @"Init accessibility env error: uiOwner is null"}];
      return;
    }

    NSArray<NSString *> *paramsArray = prefetchData[MSG_MUTATION_STYLES];
    if (![paramsArray isKindOfClass:NSArray.class]) {
      [weakSelf
          invokeCallback:callback
              withParams:@{
                MSG_KEY : [NSString stringWithFormat:@"Params error with %@", MSG_MUTATION_STYLES]
              }];
      return;
    }

    __block BOOL checkString = YES;
    [paramsArray
        enumerateObjectsUsingBlock:^(NSString *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
          if (![obj isKindOfClass:NSString.class]) {
            *stop = YES;
            checkString = NO;
          }
        }];

    if (!checkString) {
      [weakSelf invokeCallback:callback
                    withParams:@{
                      MSG_KEY : [NSString
                          stringWithFormat:@"Params error with %@: params must be string values",
                                           MSG_MUTATION_STYLES]
                    }];
      return;
    }

    [owner setA11yFilter:[NSSet setWithArray:paramsArray]];

    [weakSelf invokeCallback:callback withParams:@{MSG_KEY : @"Success"}];
  }];
}

- (void)accessibilityAnnounce:(NSDictionary *)args callback:(LynxCallbackBlock)callback {
  __weak typeof(self) weakSelf = self;
  [self runOnUIThreadSafely:^{
    NSString *content = args[@"content"];
    if (content) {
      UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, content);
      [weakSelf invokeCallback:callback withParams:@{MSG_KEY : @"Success"}];
    } else {
      [weakSelf invokeCallback:callback withParams:@{MSG_KEY : @"Params error: no content found"}];
    }
  }];
}
@end
