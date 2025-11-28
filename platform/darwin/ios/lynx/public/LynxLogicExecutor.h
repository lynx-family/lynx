// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Foundation/Foundation.h>
#import <Lynx/LynxViewGroup.h>

@class LynxView;
@class LynxTemplateBundle;

NS_ASSUME_NONNULL_BEGIN
extern NSString *const kSendGlobalEvent;

@protocol LynxLogicExecutor <NSObject>

/**
 * @param lynxView The LynxView that triggered the event.
 * @param event A dictionary containing event data. It should have a 'method' key for the method
 * name and an 'args' key for the parameters.
 */
- (void)onLynxEvent:(LynxView *)lynxView event:(NSDictionary *)event;

- (void)destroy;
@end
NS_ASSUME_NONNULL_END
