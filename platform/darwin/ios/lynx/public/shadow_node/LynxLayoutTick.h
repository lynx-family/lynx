// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

typedef void (^LynxOnLayoutBlock)(void);

// TODO: use producer and consumer mode
@interface LynxLayoutTick : NSObject

- (nonnull instancetype)init;

- (void)setLayoutBlock:(nonnull LynxOnLayoutBlock)block;
- (void)requestLayout;
- (void)requestLayout:(nonnull LynxOnLayoutBlock)block;
- (void)triggerLayout;
- (void)cancelLayoutRequest;

@end
